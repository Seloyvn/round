#include "Actors/RoundAbilityActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter1.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundCharacter4.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundCharacter6.h"
#include "Actors/RoundCharacterCleric.h"
#include "Actors/RoundCharacterChronomancer.h"
#include "Actors/RoundTerrainActor.h"
#include "Effects/RoundSilenceEffect.h"
#include "Effects/RoundBindEffect.h"
#include "Effects/RoundBleedEffect.h"
#include "Effects/RoundDelayedAbilityEffect.h"
#include "Effects/RoundInvisibilityEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundSlowEffect.h"
#include "Effects/RoundTauntEffect.h"
#include "Interfaces/RoundBoardDamageableInterface.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundFieldObjectActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundTerrainHelpers.h"
#include "Data/RoundSkillProgressionData.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundAbilityRuntime, Log, All);

ARoundAbilityActor::ARoundAbilityActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
}

void ARoundAbilityActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundAbilityActor, CurrentCooldown);
    DOREPLIFETIME(ARoundAbilityActor, CurrentCharges);
    DOREPLIFETIME(ARoundAbilityActor, RowName);
}

namespace
{
double FindAbilityValueCaseInsensitive(const TMap<FString, double>& Values, const FString& Key, bool& bOutFound)
{
    if (const double* ExactValue = Values.Find(Key))
    {
        bOutFound = true;
        return *ExactValue;
    }

    for (const TPair<FString, double>& Pair : Values)
    {
        if (Pair.Key.Equals(Key, ESearchCase::IgnoreCase))
        {
            bOutFound = true;
            return Pair.Value;
        }
    }

    bOutFound = false;
    return 0.0;
}

TArray<FString> BuildInitiativeCompatibleAbilityKeys(const FString& Key)
{
    TArray<FString> Keys;
    Keys.Add(Key);

    auto AddUniqueKey = [&Keys](FString Candidate)
    {
        if (!Candidate.IsEmpty())
        {
            Keys.AddUnique(MoveTemp(Candidate));
        }
    };

    FString Migrated = Key;
    Migrated.ReplaceInline(TEXT("CooldownTurns"), TEXT("CooldownInitiative"));
    Migrated.ReplaceInline(TEXT("DurationTurns"), TEXT("DurationInitiative"));
    if (!Migrated.Contains(TEXT("Initiative")) && Migrated.Contains(TEXT("Duration")))
    {
        Migrated += TEXT("Initiative");
    }
    AddUniqueKey(Migrated);

    FString Legacy = Key;
    Legacy.ReplaceInline(TEXT("CooldownInitiative"), TEXT("CooldownTurns"));
    Legacy.ReplaceInline(TEXT("DurationInitiative"), TEXT("DurationTurns"));
    if (Legacy.EndsWith(TEXT("Initiative")) && !Legacy.Contains(TEXT("CooldownTurns")) && !Legacy.Contains(TEXT("DurationTurns")))
    {
        Legacy.LeftChopInline(FCString::Strlen(TEXT("Initiative")));
    }
    AddUniqueKey(Legacy);

    return Keys;
}

ARoundModifierEffect* AddAbilityModifierEffect(
    ARoundCharacter* TargetCharacter,
    ARoundCharacter* CauserCharacter,
    const FName EffectId,
    const FString& DisplayName,
    const FString& Description,
    const int32 DurationInitiative,
    const double IncomingDamageMultiplier = 1.0,
    const double OutgoingDamageMultiplier = 1.0,
    const int32 MoveCostAdd = 0,
    const bool bBlockMovementBonuses = false,
    const bool bOnlyAffectCauserDamage = false,
    const int32 IncomingEffectDurationAdd = 0,
    const int32 DefenseIgnoreAdd = 0,
    const bool bOnlyAgainstRangedSources = false,
    const double HealFractionFromDamageDealt = 0.0)
{
    if (!TargetCharacter || !TargetCharacter->GetWorld())
    {
        return nullptr;
    }

    ARoundModifierEffect* ModifierEffect = TargetCharacter->GetWorld()->SpawnActor<ARoundModifierEffect>(ARoundModifierEffect::StaticClass());
    if (!ModifierEffect)
    {
        return nullptr;
    }

    ModifierEffect->EffectId = EffectId;
    ModifierEffect->DisplayName = DisplayName;
    ModifierEffect->Description = Description;
    ModifierEffect->IncomingDamageMultiplier = IncomingDamageMultiplier;
    ModifierEffect->OutgoingDamageMultiplier = OutgoingDamageMultiplier;
    ModifierEffect->MoveCostAdd = MoveCostAdd;
    ModifierEffect->bBlockMovementBonuses = bBlockMovementBonuses;
    ModifierEffect->bOnlyAffectCauserDamage = bOnlyAffectCauserDamage;
    ModifierEffect->IncomingEffectDurationAdd = IncomingEffectDurationAdd;
    ModifierEffect->DefenseIgnoreAdd = DefenseIgnoreAdd;
    ModifierEffect->bOnlyAgainstRangedSources = bOnlyAgainstRangedSources;
    ModifierEffect->HealFractionFromDamageDealt = HealFractionFromDamageDealt;
    ModifierEffect->CauserCharacter = CauserCharacter;
    ModifierEffect->SetDurationInitiative(FMath::Max(1, DurationInitiative), false);
    TargetCharacter->AddEffect(ModifierEffect);
    return ModifierEffect;
}

}

