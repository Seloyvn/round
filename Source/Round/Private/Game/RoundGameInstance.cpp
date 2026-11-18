#include "Game/RoundGameInstance.h"

#include "Data/RoundGameDataRegistryLookup.h"
#include "Data/RoundTeamPresetData.h"
#include "Engine/DataTable.h"
#include "GameFramework/SaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

namespace
{
constexpr TCHAR TeamSaveSlotName[] = TEXT("Saves");

const FProperty* FindPropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    if (!Struct)
    {
        return nullptr;
    }

    const FString PrefixString(Prefix);

    for (TFieldIterator<FProperty> It(Struct); It; ++It)
    {
        const FProperty* Property = *It;
        if (!Property)
        {
            continue;
        }

        if (Property->GetName().Equals(PrefixString, ESearchCase::CaseSensitive)
            || Property->GetAuthoredName().Equals(PrefixString, ESearchCase::CaseSensitive))
        {
            return Property;
        }
    }

    for (TFieldIterator<FProperty> It(Struct); It; ++It)
    {
        const FProperty* Property = *It;
        if (!Property)
        {
            continue;
        }

        if (Property->GetName().StartsWith(PrefixString) || Property->GetAuthoredName().StartsWith(PrefixString))
        {
            return Property;
        }
    }

    return nullptr;
}

const FArrayProperty* FindArrayPropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FArrayProperty>(FindPropertyByPrefix(Struct, Prefix));
}

const FStructProperty* FindStructPropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FStructProperty>(FindPropertyByPrefix(Struct, Prefix));
}

const FStrProperty* FindStringPropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FStrProperty>(FindPropertyByPrefix(Struct, Prefix));
}

const FInt64Property* FindInt64PropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FInt64Property>(FindPropertyByPrefix(Struct, Prefix));
}

const FIntProperty* FindIntPropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FIntProperty>(FindPropertyByPrefix(Struct, Prefix));
}

const FByteProperty* FindBytePropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FByteProperty>(FindPropertyByPrefix(Struct, Prefix));
}

const FEnumProperty* FindEnumPropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FEnumProperty>(FindPropertyByPrefix(Struct, Prefix));
}

const FMapProperty* FindMapPropertyByPrefix(const UStruct* Struct, const TCHAR* Prefix)
{
    return CastField<FMapProperty>(FindPropertyByPrefix(Struct, Prefix));
}

FString ReadStringProperty(const UStruct* Struct, const void* StructData, const TCHAR* Prefix)
{
    if (const FStrProperty* Property = FindStringPropertyByPrefix(Struct, Prefix))
    {
        return Property->GetPropertyValue_InContainer(StructData);
    }

    return FString();
}

int64 ReadInt64Property(const UStruct* Struct, const void* StructData, const TCHAR* Prefix)
{
    if (const FInt64Property* Property = FindInt64PropertyByPrefix(Struct, Prefix))
    {
        return Property->GetPropertyValue_InContainer(StructData);
    }

    if (const FIntProperty* IntProperty = FindIntPropertyByPrefix(Struct, Prefix))
    {
        return IntProperty->GetPropertyValue_InContainer(StructData);
    }

    return 0;
}

FVector2D ReadVector2DProperty(const UStruct* Struct, const void* StructData, const TCHAR* Prefix)
{
    if (const FStructProperty* Property = FindStructPropertyByPrefix(Struct, Prefix))
    {
        if (Property->Struct == TBaseStructure<FVector2D>::Get())
        {
            if (const FVector2D* Value = Property->ContainerPtrToValuePtr<FVector2D>(StructData))
            {
                return *Value;
            }
        }
    }

    return FVector2D::ZeroVector;
}

ERoundFormat ReadFormatProperty(const UStruct* Struct, const void* StructData, const TCHAR* Prefix)
{
    if (const FEnumProperty* Property = FindEnumPropertyByPrefix(Struct, Prefix))
    {
        return static_cast<ERoundFormat>(Property->GetUnderlyingProperty()->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(StructData)));
    }

    if (const FByteProperty* Property = FindBytePropertyByPrefix(Struct, Prefix))
    {
        return static_cast<ERoundFormat>(Property->GetPropertyValue_InContainer(StructData));
    }

    return ERoundFormat::Format3x16;
}

ERoundTerrainType ReadTerrainTypeFromRaw(const FProperty* Property, const void* ValuePtr)
{
    if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        return static_cast<ERoundTerrainType>(ByteProperty->GetPropertyValue(ValuePtr));
    }

    if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        return static_cast<ERoundTerrainType>(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr));
    }

    return ERoundTerrainType::Ground;
}

double ReadDoubleFromRaw(const FProperty* Property, const void* ValuePtr)
{
    if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        return DoubleProperty->GetPropertyValue(ValuePtr);
    }

    if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        return static_cast<double>(FloatProperty->GetPropertyValue(ValuePtr));
    }

    if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        return static_cast<double>(IntProperty->GetPropertyValue(ValuePtr));
    }

    return 0.0;
}

