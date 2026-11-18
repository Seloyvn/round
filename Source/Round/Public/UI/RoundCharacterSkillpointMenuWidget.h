#pragma once

#include "CoreMinimal.h"
#include "UI/RoundSkillpointMenuWidget.h"
#include "RoundCharacterSkillpointMenuWidget.generated.h"

UCLASS(Blueprintable)
class ROUND_API URoundCharacterSkillpointMenuWidget : public URoundSkillpointMenuWidget
{
    GENERATED_BODY()

public:
    virtual void ReloadPoints() override;
    virtual bool LearnSkillpoint(int32 Point, int32 Value = 1) override;
    virtual bool UnlearnSkillpoint(int32 Point, int32 Value = 1) override;
    virtual int32 CurrentSkillPoints() const override;
    virtual int64 CurrentSkillMask() const override;
    virtual bool CurrentSkillLearned() const override;
    virtual bool IsTierUnlocked(int32 TierIndex) const override;
    virtual bool IsTierChoiceSelected(int32 TierIndex, int32 ChoiceIndex) const override;
    virtual bool CanSelectTierChoice(int32 TierIndex, int32 ChoiceIndex) const override;
    virtual bool SelectTierChoice(int32 TierIndex, int32 ChoiceIndex) override;
    virtual bool ClearTierChoice(int32 TierIndex) override;
    virtual bool LearnSkill() override;
    virtual bool UnlearnSkill() override;
};