ERoundButtonState ARoundAbilityActor::GetAbilityVisibility() const
{
    if (!OwningCharacter || !OwningCharacter->Attributes)
    {
        return ERoundButtonState::Disabled;
    }

    if (OwningCharacter->HasTurnActionBlock())
    {
        return ERoundButtonState::Disabled;
    }

    if (!UsesCharges() && CurrentCooldown > 0)
    {
        return ERoundButtonState::Disabled;
    }

    if (!HasAvailableCharge())
    {
        return ERoundButtonState::Disabled;
    }

    const bool bIsSilenced = OwningCharacter->ActiveEffects.ContainsByPredicate([this](const ARoundEffectActor* Effect)
    {
        return IsValid(Effect) && Effect->PreventsAbilityUse(this);
    });
    if (bIsSilenced)
    {
        return ERoundButtonState::Disabled;
    }

    if (OwningCharacter->Attributes->CurrentInitiative < GetAbilityCost())
    {
        return ERoundButtonState::Disabled;
    }

    if (const ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(OwningCharacter))
    {
        if (!Chronomancer->HasEnoughFragments(GetAbilityFragmentCost()))
        {
            return ERoundButtonState::Disabled;
        }
    }

    const int32 ChainCost = FMath::Max(0, FMath::TruncToInt(GetAbilityValueFromKey(TEXT("ChainCost"))));
    if (ChainCost > 0)
    {
        const ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(OwningCharacter);
        if (!Character1 || !Character1->TryHasChain(ChainCost))
        {
            return ERoundButtonState::Disabled;
        }
    }

    return ERoundButtonState::Enabled;
}

double ARoundAbilityActor::GetAbilityValueFromKey(const FString& Key) const
{
    double Value = 0.0;
    return TryGetAbilityValueFromKey(Key, Value) ? Value : 0.0;
}

TMap<FString, double> ARoundAbilityActor::GetResolvedAbilityValues() const
{
    TSet<FString> Keys;
    if (AbilityData)
    {
        for (const TPair<FString, double>& Entry : AbilityData->Values)
        {
            Keys.Add(Entry.Key);
        }
        if (AbilityData->SkillProgression)
        {
            const TArray<FRoundSkillpointDetails>& Levels = AbilityData->SkillProgression->Levels;
            for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
            {
                const int32 Selection = static_cast<int32>((Points >> (TierIndex * 2)) & 0x3);
                const int32 LevelIndex = (TierIndex * 3) + (Selection - 1);
                if (Selection <= 0 || !Levels.IsValidIndex(LevelIndex))
                {
                    continue;
                }
                for (const TPair<FString, double>& Entry : Levels[LevelIndex].Values)
                {
                    Keys.Add(Entry.Key);
                }
            }
        }
    }

    TMap<FString, double> Resolved;
    for (const FString& Key : Keys)
    {
        double Value = 0.0;
        if (TryGetAbilityValueFromKey(Key, Value))
        {
            Resolved.Add(Key, Value);
        }
    }
    return Resolved;
}

bool ARoundAbilityActor::TryGetAbilityValueFromKey(const FString& Key, double& OutValue) const
{
    if (!AbilityData)
    {
        OutValue = 0.0;
        return false;
    }

    double ResolvedValue = 0.0;
    bool bFoundValue = false;
    const TArray<FString> CandidateKeys = BuildInitiativeCompatibleAbilityKeys(Key);
    for (const FString& CandidateKey : CandidateKeys)
    {
        const double FoundValue = FindAbilityValueCaseInsensitive(AbilityData->Values, CandidateKey, bFoundValue);
        if (bFoundValue)
        {
            ResolvedValue = FoundValue;
            break;
        }
    }

    if (AbilityData->SkillProgression)
    {
        const TArray<FRoundSkillpointDetails>& Levels = AbilityData->SkillProgression->Levels;
        for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
        {
            const int32 Selection = static_cast<int32>((Points >> (TierIndex * 2)) & 0x3);
            if (Selection <= 0)
            {
                continue;
            }

            const int32 LevelIndex = (TierIndex * 3) + (Selection - 1);
            if (!Levels.IsValidIndex(LevelIndex))
            {
                continue;
            }

            const FRoundSkillpointDetails& Level = Levels[LevelIndex];
            for (const FString& CandidateKey : CandidateKeys)
            {
                bool bFoundSkillValue = false;
                const double FoundSkillValue = FindAbilityValueCaseInsensitive(Level.Values, CandidateKey, bFoundSkillValue);
                if (bFoundSkillValue)
                {
                    ResolvedValue += FoundSkillValue;
                    bFoundValue = true;
                    break;
                }
            }
        }
    }

    OutValue = ResolvedValue;
    return bFoundValue;
}

int32 ARoundAbilityActor::GetAbilityDurationInitiativeFromKey(const FString& Key) const
{
    const int32 RawDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(Key)));

    if (RawDuration >= 400 && RawDuration % 40 == 0)
    {
        return RawDuration / 40;
    }

    return RawDuration;
}

int32 ARoundAbilityActor::GetAbilityDamage() const
{
    int32 Damage = AbilityData ? AbilityData->BaseDamage : 0;
    double ResolvedDamage = 0.0;
    if (TryGetAbilityValueFromKey(TEXT("Damage"), ResolvedDamage))
    {
        Damage = FMath::RoundToInt((AbilityData ? AbilityData->BaseDamage : 0.0) + ResolvedDamage);
    }

    return Damage;
}

int32 ARoundAbilityActor::GetAbilityCooldown() const
{
    double Value = 0.0;
    if (TryGetAbilityValueFromKey(TEXT("Cooldown"), Value))
    {
        int32 Cooldown = FMath::Max(0, FMath::RoundToInt((AbilityData ? AbilityData->Cooldown : 0.0) + Value));
        if (const ARoundCharacter4* Mage = Cast<ARoundCharacter4>(OwningCharacter)) Cooldown = FMath::RoundToInt(Cooldown * Mage->GetStoredElementCooldownMultiplier());
        return Cooldown;
    }

    int32 Cooldown = AbilityData ? AbilityData->Cooldown : 0;
    if (const ARoundCharacter4* Mage = Cast<ARoundCharacter4>(OwningCharacter)) Cooldown = FMath::RoundToInt(Cooldown * Mage->GetStoredElementCooldownMultiplier());
    return Cooldown;
}