FRoundFormatDefinition ReadFormatEntry(const UStruct* Struct, const void* StructData)
{
    FRoundFormatDefinition Result;
    Result.Format = ReadFormatProperty(Struct, StructData, TEXT("Format"));
    Result.Name = ReadStringProperty(Struct, StructData, TEXT("Name"));
    Result.Characters = static_cast<int32>(ReadInt64Property(Struct, StructData, TEXT("Characters")));
    Result.Skillpoints = static_cast<int32>(ReadInt64Property(Struct, StructData, TEXT("Skillpoints")));
    Result.FieldSize = static_cast<int32>(ReadInt64Property(Struct, StructData, TEXT("FieldSize")));
    Result.Terrain = static_cast<int32>(ReadInt64Property(Struct, StructData, TEXT("Terrain")));
    Result.StartPosition = ReadVector2DProperty(Struct, StructData, TEXT("StartPosition"));
    Result.SkillAmount = static_cast<int32>(ReadInt64Property(Struct, StructData, TEXT("SkillAmount")));

    if (const FMapProperty* TerrainWeightsProperty = FindMapPropertyByPrefix(Struct, TEXT("TerrainWeights")))
    {
        FScriptMapHelper MapHelper(TerrainWeightsProperty, TerrainWeightsProperty->ContainerPtrToValuePtr<void>(StructData));
        for (int32 Index = 0; Index < MapHelper.GetMaxIndex(); ++Index)
        {
            if (!MapHelper.IsValidIndex(Index))
            {
                continue;
            }

            Result.TerrainWeights.Add(
                ReadTerrainTypeFromRaw(TerrainWeightsProperty->KeyProp, MapHelper.GetKeyPtr(Index)),
                ReadDoubleFromRaw(TerrainWeightsProperty->ValueProp, MapHelper.GetValuePtr(Index)));
        }
    }

    return Result;
}

FRoundSkillEntry ReadSkillEntry(const UStruct* Struct, const void* StructData)
{
    FRoundSkillEntry Result;
    Result.Name = ReadStringProperty(Struct, StructData, TEXT("Name"));
    Result.Points = ReadInt64Property(Struct, StructData, TEXT("Points"));
    return Result;
}

FRoundCharacterEntry ReadCharacterEntry(const UStruct* Struct, const void* StructData)
{
    FRoundCharacterEntry Result;
    Result.StartPosition = ReadVector2DProperty(Struct, StructData, TEXT("StartPosition"));
    Result.Character = ReadStringProperty(Struct, StructData, TEXT("Character"));
    Result.Skillpoints = ReadInt64Property(Struct, StructData, TEXT("Skillpoints"));
    Result.CharacterSkillpoints = ReadInt64Property(Struct, StructData, TEXT("CharacterSkillpoints"));
    Result.CharacterSkillSelections = ReadInt64Property(Struct, StructData, TEXT("CharacterSkillSelections"));

    if (const FArrayProperty* SkillsProperty = FindArrayPropertyByPrefix(Struct, TEXT("Skills")))
    {
        if (const FStructProperty* SkillStructProperty = CastField<FStructProperty>(SkillsProperty->Inner))
        {
            FScriptArrayHelper SkillsArrayHelper(SkillsProperty, SkillsProperty->ContainerPtrToValuePtr<void>(StructData));
            Result.Skills.Reserve(SkillsArrayHelper.Num());

            for (int32 Index = 0; Index < SkillsArrayHelper.Num(); ++Index)
            {
                Result.Skills.Add(ReadSkillEntry(SkillStructProperty->Struct, SkillsArrayHelper.GetRawPtr(Index)));
            }
        }
    }

    return Result;
}

FRoundTeamDefinition ReadTeamEntry(const UStruct* Struct, const void* StructData)
{
    FRoundTeamDefinition Result;
    Result.Name = ReadStringProperty(Struct, StructData, TEXT("Name"));
    Result.Format = ReadFormatProperty(Struct, StructData, TEXT("Format"));

    if (const FArrayProperty* CharactersProperty = FindArrayPropertyByPrefix(Struct, TEXT("Characters")))
    {
        if (const FStructProperty* CharacterStructProperty = CastField<FStructProperty>(CharactersProperty->Inner))
        {
            FScriptArrayHelper CharacterArrayHelper(CharactersProperty, CharactersProperty->ContainerPtrToValuePtr<void>(StructData));
            Result.Characters.Reserve(CharacterArrayHelper.Num());

            for (int32 Index = 0; Index < CharacterArrayHelper.Num(); ++Index)
            {
                Result.Characters.Add(ReadCharacterEntry(CharacterStructProperty->Struct, CharacterArrayHelper.GetRawPtr(Index)));
            }
        }
    }

    return Result;
}

FRoundCharacterEntry MakePresetCharacter(const TCHAR* CharacterId, const int32 X, const int32 Y)
{
    FRoundCharacterEntry Result;
    Result.Character = CharacterId;
    Result.StartPosition = FVector2D(static_cast<double>(X), static_cast<double>(Y));
    return Result;
}

int64 SetPackedTierSelectionForPreset(const int64 PackedValue, const int32 TierIndex, const int32 SelectionValue)
{
    const int64 Shift = TierIndex * 2;
    const int64 Cleared = PackedValue & ~(static_cast<int64>(0x3) << Shift);
    return Cleared | ((static_cast<int64>(SelectionValue) & 0x3) << Shift);
}

int32 CountSelectedTiersForPreset(const int64 PackedValue)
{
    int32 Count = 0;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        if (((PackedValue >> (TierIndex * 2)) & 0x3) == 0)
        {
            break;
        }

        ++Count;
    }

    return Count;
}

FRoundSkillEntry MakeLearnedSkill(const TCHAR* SkillName, const int64 PackedSelections = 0)
{
    FRoundSkillEntry Result;
    Result.Name = SkillName;
    Result.Points = PackedSelections;
    return Result;
}

FRoundSkillEntry MakeThreeTierSkill(
    const TCHAR* SkillName,
    const int32 Tier0Selection,
    const int32 Tier1Selection,
    const int32 Tier2Selection)
{
    int64 PackedSelections = 0;
    PackedSelections = SetPackedTierSelectionForPreset(PackedSelections, 0, Tier0Selection);
    PackedSelections = SetPackedTierSelectionForPreset(PackedSelections, 1, Tier1Selection);
    PackedSelections = SetPackedTierSelectionForPreset(PackedSelections, 2, Tier2Selection);
    return MakeLearnedSkill(SkillName, PackedSelections);
}

FRoundSkillEntry MakeSkillWithSelections(const TCHAR* SkillName, std::initializer_list<int32> TierSelections)
{
    int64 PackedSelections = 0;
    int32 TierIndex = 0;
    for (const int32 Selection : TierSelections)
    {
        if (TierIndex >= 4 || Selection <= 0)
        {
            break;
        }

        PackedSelections = SetPackedTierSelectionForPreset(PackedSelections, TierIndex, Selection);
        ++TierIndex;
    }

    return MakeLearnedSkill(SkillName, PackedSelections);
}

FRoundCharacterEntry FinalizePresetCharacter(FRoundCharacterEntry Character)
{
    Character.Skillpoints = 0;
    for (const FRoundSkillEntry& Skill : Character.Skills)
    {
        Character.Skillpoints += CountSelectedTiersForPreset(Skill.Points);
    }

    Character.CharacterSkillpoints = CountSelectedTiersForPreset(Character.CharacterSkillSelections);

    return Character;
}

void SetPresetPassiveChoice(FRoundCharacterEntry& Character, const int32 TierIndex, const int32 SelectionValue)
{
    Character.CharacterSkillSelections = SetPackedTierSelectionForPreset(Character.CharacterSkillSelections, TierIndex, SelectionValue);
    Character.CharacterSkillpoints = CountSelectedTiersForPreset(Character.CharacterSkillSelections);
}