int32 ARoundAbilityActor::GetAbilityMaxCharges() const
{
    const int32 MaxCharges = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Charges")));
    const int32 AlternateMaxCharges = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MaxCharges")));
    const int32 ResolvedMax = FMath::Max(MaxCharges, AlternateMaxCharges);
    return FMath::Max(1, ResolvedMax > 0 ? ResolvedMax : 1);
}

bool ARoundAbilityActor::UsesCharges() const
{
    return GetAbilityMaxCharges() > 1;
}

int32 ARoundAbilityActor::GetAbilityCost() const
{
    int32 Cost = 0;
    double Value = 0.0;
    if (TryGetAbilityValueFromKey(TEXT("Cost"), Value))
    {
        Cost = FMath::Max(0, FMath::RoundToInt(Value));
    }
    else
    {
        double InitiativeCost = 0.0;
        if (TryGetAbilityValueFromKey(TEXT("InitiativeCost"), InitiativeCost))
        {
            Cost = FMath::Max(0, FMath::RoundToInt((AbilityData ? AbilityData->InitiativeCost : 0.0) + InitiativeCost));
        }
        else
        {
            Cost = AbilityData ? AbilityData->InitiativeCost : 0;
        }
    }

    if (AbilityData && AbilityData->AbilityId == FName(TEXT("PuppetMove")))
    {
        if (const ARoundCharacter6* Character6 = Cast<ARoundCharacter6>(OwningCharacter))
        {
            Cost = FMath::Max(0, Cost - FMath::RoundToInt(Character6->GetPassiveValueFromKey(TEXT("PuppetMoveCostReduction"))));
        }
    }

    if (const ARoundCharacter4* Mage = Cast<ARoundCharacter4>(OwningCharacter))
    {
        Cost = FMath::Max(0, Cost - Mage->GetStoredElementAbilityCostReduction());
    }

    return Cost;
}

int32 ARoundAbilityActor::GetAbilityFragmentCost() const
{
    return FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("FragmentCost"))));
}

int32 ARoundAbilityActor::GetAbilityRange() const
{
    double Value = 0.0;
    if (TryGetAbilityValueFromKey(TEXT("Range"), Value))
    {
        int32 Range = FMath::Max(0, FMath::RoundToInt((AbilityData ? AbilityData->Range : 1.0) + Value));
        if (const ARoundCharacter4* Mage = Cast<ARoundCharacter4>(OwningCharacter)) Range += Mage->GetStoredElementRangeAdd();
        return Range;
    }

    int32 Range = AbilityData ? AbilityData->Range : 1;
    if (const ARoundCharacter4* Mage = Cast<ARoundCharacter4>(OwningCharacter)) Range += Mage->GetStoredElementRangeAdd();
    return Range;
}

ERoundDamageType ARoundAbilityActor::GetAbilityDamageType() const
{
    return AbilityData ? AbilityData->DamageType : ERoundDamageType::Physical;
}

int64 ARoundAbilityActor::GetAllSkillpoints() const
{
    return Points;
}

bool ARoundAbilityActor::HasSkillpointSkilled(int32 SkillpointIndex) const
{
    if (SkillpointIndex <= 0)
    {
        return false;
    }

    const int32 ZeroBasedIndex = SkillpointIndex - 1;
    const int32 TierIndex = ZeroBasedIndex / 3;
    const int32 ChoiceIndex = ZeroBasedIndex % 3;
    const int32 Selection = static_cast<int32>((Points >> (TierIndex * 2)) & 0x3);
    return Selection == (ChoiceIndex + 1);
}

void ARoundAbilityActor::SetCooldown(int32 NewCooldown)
{
    CurrentCooldown = FMath::Max(0, NewCooldown);
}

void ARoundAbilityActor::AdvanceCooldown()
{
    ReduceCooldown(1);
}

void ARoundAbilityActor::ReduceCooldown(int32 Amount)
{
    CurrentCooldown = FMath::Max(0, CurrentCooldown - FMath::Max(0, Amount));

    if (UsesCharges() && CurrentCooldown <= 0)
    {
        const int32 MaxCharges = GetAbilityMaxCharges();
        if (CurrentCharges < MaxCharges)
        {
            CurrentCharges = FMath::Min(MaxCharges, CurrentCharges + 1);
            if (CurrentCharges < MaxCharges)
            {
                CurrentCooldown = GetAbilityCooldown();
            }
        }
    }
}

int32 ARoundAbilityActor::OnTakeDamage(int32 InDamage)
{
    return InDamage;
}

void ARoundAbilityActor::InitializeCharges()
{
    CurrentCharges = GetAbilityMaxCharges();
}

void ARoundAbilityActor::OnGrantedToCharacter()
{
}

bool ARoundAbilityActor::RequiresHandCardSelection() const
{
    return false;
}

int32 ARoundAbilityActor::GetMinHandCardSelections() const
{
    return 0;
}

int32 ARoundAbilityActor::GetMaxHandCardSelections() const
{
    return 0;
}

bool ARoundAbilityActor::IsValidHandCardSelectionIndex(int32 CardIndex) const
{
    return CardIndex >= 0;
}

void ARoundAbilityActor::SetPendingHandSelectionIndices(const TArray<int32>& InIndices)
{
    PendingHandSelectionIndices = InIndices;
}

const TArray<int32>& ARoundAbilityActor::GetPendingHandSelectionIndices() const
{
    return PendingHandSelectionIndices;
}

void ARoundAbilityActor::ClearPendingHandSelectionIndices()
{
    PendingHandSelectionIndices.Reset();
}

bool ARoundAbilityActor::HasAvailableCharge() const
{
    return !UsesCharges() || CurrentCharges > 0;
}