FRoundCharacterEntry BuildCharacter1Preset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character1"), X, Y);
    SetPresetPassiveChoice(Character, 0, 1);
    SetPresetPassiveChoice(Character, 1, 3);
    SetPresetPassiveChoice(Character, 2, 1);
    SetPresetPassiveChoice(Character, 3, 2);

    int64 Knockback = 0;
    Knockback = SetPackedTierSelectionForPreset(Knockback, 0, 3);
    Knockback = SetPackedTierSelectionForPreset(Knockback, 1, 1);
    Knockback = SetPackedTierSelectionForPreset(Knockback, 2, 2);

    int64 Delay = 0;
    Delay = SetPackedTierSelectionForPreset(Delay, 0, 1);
    Delay = SetPackedTierSelectionForPreset(Delay, 1, 1);
    Delay = SetPackedTierSelectionForPreset(Delay, 2, 1);

    int64 Shackle = 0;
    Shackle = SetPackedTierSelectionForPreset(Shackle, 0, 2);
    Shackle = SetPackedTierSelectionForPreset(Shackle, 1, 2);
    Shackle = SetPackedTierSelectionForPreset(Shackle, 2, 1);

    int64 Bind = 0;
    Bind = SetPackedTierSelectionForPreset(Bind, 0, 2);
    Bind = SetPackedTierSelectionForPreset(Bind, 1, 2);
    Bind = SetPackedTierSelectionForPreset(Bind, 2, 1);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Knockback"), {3, 1, 2, 1}),
        MakeSkillWithSelections(TEXT("Delay"), {1, 1}),
        MakeSkillWithSelections(TEXT("Shackle"), {2, 2, 1, 1}),
        MakeSkillWithSelections(TEXT("Bind"), {2, 2})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter1ControlPreset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character1"), X, Y);
    SetPresetPassiveChoice(Character, 0, 1);
    SetPresetPassiveChoice(Character, 1, 2);
    SetPresetPassiveChoice(Character, 2, 1);
    SetPresetPassiveChoice(Character, 3, 2);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Knockback"), {3, 1}),
        MakeSkillWithSelections(TEXT("Shackle"), {2, 2, 1, 1}),
        MakeSkillWithSelections(TEXT("Bind"), {2, 2, 1}),
        MakeSkillWithSelections(TEXT("Taunt"), {1, 2, 1})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter2Preset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character2"), X, Y);
    SetPresetPassiveChoice(Character, 0, 2);
    SetPresetPassiveChoice(Character, 1, 2);
    SetPresetPassiveChoice(Character, 2, 3);
    SetPresetPassiveChoice(Character, 3, 1);

    int64 Snipe = 0;
    Snipe = SetPackedTierSelectionForPreset(Snipe, 0, 2);
    Snipe = SetPackedTierSelectionForPreset(Snipe, 1, 3);
    Snipe = SetPackedTierSelectionForPreset(Snipe, 2, 2);

    int64 Concentrate = 0;
    Concentrate = SetPackedTierSelectionForPreset(Concentrate, 0, 2);
    Concentrate = SetPackedTierSelectionForPreset(Concentrate, 1, 2);
    Concentrate = SetPackedTierSelectionForPreset(Concentrate, 2, 1);

    int64 Aim = 0;
    Aim = SetPackedTierSelectionForPreset(Aim, 1, 1);
    Aim = SetPackedTierSelectionForPreset(Aim, 0, 3);
    Aim = SetPackedTierSelectionForPreset(Aim, 2, 1);

    int64 PiercingShot = 0;
    PiercingShot = SetPackedTierSelectionForPreset(PiercingShot, 0, 2);
    PiercingShot = SetPackedTierSelectionForPreset(PiercingShot, 1, 3);
    PiercingShot = SetPackedTierSelectionForPreset(PiercingShot, 2, 2);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Snipe"), {2, 3, 2, 1}),
        MakeSkillWithSelections(TEXT("Concentrate"), {2, 2}),
        MakeSkillWithSelections(TEXT("Aim"), {3, 1}),
        MakeSkillWithSelections(TEXT("PiercingShot"), {2, 3, 2})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter2HuntPreset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character2"), X, Y);
    SetPresetPassiveChoice(Character, 0, 2);
    SetPresetPassiveChoice(Character, 1, 2);
    SetPresetPassiveChoice(Character, 2, 3);
    SetPresetPassiveChoice(Character, 3, 3);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Snipe"), {2, 3, 2, 1}),
        MakeSkillWithSelections(TEXT("Concentrate"), {2, 2}),
        MakeSkillWithSelections(TEXT("PiercingShot"), {2, 3, 2}),
        MakeSkillWithSelections(TEXT("SuppressingFire"), {3, 2, 2})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter3Preset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character3"), X, Y);
    SetPresetPassiveChoice(Character, 0, 1);
    SetPresetPassiveChoice(Character, 1, 2);
    SetPresetPassiveChoice(Character, 2, 2);
    SetPresetPassiveChoice(Character, 3, 1);

    int64 Shadowstrike = 0;
    Shadowstrike = SetPackedTierSelectionForPreset(Shadowstrike, 0, 2);
    Shadowstrike = SetPackedTierSelectionForPreset(Shadowstrike, 1, 3);
    Shadowstrike = SetPackedTierSelectionForPreset(Shadowstrike, 2, 2);

    int64 Armorpen = 0;
    Armorpen = SetPackedTierSelectionForPreset(Armorpen, 0, 1);
    Armorpen = SetPackedTierSelectionForPreset(Armorpen, 1, 3);
    Armorpen = SetPackedTierSelectionForPreset(Armorpen, 2, 1);

    int64 Execute = 0;
    Execute = SetPackedTierSelectionForPreset(Execute, 0, 2);
    Execute = SetPackedTierSelectionForPreset(Execute, 1, 2);
    Execute = SetPackedTierSelectionForPreset(Execute, 2, 2);

    int64 HookShot = 0;
    HookShot = SetPackedTierSelectionForPreset(HookShot, 0, 3);
    HookShot = SetPackedTierSelectionForPreset(HookShot, 1, 1);
    HookShot = SetPackedTierSelectionForPreset(HookShot, 2, 2);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Shadowstrike"), {2, 3, 2}),
        MakeSkillWithSelections(TEXT("Armorpen"), {1, 3, 1}),
        MakeSkillWithSelections(TEXT("Execute"), {2, 2, 2, 1}),
        MakeSkillWithSelections(TEXT("HookShot"), {3, 1})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter3HuntPreset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character3"), X, Y);
    SetPresetPassiveChoice(Character, 0, 1);
    SetPresetPassiveChoice(Character, 1, 2);
    SetPresetPassiveChoice(Character, 2, 2);
    SetPresetPassiveChoice(Character, 3, 3);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Shadowstrike"), {2, 3}),
        MakeSkillWithSelections(TEXT("Armorpen"), {1, 3, 1}),
        MakeSkillWithSelections(TEXT("Execute"), {2, 2, 2, 1}),
        MakeSkillWithSelections(TEXT("SmokeStep"), {3, 3, 2})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter4Preset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character4"), X, Y);
    SetPresetPassiveChoice(Character, 0, 2);
    SetPresetPassiveChoice(Character, 1, 3);
    SetPresetPassiveChoice(Character, 2, 1);
    SetPresetPassiveChoice(Character, 3, 1);

    int64 Fireball = 0;
    Fireball = SetPackedTierSelectionForPreset(Fireball, 1, 3);
    Fireball = SetPackedTierSelectionForPreset(Fireball, 0, 1);
    Fireball = SetPackedTierSelectionForPreset(Fireball, 2, 1);

    int64 Icebeam = 0;
    Icebeam = SetPackedTierSelectionForPreset(Icebeam, 0, 2);
    Icebeam = SetPackedTierSelectionForPreset(Icebeam, 1, 3);
    Icebeam = SetPackedTierSelectionForPreset(Icebeam, 2, 2);

    int64 Lightningstrike = 0;
    Lightningstrike = SetPackedTierSelectionForPreset(Lightningstrike, 1, 2);
    Lightningstrike = SetPackedTierSelectionForPreset(Lightningstrike, 0, 2);
    Lightningstrike = SetPackedTierSelectionForPreset(Lightningstrike, 2, 1);

    int64 Rockblast = 0;
    Rockblast = SetPackedTierSelectionForPreset(Rockblast, 0, 2);
    Rockblast = SetPackedTierSelectionForPreset(Rockblast, 1, 2);
    Rockblast = SetPackedTierSelectionForPreset(Rockblast, 2, 3);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Fireball"), {1, 3, 1}),
        MakeSkillWithSelections(TEXT("Icebeam"), {2, 3, 2}),
        MakeSkillWithSelections(TEXT("Lightningstrike"), {2, 2}),
        MakeSkillWithSelections(TEXT("Rockblast"), {2, 2, 3, 1})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter4ControlPreset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character4"), X, Y);
    SetPresetPassiveChoice(Character, 0, 2);
    SetPresetPassiveChoice(Character, 1, 1);
    SetPresetPassiveChoice(Character, 2, 1);
    SetPresetPassiveChoice(Character, 3, 1);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Fireball"), {3, 2}),
        MakeSkillWithSelections(TEXT("Icebeam"), {2, 3, 2, 1}),
        MakeSkillWithSelections(TEXT("Rockblast"), {2, 2, 3, 1}),
        MakeSkillWithSelections(TEXT("ArcaneShift"), {2, 3})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter5Preset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character5"), X, Y);
    SetPresetPassiveChoice(Character, 0, 1);
    SetPresetPassiveChoice(Character, 1, 1);
    SetPresetPassiveChoice(Character, 2, 3);
    SetPresetPassiveChoice(Character, 3, 3);

    int64 Dash = 0;
    Dash = SetPackedTierSelectionForPreset(Dash, 0, 2);
    Dash = SetPackedTierSelectionForPreset(Dash, 1, 1);
    Dash = SetPackedTierSelectionForPreset(Dash, 2, 3);

    int64 Decimate = 0;
    Decimate = SetPackedTierSelectionForPreset(Decimate, 0, 2);
    Decimate = SetPackedTierSelectionForPreset(Decimate, 1, 1);
    Decimate = SetPackedTierSelectionForPreset(Decimate, 2, 2);

    int64 SpinToWin = 0;
    SpinToWin = SetPackedTierSelectionForPreset(SpinToWin, 1, 1);
    SpinToWin = SetPackedTierSelectionForPreset(SpinToWin, 0, 3);
    SpinToWin = SetPackedTierSelectionForPreset(SpinToWin, 2, 1);

    int64 Heal = 0;
    Heal = SetPackedTierSelectionForPreset(Heal, 1, 3);
    Heal = SetPackedTierSelectionForPreset(Heal, 0, 2);
    Heal = SetPackedTierSelectionForPreset(Heal, 2, 2);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Dash"), {2, 1, 3}),
        MakeSkillWithSelections(TEXT("Decimate"), {2, 1, 2, 1}),
        MakeSkillWithSelections(TEXT("SpinToWin"), {3, 1, 1}),
        MakeSkillWithSelections(TEXT("Heal"), {2, 3})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter5HunterPreset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character5"), X, Y);
    SetPresetPassiveChoice(Character, 0, 2);
    SetPresetPassiveChoice(Character, 1, 3);
    SetPresetPassiveChoice(Character, 2, 3);
    SetPresetPassiveChoice(Character, 3, 3);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("Dash"), {2, 1, 3}),
        MakeSkillWithSelections(TEXT("Decimate"), {2, 1, 2}),
        MakeSkillWithSelections(TEXT("Heal"), {2, 3}),
        MakeSkillWithSelections(TEXT("Rend"), {2, 3, 2, 1})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter6Preset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character6"), X, Y);
    SetPresetPassiveChoice(Character, 0, 1);
    SetPresetPassiveChoice(Character, 1, 2);
    SetPresetPassiveChoice(Character, 2, 1);
    SetPresetPassiveChoice(Character, 3, 2);

    int64 PuppetSwitch = 0;
    PuppetSwitch = SetPackedTierSelectionForPreset(PuppetSwitch, 0, 2);
    PuppetSwitch = SetPackedTierSelectionForPreset(PuppetSwitch, 1, 2);
    PuppetSwitch = SetPackedTierSelectionForPreset(PuppetSwitch, 2, 1);

    int64 PuppetSlash = 0;
    PuppetSlash = SetPackedTierSelectionForPreset(PuppetSlash, 0, 2);
    PuppetSlash = SetPackedTierSelectionForPreset(PuppetSlash, 1, 2);
    PuppetSlash = SetPackedTierSelectionForPreset(PuppetSlash, 2, 1);

    int64 PuppetHit = 0;
    PuppetHit = SetPackedTierSelectionForPreset(PuppetHit, 0, 3);
    PuppetHit = SetPackedTierSelectionForPreset(PuppetHit, 1, 1);
    PuppetHit = SetPackedTierSelectionForPreset(PuppetHit, 2, 1);

    int64 PuppetDefend = 0;
    PuppetDefend = SetPackedTierSelectionForPreset(PuppetDefend, 1, 1);
    PuppetDefend = SetPackedTierSelectionForPreset(PuppetDefend, 0, 2);
    PuppetDefend = SetPackedTierSelectionForPreset(PuppetDefend, 2, 1);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("PuppetSwitch"), {2, 2, 1}),
        MakeSkillWithSelections(TEXT("PuppetSlash"), {2, 2, 1, 1}),
        MakeSkillWithSelections(TEXT("PuppetHit"), {3, 1}),
        MakeSkillWithSelections(TEXT("PuppetDefend"), {2, 1, 1})
    };

    return FinalizePresetCharacter(Character);
}

FRoundCharacterEntry BuildCharacter6SnarePreset(const int32 X, const int32 Y)
{
    FRoundCharacterEntry Character = MakePresetCharacter(TEXT("Character6"), X, Y);
    SetPresetPassiveChoice(Character, 0, 1);
    SetPresetPassiveChoice(Character, 1, 2);
    SetPresetPassiveChoice(Character, 2, 2);
    SetPresetPassiveChoice(Character, 3, 3);

    Character.Skills = {
        MakeSkillWithSelections(TEXT("PuppetSwitch"), {2, 2}),
        MakeSkillWithSelections(TEXT("PuppetSlash"), {2, 2, 1}),
        MakeSkillWithSelections(TEXT("PuppetDefend"), {2, 1, 1, 1}),
        MakeSkillWithSelections(TEXT("StringSnare"), {1, 2, 2})
    };

    return FinalizePresetCharacter(Character);
}

FRoundTeamDefinition MakePresetTeam(const TCHAR* TeamName, const ERoundFormat Format, std::initializer_list<FRoundCharacterEntry> Characters)
{
    FRoundTeamDefinition Result;
    Result.Name = TeamName;
    Result.Format = Format;
    Result.Characters = Characters;
    return Result;
}

TArray<FRoundTeamDefinition> BuildDefaultTeamPresets()
{
    TArray<FRoundTeamDefinition> Result;

    FRoundTeamDefinition Frontdruck = MakePresetTeam(TEXT("Frontdruck"), ERoundFormat::Format3x16, {
        BuildCharacter1Preset(-1, 0),
        BuildCharacter5Preset( 0, 1),
        BuildCharacter4Preset( 1, -1)
    });
    Result.Add(Frontdruck);

    FRoundTeamDefinition Jagdkommando = MakePresetTeam(TEXT("Jagdkommando"), ERoundFormat::Format3x16, {
        BuildCharacter3HuntPreset(-1, 1),
        BuildCharacter2HuntPreset( 1, -1),
        BuildCharacter6SnarePreset( 0, 0)
    });
    Result.Add(Jagdkommando);

    FRoundTeamDefinition Raumkontrolle = MakePresetTeam(TEXT("Raumkontrolle"), ERoundFormat::Format3x16, {
        BuildCharacter1ControlPreset(-1, 1),
        BuildCharacter6SnarePreset( 0, 0),
        BuildCharacter4ControlPreset( 1, -1)
    });
    Result.Add(Raumkontrolle);

    FRoundTeamDefinition Blutjagd = MakePresetTeam(TEXT("Blutjagd"), ERoundFormat::Format3x16, {
        BuildCharacter5HunterPreset(-1, 1),
        BuildCharacter3HuntPreset( 0, 0),
        BuildCharacter2HuntPreset( 1, -1)
    });
    Result.Add(Blutjagd);

    return Result;
}