bool ARoundAbilityActor::ConsumeCharge()
{
    if (!UsesCharges())
    {
        return true;
    }

    if (CurrentCharges <= 0)
    {
        return false;
    }

    --CurrentCharges;
    if (CurrentCharges < GetAbilityMaxCharges() && CurrentCooldown <= 0)
    {
        SetCooldown(GetAbilityCooldown());
    }
    return true;
}

void ARoundAbilityActor::RefreshRuntimeState()
{
    CurrentCharges = UsesCharges()
        ? FMath::Clamp(CurrentCharges, 0, GetAbilityMaxCharges())
        : 1;
}

bool ARoundAbilityActor::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    if (!AbilityData || !OwningCharacter || !OwningCharacter->CurrentTile)
    {
        return false;
    }

    if (GetAbilityVisibility() != ERoundButtonState::Enabled)
    {
        return false;
    }

    if (!AbilityData->bRequiresTargetTile && !AbilityData->bRequiresTargetCharacter)
    {
        return true;
    }

    if (!TargetTile || !IsTargetInRange(TargetTile))
    {
        return false;
    }

    return MatchesFieldState(TargetTile);
}

void ARoundAbilityActor::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (TargetTile)
    {
        OutTiles.AddUnique(const_cast<ARoundGridTile*>(TargetTile));
    }
}

FRoundActionResult ARoundAbilityActor::ExecuteAbility(ARoundGridTile* TargetTile)
{
    UE_LOG(
        LogRoundAbilityRuntime,
        Log,
        TEXT("ExecuteAbility Base Ability=%s RuntimeClass=%s AbilityId=%s TargetTile=%s"),
        *GetName(),
        *GetNameSafe(GetClass()),
        *(AbilityData ? AbilityData->AbilityId.ToString() : RowName.ToString()),
        *GetNameSafe(TargetTile));
    const int32 CoreOperation = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation")));
    if ((CoreOperation == 5 || CoreOperation == 13 || (CoreOperation >= 31 && CoreOperation <= 34)
        || CoreOperation == 41 || CoreOperation == 42 || CoreOperation == 47 || CoreOperation == 48) && OwningCharacter)
    {
        if (ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState())
        {
            return RoundGameState->ExecuteCoreAbility(OwningCharacter, this, nullptr, TargetTile);
        }
    }
    if ((CoreOperation == 1 || CoreOperation == 2 || CoreOperation == 4
        || (CoreOperation >= 6 && CoreOperation <= 11) || CoreOperation == 14 || CoreOperation == 15
        || CoreOperation == 26 || CoreOperation == 27 || CoreOperation == 30
        || (CoreOperation >= 36 && CoreOperation <= 40) || CoreOperation == 43
        || CoreOperation == 49 || CoreOperation == 51) && OwningCharacter)
    {
        ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
        if (ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState())
        {
            return RoundGameState->ExecuteCoreAbility(OwningCharacter, this, TargetCharacter);
        }
    }
    if ((CoreOperation == 3 || CoreOperation == 12 || CoreOperation == 28 || CoreOperation == 29
        || CoreOperation == 35 || (CoreOperation >= 44 && CoreOperation <= 46)
        || CoreOperation == 50) && OwningCharacter)
    {
        if (ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState())
        {
            return RoundGameState->ExecuteCoreAbility(OwningCharacter, this, OwningCharacter);
        }
    }
    return ExecuteDefaultAbility(TargetTile);
}

FRoundActionResult ARoundAbilityActor::ExecuteDelayedResolution(ARoundGridTile* TargetTile)
{
    return ExecuteResolvedAbility(TargetTile);
}

bool ARoundAbilityActor::ConsumeAbilityActivation(const ARoundGridTile* TargetTile, FRoundActionResult& OutResult)
{
    UE_LOG(
        LogRoundAbilityRuntime,
        Log,
        TEXT("ConsumeAbilityActivation Ability=%s RuntimeClass=%s AbilityId=%s TargetTile=%s Charges=%d Cooldown=%d"),
        *GetName(),
        *GetNameSafe(GetClass()),
        *(AbilityData ? AbilityData->AbilityId.ToString() : RowName.ToString()),
        *GetNameSafe(TargetTile),
        CurrentCharges,
        CurrentCooldown);
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes)
    {
        OutResult.Reason = TEXT("Ability target is not legal.");
        UE_LOG(
            LogRoundAbilityRuntime,
            Warning,
            TEXT("ConsumeAbilityActivation rejected Ability=%s RuntimeClass=%s Reason=%s"),
            *GetName(),
            *GetNameSafe(GetClass()),
            *OutResult.Reason);
        return false;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        OutResult.Reason = TEXT("Not enough initiative.");
        UE_LOG(
            LogRoundAbilityRuntime,
            Warning,
            TEXT("ConsumeAbilityActivation rejected Ability=%s RuntimeClass=%s Reason=%s"),
            *GetName(),
            *GetNameSafe(GetClass()),
            *OutResult.Reason);
        return false;
    }

    if (ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(OwningCharacter))
    {
        if (!Chronomancer->SpendFragments(GetAbilityFragmentCost()))
        {
            OutResult.Reason = TEXT("Not enough fragments.");
            OwningCharacter->Attributes->GainInitiative(GetAbilityCost());
            UE_LOG(
                LogRoundAbilityRuntime,
                Warning,
                TEXT("ConsumeAbilityActivation rejected Ability=%s RuntimeClass=%s Reason=%s"),
                *GetName(),
                *GetNameSafe(GetClass()),
                *OutResult.Reason);
            return false;
        }
    }

    if (ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(OwningCharacter))
    {
        const int32 ChainCost = FMath::Max(0, FMath::TruncToInt(GetAbilityValueFromKey(TEXT("ChainCost"))));
        if (ChainCost > 0 && !Character1->ConsumeChains(ChainCost, true))
        {
            OutResult.Reason = TEXT("Not enough chains.");
            OwningCharacter->Attributes->GainInitiative(GetAbilityCost());
            if (ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(OwningCharacter))
            {
                Chronomancer->GainFragments(GetAbilityFragmentCost());
            }
            UE_LOG(
                LogRoundAbilityRuntime,
                Warning,
                TEXT("ConsumeAbilityActivation rejected Ability=%s RuntimeClass=%s Reason=%s"),
                *GetName(),
                *GetNameSafe(GetClass()),
                *OutResult.Reason);
            return false;
        }
    }

    if (!ConsumeCharge())
    {
        OutResult.Reason = TEXT("No charges remaining.");
        UE_LOG(
            LogRoundAbilityRuntime,
            Warning,
            TEXT("ConsumeAbilityActivation rejected Ability=%s RuntimeClass=%s Reason=%s"),
            *GetName(),
            *GetNameSafe(GetClass()),
            *OutResult.Reason);
        return false;
    }

    if (!UsesCharges())
    {
        SetCooldown(GetAbilityCooldown());
    }
    UE_LOG(
        LogRoundAbilityRuntime,
        Log,
        TEXT("ConsumeAbilityActivation success Ability=%s RuntimeClass=%s ChargesNow=%d CooldownNow=%d"),
        *GetName(),
        *GetNameSafe(GetClass()),
        CurrentCharges,
        CurrentCooldown);
    return true;
}