void AddBuiltInFormatDefinitions(TMap<ERoundFormat, FRoundFormatDefinition>& OutDefinitions)
{
    FRoundFormatDefinition Format3x16;
    Format3x16.Format = ERoundFormat::Format3x16;
    Format3x16.Name = TEXT("3-16");
    Format3x16.Characters = 3;
    Format3x16.Skillpoints = 16;
    Format3x16.FieldSize = 37;
    Format3x16.Terrain = 12;
    Format3x16.StartPosition = FVector2D::ZeroVector;
    Format3x16.SkillAmount = 4;
    Format3x16.TerrainWeights.Add(ERoundTerrainType::Ground, 0.55);
    Format3x16.TerrainWeights.Add(ERoundTerrainType::Forest, 0.12);
    Format3x16.TerrainWeights.Add(ERoundTerrainType::Street, 0.10);
    Format3x16.TerrainWeights.Add(ERoundTerrainType::Fire, 0.08);
    Format3x16.TerrainWeights.Add(ERoundTerrainType::Rocks, 0.08);
    Format3x16.TerrainWeights.Add(ERoundTerrainType::Rubble, 0.05);
    Format3x16.TerrainWeights.Add(ERoundTerrainType::Wall, 0.02);

    FRoundFormatDefinition FormatTest;
    FormatTest.Format = ERoundFormat::Formattest;
    FormatTest.Name = TEXT("test");
    FormatTest.Characters = 3;
    FormatTest.Skillpoints = 16;
    FormatTest.FieldSize = 19;
    FormatTest.Terrain = 0;
    FormatTest.StartPosition = FVector2D::ZeroVector;
    FormatTest.SkillAmount = 4;
    FormatTest.TerrainWeights.Add(ERoundTerrainType::Ground, 1.0);

    FRoundFormatDefinition Format5x25;
    Format5x25.Format = ERoundFormat::Format5x25;
    Format5x25.Name = TEXT("5-25");
    Format5x25.Characters = 5;
    Format5x25.Skillpoints = 25;
    Format5x25.FieldSize = 61;
    Format5x25.Terrain = 18;
    Format5x25.StartPosition = FVector2D::ZeroVector;
    Format5x25.SkillAmount = 5;
    Format5x25.TerrainWeights.Add(ERoundTerrainType::Ground, 0.52);
    Format5x25.TerrainWeights.Add(ERoundTerrainType::Forest, 0.13);
    Format5x25.TerrainWeights.Add(ERoundTerrainType::Street, 0.11);
    Format5x25.TerrainWeights.Add(ERoundTerrainType::Fire, 0.07);
    Format5x25.TerrainWeights.Add(ERoundTerrainType::Rocks, 0.08);
    Format5x25.TerrainWeights.Add(ERoundTerrainType::Rubble, 0.06);
    Format5x25.TerrainWeights.Add(ERoundTerrainType::Wall, 0.03);

    FRoundFormatDefinition Format5x16 = Format5x25;
    Format5x16.Format = ERoundFormat::Format5x16;
    Format5x16.Name = TEXT("5-16");
    Format5x16.Skillpoints = 16;

    OutDefinitions.FindOrAdd(Format3x16.Format) = Format3x16;
    OutDefinitions.FindOrAdd(FormatTest.Format) = FormatTest;
    OutDefinitions.FindOrAdd(Format5x25.Format) = Format5x25;
    OutDefinitions.FindOrAdd(Format5x16.Format) = Format5x16;
}
}

void URoundGameInstance::Init()
{
    Super::Init();

    InitializeDefaultFormats();
    LoadSavedRoundTeams();

    if (LoadedTeams.Num() > 0)
    {
        StrTeam = LoadedTeams[0];
    }
}

bool URoundGameInstance::AddOrReplaceTeam(const FTeamLoadout& TeamLoadout)
{
    if (TeamLoadout.TeamName.IsEmpty())
    {
        return false;
    }

    if (FTeamLoadout* ExistingTeam = SavedTeams.FindByPredicate([&TeamLoadout](const FTeamLoadout& Team)
    {
        return Team.TeamName.Equals(TeamLoadout.TeamName, ESearchCase::IgnoreCase);
    }))
    {
        *ExistingTeam = TeamLoadout;
        return true;
    }

    SavedTeams.Add(TeamLoadout);
    return true;
}

bool URoundGameInstance::TryGetFormatDefinition(const ERoundFormat Format, FRoundFormatDefinition& OutFormat) const
{
    if (const FRoundFormatDefinition* ExistingFormat = FormatDefinitions.Find(Format))
    {
        OutFormat = *ExistingFormat;
        return true;
    }

    // While formats are not exposed as a player choice, the single DataTable row
    // is authoritative even if an older team save still carries another enum ID.
    if (FormatDefinitions.Num() == 1)
    {
        OutFormat = FormatDefinitions.CreateConstIterator().Value();
        return true;
    }

    return false;
}

bool URoundGameInstance::TryGetPrimaryFormatDefinition(FRoundFormatDefinition& OutFormat) const
{
    if (FormatTable)
    {
        if (FormatTable->GetRowStruct() == FRoundFormatTableRow::StaticStruct())
        {
            static const FString ContextString(TEXT("PrimaryRoundFormat"));
            TArray<FRoundFormatTableRow*> Rows;
            FormatTable->GetAllRows(ContextString, Rows);
            if (!Rows.IsEmpty() && Rows[0])
            {
                OutFormat = Rows[0]->ToDefinition();
                return true;
            }
        }
        else if (const UScriptStruct* RowStruct = FormatTable->GetRowStruct())
        {
            for (const TPair<FName, uint8*>& RowPair : FormatTable->GetRowMap())
            {
                if (RowPair.Value)
                {
                    OutFormat = ReadFormatEntry(RowStruct, RowPair.Value);
                    return true;
                }
            }
        }
    }

    if (const FRoundFormatDefinition* Preferred = FormatDefinitions.Find(ERoundFormat::Format5x16))
    {
        OutFormat = *Preferred;
        return true;
    }
    for (const TPair<ERoundFormat, FRoundFormatDefinition>& Pair : FormatDefinitions)
    {
        OutFormat = Pair.Value;
        return true;
    }
    return false;
}

TArray<FRoundTeamDefinition> URoundGameInstance::GetBuiltInTeamPresets()
{
    if (const URoundTeamPresetData* Presets = RoundGameDataRegistryLookup::ResolveDefaultTeamPresets())
    {
        return Presets->Teams;
    }

    return {};
}