bool ARoundAbilityActor::TryScheduleDelayedAbility(ARoundGridTile* TargetTile, FRoundActionResult& OutResult)
{
    const int32 DelayedExecutionInitiative = FMath::Max(
        GetAbilityDurationInitiativeFromKey(TEXT("DelayedExecutionInitiative")),
        GetAbilityDurationInitiativeFromKey(TEXT("DelayAbilityInitiative")));
    if (DelayedExecutionInitiative <= 0 || !OwningCharacter || !GetWorld())
    {
        return false;
    }

    ARoundDelayedAbilityEffect* DelayedEffect = GetWorld()->SpawnActor<ARoundDelayedAbilityEffect>(ARoundDelayedAbilityEffect::StaticClass());
    if (!DelayedEffect)
    {
        OutResult.Reason = TEXT("Failed to delay ability.");
        return false;
    }

    DelayedEffect->EffectId = FName(*FString::Printf(TEXT("%sDelayedExecution"), *GetName()));
    DelayedEffect->DisplayName = AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Delayed Ability");
    DelayedEffect->Description = AbilityData ? AbilityData->Description.ToString() : TEXT("Die Faehigkeit wird spaeter ausgeloest.");
    DelayedEffect->DelayedAbility = this;
    DelayedEffect->bAllowTurnsWhileDelayed =
        GetAbilityValueFromKey(TEXT("AllowTurnsWhileDelayed")) > 0.0
        || GetAbilityValueFromKey(TEXT("CanActWhileDelayed")) > 0.0;
    DelayedEffect->TrackedTargetCharacter = ResolveTargetCharacter(TargetTile);
    DelayedEffect->bPreferTrackedTargetCharacter =
        GetAbilityValueFromKey(TEXT("DelayedTracksTargetCharacter")) > 0.0
        || (DelayedEffect->TrackedTargetCharacter && OwningCharacter && DelayedEffect->TrackedTargetCharacter->Team == OwningCharacter->Team);
    DelayedEffect->bHasTargetTile = TargetTile != nullptr;
    DelayedEffect->TargetTilePosition = TargetTile ? TargetTile->GridPosition : FVector2D::ZeroVector;
    DelayedEffect->CauserCharacter = OwningCharacter;
    DelayedEffect->SetDurationInitiative(DelayedExecutionInitiative, false);
    OwningCharacter->AddEffect(DelayedEffect);

    OutResult.bSuccess = true;
    OutResult.Reason = TEXT("Ability delayed.");

    if (!DelayedEffect->bAllowTurnsWhileDelayed)
    {
        OwningCharacter->EndTurn();
    }

    return true;
}

FRoundActionResult ARoundAbilityActor::ExecuteDefaultAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!ConsumeAbilityActivation(TargetTile, Result))
    {
        return Result;
    }

    if (TryScheduleDelayedAbility(TargetTile, Result))
    {
        return Result;
    }

    return ExecuteResolvedAbility(TargetTile);
}

FRoundActionResult ARoundAbilityActor::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!OwningCharacter || !OwningCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (GetAbilityDamage() > 0)
    {
        bool bAnyDestroyed = false;
        ApplyAbilityDamageToTile(TargetTile, GetAbilityDamage(), DefenseIgnore, bAnyDestroyed, true);
    }

    const int32 DelayAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Delay"))));
    if (TargetCharacter && DelayAmount > 0)
    {
        TargetCharacter->ApplyDelay(DelayAmount);
    }

    const int32 HealAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Heal"))));
    if (HealAmount > 0)
    {
        ARoundCharacter* HealTarget = TargetCharacter ? TargetCharacter : OwningCharacter.Get();
        if (HealTarget)
        {
            HealTarget->Heal(HealAmount);
        }
    }

    const int32 DurationInitiative = FMath::Max(
        GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative")),
        GetAbilityDurationInitiativeFromKey(TEXT("SlowDurationInitiative")));
    const int32 LegacySlowDurationTurns = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SlowDuration"))));
    int32 DebuffDuration = DurationInitiative > 0 ? DurationInitiative : (LegacySlowDurationTurns > 0 ? LegacySlowDurationTurns * 40 : 40);
    if (const ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(OwningCharacter))
    {
        DebuffDuration = Character1->AdjustDebuffDurationInitiative(DebuffDuration);
    }

    const int32 ImmediateInitiativeLoss = FMath::Max(
        FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ImmediateInitiativeLoss"))),
        FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit"))));
    if (TargetCharacter && ImmediateInitiativeLoss > 0)
    {
        TargetCharacter->ApplyDelay(ImmediateInitiativeLoss);
    }

    if (TargetCharacter)
    {
        const double SlowAmount = FMath::Max(GetAbilityValueFromKey(TEXT("SlowAmount")), GetAbilityValueFromKey(TEXT("SlowStrength")));
        const int32 MoveCostAdd = FMath::Max(
            FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAddOnTarget"))),
            FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAdd"))));
        const bool bBlockMovementBonuses = GetAbilityValueFromKey(TEXT("BlockMovementBonusesOnTarget")) > 0.0 || GetAbilityValueFromKey(TEXT("BlockMovementBonuses")) > 0.0;
        const int32 InitiativeLossOnMove = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnMove"))));
        const int32 InitiativeLossOnExpire = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnExpire"))));
        if (SlowAmount > 0.0 || MoveCostAdd > 0 || bBlockMovementBonuses || InitiativeLossOnMove > 0 || InitiativeLossOnExpire > 0)
        {
            if (ARoundSlowEffect* SlowEffect = GetWorld()->SpawnActor<ARoundSlowEffect>(ARoundSlowEffect::StaticClass()))
            {
                SlowEffect->EffectId = FName(*FString::Printf(TEXT("%sSlow"), *GetName()));
                SlowEffect->DisplayName = AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Slow");
                SlowEffect->Description = AbilityData ? AbilityData->Description.ToString() : TEXT("Verlangsamt.");
                SlowEffect->Amount = SlowAmount;
                SlowEffect->InitiativeLossOnMove = InitiativeLossOnMove;
                SlowEffect->InitiativeLossOnExpire = InitiativeLossOnExpire;
                SlowEffect->bBlockMovementBonuses = bBlockMovementBonuses;
                SlowEffect->CauserCharacter = OwningCharacter;
                SlowEffect->SetDurationInitiative(DebuffDuration, false);
                TargetCharacter->AddEffect(SlowEffect);
            }
        }

        const int32 BleedDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BleedDamagePerTurn"))));
        const int32 BleedDuration = FMath::Max(GetAbilityDurationInitiativeFromKey(TEXT("BleedDurationInitiative")), 0);
        if (BleedDamage > 0 && BleedDuration > 0)
        {
            if (ARoundBleedEffect* BleedEffect = GetWorld()->SpawnActor<ARoundBleedEffect>(ARoundBleedEffect::StaticClass()))
            {
                BleedEffect->EffectId = FName(*FString::Printf(TEXT("%sBleed"), *GetName()));
                BleedEffect->DisplayName = TEXT("Bleed");
                BleedEffect->Description = TEXT("Erleidet zu Zugbeginn Schaden.");
                BleedEffect->Amount = BleedDamage;
                BleedEffect->CauserCharacter = OwningCharacter;
                BleedEffect->SetDurationInitiative(BleedDuration, false);
                TargetCharacter->AddEffect(BleedEffect);
            }
        }

        const FString AbilityIdString = AbilityData ? AbilityData->AbilityId.ToString() : FString();
        if (AbilityIdString.Equals(TEXT("Bind"), ESearchCase::IgnoreCase))
        {
            if (ARoundBindEffect* BindEffect = GetWorld()->SpawnActor<ARoundBindEffect>(ARoundBindEffect::StaticClass()))
            {
                BindEffect->EffectId = TEXT("Bind");
                BindEffect->DisplayName = AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Bind");
                BindEffect->Description = AbilityData ? AbilityData->Description.ToString() : TEXT("Bestrafte Bewegung.");
                BindEffect->TriggerDamage = GetAbilityDamage();
                BindEffect->DelayAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DelayAmount"))));
                BindEffect->TriggerOutgoingDamageMultiplier = GetAbilityValueFromKey(TEXT("TriggerOutgoingDamageMultiplier"));
                BindEffect->TriggerSlowAmount = FMath::Max(GetAbilityValueFromKey(TEXT("TriggerSlowAmount")), 0.0);
                BindEffect->TriggerSlowDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TriggerSlowDuration"))));
                BindEffect->TriggerAoEInitiativeLossRadius = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TriggerAoEInitiativeLossRadius"))));
                BindEffect->TriggerAoEInitiativeLossAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TriggerAoEInitiativeLossAmount"))));
                BindEffect->TriggerInitiativeGainForCauser = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TriggerInitiativeGainForCauser"))));
                BindEffect->bBlockMovementBonusesOnTarget = bBlockMovementBonuses;
                BindEffect->CauserCharacter = OwningCharacter;
                BindEffect->SetDurationInitiative(DebuffDuration, false);
                TargetCharacter->AddEffect(BindEffect);
            }
        }

        if (AbilityIdString.Equals(TEXT("Taunt"), ESearchCase::IgnoreCase))
        {
            if (ARoundTauntEffect* TauntEffect = GetWorld()->SpawnActor<ARoundTauntEffect>(ARoundTauntEffect::StaticClass()))
            {
                TauntEffect->EffectId = TEXT("Taunt");
                TauntEffect->DisplayName = AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Taunt");
                TauntEffect->Description = AbilityData ? AbilityData->Description.ToString() : TEXT("Verursacht gegen andere Ziele weniger Schaden.");
                const double ReductionAmount = GetAbilityValueFromKey(TEXT("ReductionAmount"));
                const double OutgoingDamageMultiplierWhileTaunted = GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierWhileTaunted"));
                TauntEffect->Amount = OutgoingDamageMultiplierWhileTaunted > 0.0
                    ? OutgoingDamageMultiplierWhileTaunted
                    : (ReductionAmount > 0.0 ? 1.0 - ReductionAmount : 0.85);
                TauntEffect->InitiativeLossWhenIgnoringCauser = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossWhenIgnoringCauser"))));
                TauntEffect->InitiativeGainForCauserWhenAttacked = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainForCauserWhenAttacked"))));
                TauntEffect->CauserCharacter = OwningCharacter;
                TauntEffect->SetDurationInitiative(DebuffDuration, false);
                TargetCharacter->AddEffect(TauntEffect);
            }
        }

        const double IncomingDamageMultiplierFromCauser = FMath::Max(
            GetAbilityValueFromKey(TEXT("IncomingDamageMultiplierFromCauser")),
            GetAbilityValueFromKey(TEXT("IncomingDamageMultiplierOnHit")));
        if (IncomingDamageMultiplierFromCauser > 0.0 && IncomingDamageMultiplierFromCauser != 1.0)
        {
            AddAbilityModifierEffect(
                TargetCharacter,
                OwningCharacter,
                TEXT("AbilityIncomingDamageFromCauser"),
                AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Exposed"),
                TEXT("Erleidet mehr Schaden vom Verursacher."),
                DebuffDuration,
                IncomingDamageMultiplierFromCauser,
                1.0,
                0,
                false,
                true);
        }

        const double OutgoingDamageMultiplierOnTarget = FMath::Min(
            GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierOnTarget")) > 0.0 ? GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierOnTarget")) : 1.0,
            GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierAfterHit")) > 0.0 ? GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierAfterHit")) : 1.0);
        if (OutgoingDamageMultiplierOnTarget > 0.0 && OutgoingDamageMultiplierOnTarget < 1.0)
        {
            AddAbilityModifierEffect(
                TargetCharacter,
                OwningCharacter,
                TEXT("AbilityOutgoingDamageReduction"),
                AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Weakened"),
                TEXT("Verursacht weniger Schaden."),
                DebuffDuration,
                1.0,
                OutgoingDamageMultiplierOnTarget);
        }

        const int32 DefenseIgnoreDebuff = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnoreDebuff"))));
        const int32 IncomingEffectDurationAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("IncomingEffectDurationAddOnTarget"))));
        if (DefenseIgnoreDebuff > 0 || IncomingEffectDurationAdd > 0 || MoveCostAdd > 0 || bBlockMovementBonuses)
        {
            AddAbilityModifierEffect(
                TargetCharacter,
                OwningCharacter,
                TEXT("AbilityTargetModifier"),
                AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Debuffed"),
                AbilityData ? AbilityData->Description.ToString() : TEXT("Geschwaecht."),
                DebuffDuration,
                1.0,
                1.0,
                MoveCostAdd,
                bBlockMovementBonuses,
                false,
                IncomingEffectDurationAdd,
                DefenseIgnoreDebuff);
        }
    }

    if (OwningCharacter)
    {
        const int32 SelfInitiativeGainOnUse = FMath::Max(
            FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SelfInitiativeGainOnUse"))),
            FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnUse"))));
        if (SelfInitiativeGainOnUse > 0 && OwningCharacter->Attributes)
        {
            OwningCharacter->Attributes->CurrentInitiative += SelfInitiativeGainOnUse;
        }

        const double SelfIncomingDamageMultiplier = FMath::Min(
            GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplierAfterUse")) > 0.0 ? GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplierAfterUse")) : 1.0,
            GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplier")) > 0.0 ? GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplier")) : 1.0);
        const double SelfOutgoingDamageMultiplier = FMath::Max(
            GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierAfterUse")),
            GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierAfterUseSecondary")));
        const double LifestealFractionAfterUse = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("LifestealFractionAfterUse")));
        if ((SelfIncomingDamageMultiplier > 0.0 && SelfIncomingDamageMultiplier < 1.0)
            || SelfOutgoingDamageMultiplier > 1.0
            || LifestealFractionAfterUse > 0.0)
        {
            AddAbilityModifierEffect(
                OwningCharacter,
                OwningCharacter,
                TEXT("AbilitySelfBuff"),
                AbilityData ? AbilityData->DisplayName.ToString() : TEXT("Buff"),
                AbilityData ? AbilityData->Description.ToString() : TEXT("Selbstbuff."),
                DebuffDuration,
                SelfIncomingDamageMultiplier > 0.0 ? SelfIncomingDamageMultiplier : 1.0,
                SelfOutgoingDamageMultiplier > 0.0 ? SelfOutgoingDamageMultiplier : 1.0,
                0,
                false,
                false,
                0,
                0,
                false,
                LifestealFractionAfterUse);
        }

        const FString AbilityIdString = AbilityData ? AbilityData->AbilityId.ToString() : FString();
        if ((AbilityIdString.Equals(TEXT("SmokeStep"), ESearchCase::IgnoreCase) || AbilityIdString.Equals(TEXT("Shadowstrike"), ESearchCase::IgnoreCase))
            && DebuffDuration > 0)
        {
            if (ARoundInvisibilityEffect* InvisibilityEffect = GetWorld()->SpawnActor<ARoundInvisibilityEffect>(ARoundInvisibilityEffect::StaticClass()))
            {
                InvisibilityEffect->EffectId = FName(*FString::Printf(TEXT("%sInvisibility"), *AbilityIdString));
                InvisibilityEffect->DisplayName = TEXT("Invisibility");
                InvisibilityEffect->Description = TEXT("Ist kurzzeitig unsichtbar.");
                InvisibilityEffect->CauserCharacter = OwningCharacter;
                InvisibilityEffect->SetDurationInitiative(DebuffDuration, false);
                OwningCharacter->AddEffect(InvisibilityEffect);
            }
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Ability executed.");
    return Result;
}