bool URoundGameInstance::TryGetBuiltInFormatDefinition(const ERoundFormat Format, FRoundFormatDefinition& OutFormat)
{
    TMap<ERoundFormat, FRoundFormatDefinition> BuiltInFormats;
    AddBuiltInFormatDefinitions(BuiltInFormats);
    if (const FRoundFormatDefinition* ExistingFormat = BuiltInFormats.Find(Format))
    {
        OutFormat = *ExistingFormat;
        return true;
    }

    return false;
}

void URoundGameInstance::InitializeDefaultFormats()
{
    if (FormatDefinitions.Num() > 0)
    {
        return;
    }

    UDataTable* ResolvedFormatTable = FormatTable;
    if (!ResolvedFormatTable)
    {
        ResolvedFormatTable = RoundGameDataRegistryLookup::ResolveFormatTable();
        FormatTable = ResolvedFormatTable;
    }

    if (ResolvedFormatTable && ResolvedFormatTable->GetRowStruct() == FRoundFormatTableRow::StaticStruct())
    {
        UE_LOG(LogTemp, Log, TEXT("RoundGameInstance InitializeDefaultFormats - loaded native format table rows=%d"), ResolvedFormatTable->GetRowMap().Num());

        static const FString ContextString(TEXT("RoundFormatTable"));
        TArray<FRoundFormatTableRow*> Rows;
        ResolvedFormatTable->GetAllRows(ContextString, Rows);
        for (FRoundFormatTableRow* Row : Rows)
        {
            if (!Row)
            {
                continue;
            }

            const FRoundFormatDefinition Parsed = Row->ToDefinition();
            FormatDefinitions.Add(Parsed.Format, Parsed);
        }
    }
    else if (ResolvedFormatTable)
    {
        UE_LOG(LogTemp, Log, TEXT("RoundGameInstance InitializeDefaultFormats - loaded legacy DT_Format rows=%d"), ResolvedFormatTable->GetRowMap().Num());
        if (const UScriptStruct* RowStruct = ResolvedFormatTable->GetRowStruct())
        {
            for (const TPair<FName, uint8*>& RowPair : ResolvedFormatTable->GetRowMap())
            {
                if (!RowPair.Value)
                {
                    continue;
                }

                FRoundFormatDefinition Parsed = ReadFormatEntry(RowStruct, RowPair.Value);
                if (Parsed.Name.IsEmpty())
                {
                    Parsed.Name = RowPair.Key.ToString();
                }

                FormatDefinitions.Add(Parsed.Format, Parsed);
                UE_LOG(LogTemp, Log, TEXT("RoundGameInstance Format row parsed - Name=%s FieldSize=%d Terrain=%d SkillAmount=%d Start=%s"),
                    *Parsed.Name,
                    Parsed.FieldSize,
                    Parsed.Terrain,
                    Parsed.SkillAmount,
                    *Parsed.StartPosition.ToString());
            }
        }
    }

    if (FormatDefinitions.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RoundGameInstance InitializeDefaultFormats - DT_Format empty/unavailable, using native fallback values"));
        AddBuiltInFormatDefinitions(FormatDefinitions);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("RoundGameInstance InitializeDefaultFormats - using DataTable formats=%d without native augmentation"), FormatDefinitions.Num());
}

void URoundGameInstance::LoadSavedRoundTeams()
{
    LoadedTeams.Reset();

    if (!UGameplayStatics::DoesSaveGameExist(TeamSaveSlotName, 0))
    {
        LoadedTeams = LoadDefaultTeamPresets();
        return;
    }

    USaveGame* SaveGameObject = UGameplayStatics::LoadGameFromSlot(TeamSaveSlotName, 0);
    if (!SaveGameObject)
    {
        LoadedTeams = LoadDefaultTeamPresets();
        return;
    }

    const FArrayProperty* TeamsProperty = FindArrayPropertyByPrefix(SaveGameObject->GetClass(), TEXT("Teams"));
    if (!TeamsProperty)
    {
        LoadedTeams = LoadDefaultTeamPresets();
        return;
    }

    const FStructProperty* TeamStructProperty = CastField<FStructProperty>(TeamsProperty->Inner);
    if (!TeamStructProperty)
    {
        LoadedTeams = LoadDefaultTeamPresets();
        return;
    }

    FScriptArrayHelper TeamArrayHelper(TeamsProperty, TeamsProperty->ContainerPtrToValuePtr<void>(SaveGameObject));
    LoadedTeams.Reserve(TeamArrayHelper.Num());

    for (int32 Index = 0; Index < TeamArrayHelper.Num(); ++Index)
    {
        LoadedTeams.Add(ReadTeamEntry(TeamStructProperty->Struct, TeamArrayHelper.GetRawPtr(Index)));
    }

    if (LoadedTeams.Num() == 0)
    {
        LoadedTeams = LoadDefaultTeamPresets();
    }
}

TArray<FRoundTeamDefinition> URoundGameInstance::LoadDefaultTeamPresets() const
{
    if (const URoundTeamPresetData* Presets = RoundGameDataRegistryLookup::ResolveDefaultTeamPresets())
    {
        return Presets->Teams;
    }

    UE_LOG(LogTemp, Error, TEXT("RoundGameInstance has no DefaultTeamPresets asset configured in DA_RoundGameDataRegistry"));
    return {};
}