int32 ARoundAbilityActor::ApplyAbilityDamageToTarget(ARoundCharacter* TargetCharacter, const int32 BaseDamage, const int32 DefenseIgnore, bool& bDidDie)
{
    bDidDie = false;

    if (!TargetCharacter || !OwningCharacter)
    {
        return 0;
    }

    const ERoundDamageType DamageType = GetAbilityDamageType();
    int32 ImmediateDefenseIgnore = FMath::Max(0, DefenseIgnore);
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(BaseDamage, DamageType, TargetCharacter, ImmediateDefenseIgnore);
    return TargetCharacter->ApplyRoundDamage(FinalDamage, DamageType, ImmediateDefenseIgnore, OwningCharacter, bDidDie);
}

int32 ARoundAbilityActor::ApplyAbilityDamageToBoardActor(AActor* BoardActor, const int32 BaseDamage, const int32 DefenseIgnore, bool& bWasDestroyed)
{
    bWasDestroyed = false;
    if (!BoardActor || !BoardActor->GetClass()->ImplementsInterface(URoundBoardDamageableInterface::StaticClass()))
    {
        return 0;
    }

    if (!IRoundBoardDamageableInterface::Execute_CanReceiveBoardDamage(BoardActor))
    {
        return 0;
    }

    int32 ImmediateDefenseIgnore = DefenseIgnore;
    const int32 DamageTypeAdjustedDamage = OwningCharacter
        ? OwningCharacter->ModifyOutgoingDamage(BaseDamage, GetAbilityDamageType(), nullptr, ImmediateDefenseIgnore)
        : BaseDamage;
    const FRoundBoardDamageResult DamageResult = IRoundBoardDamageableInterface::Execute_ApplyBoardDamage(
        BoardActor,
        DamageTypeAdjustedDamage,
        GetAbilityDamageType(),
        ImmediateDefenseIgnore,
        OwningCharacter);
    bWasDestroyed = DamageResult.bDestroyed;
    return DamageResult.AppliedDamage;
}

int32 ARoundAbilityActor::ApplyAbilityDamageToTile(ARoundGridTile* TargetTile, const int32 BaseDamage, const int32 DefenseIgnore, bool& bAnyDestroyed, const bool bIncludeCharacter)
{
    bAnyDestroyed = false;
    if (!TargetTile || BaseDamage <= 0)
    {
        return 0;
    }

    int32 HitCount = 0;

    if (bIncludeCharacter && TargetTile->OccupyingCharacter)
    {
        bool bDidDie = false;
        if (ApplyAbilityDamageToTarget(TargetTile->OccupyingCharacter, BaseDamage, DefenseIgnore, bDidDie) > 0)
        {
            ++HitCount;
            bAnyDestroyed = bAnyDestroyed || bDidDie;
        }
    }

    bool bObjectDestroyed = false;
    if (ApplyAbilityDamageToBoardActor(TargetTile->OccupyingFieldObject.Get(), BaseDamage, DefenseIgnore, bObjectDestroyed) > 0)
    {
        ++HitCount;
        bAnyDestroyed = bAnyDestroyed || bObjectDestroyed;
    }

    bool bTerrainDestroyed = false;
    if (ApplyAbilityDamageToBoardActor(TargetTile->TerrainObject.Get(), BaseDamage, DefenseIgnore, bTerrainDestroyed) > 0)
    {
        ++HitCount;
        bAnyDestroyed = bAnyDestroyed || bTerrainDestroyed;
    }

    if (GetAbilityDamageType() == ERoundDamageType::Fire && RoundTerrain::IsNatureTerrain(TargetTile->TerrainType))
    {
        TargetTile->SetTerrainType(ERoundTerrainType::Fire);
    }

    return HitCount;
}

void ARoundAbilityActor::NotifyAllyAffectedBySpell(ARoundCharacter* AffectedAlly)
{
    ARoundCharacterCleric* Cleric = Cast<ARoundCharacterCleric>(OwningCharacter);
    if (!Cleric || !AffectedAlly || AffectedAlly == Cleric || AffectedAlly->Team != Cleric->Team)
    {
        return;
    }

    Cleric->BlessAlly(AffectedAlly, 1);
}

bool ARoundAbilityActor::IsTargetInRange(const ARoundGridTile* TargetTile) const
{
    if (!OwningCharacter || !OwningCharacter->CurrentTile)
    {
        return false;
    }

    if (!TargetTile)
    {
        return !AbilityData || (!AbilityData->bRequiresTargetTile && !AbilityData->bRequiresTargetCharacter);
    }

    const int32 AbilityRange = FMath::Max(0, GetAbilityRange());
    return AbilityRange <= 0
        ? TargetTile == OwningCharacter->CurrentTile
        : OwningCharacter->CurrentTile->GetDistance(TargetTile) <= AbilityRange;
}

bool ARoundAbilityActor::MatchesFieldState(const ARoundGridTile* TargetTile) const
{
    if (!AbilityData || !OwningCharacter || !TargetTile)
    {
        return false;
    }

    ARoundCharacter* Occupant = TargetTile->OccupyingCharacter;
    switch (AbilityData->FieldState)
    {
    case ERoundFieldState::Empty:
        return Occupant == nullptr && TargetTile->OccupyingFieldObject == nullptr;
    case ERoundFieldState::Character:
        return Occupant != nullptr;
    case ERoundFieldState::Enemy:
        return Occupant && OwningCharacter->IsEnemyFor(Occupant);
    case ERoundFieldState::Ally:
        return Occupant && Occupant->Team == OwningCharacter->Team && Occupant != OwningCharacter;
    case ERoundFieldState::Self:
        return Occupant == OwningCharacter;
    case ERoundFieldState::Any:
    default:
        return true;
    }
}

ARoundCharacter* ARoundAbilityActor::ResolveTargetCharacter(const ARoundGridTile* TargetTile) const
{
    return TargetTile ? TargetTile->OccupyingCharacter : nullptr;
}

void ARoundAbilityActor::OnRep_CurrentCooldown()
{
    if (OwningCharacter)
    {
        if (ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState())
        {
            RoundGameState->OnBoardStateChanged.Broadcast();
        }
    }
}

void ARoundAbilityActor::OnRep_CurrentCharges()
{
    if (OwningCharacter)
    {
        if (ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState())
        {
            RoundGameState->OnBoardStateChanged.Broadcast();
        }
    }
}

void ARoundAbilityActor::OnRep_RowName()
{
    if (OwningCharacter)
    {
        OwningCharacter->InitializeCharacter();
    }
}
