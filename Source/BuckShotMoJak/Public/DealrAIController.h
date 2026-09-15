#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BuckshotGameMode.h"
#include "DealrAIController.generated.h"

UCLASS()
class BUCKSHOTMOJAK_API ADealrAIController : public AAIController
{
    GENERATED_BODY()

public:

    ADealrAIController();

    void TakeTurn(ABuckshotGameMode* GameMode);

private:
    // AI가 최종적으로 결정한 대상
    ETargetType PendingDecisionTarget = ETargetType::Opponent;

    // AI 판단 완료 처리
    void OnDecisionMade(ETargetType DecisionTarget);

    void ShotDecision();

    bool TryUseItem();

    bool HasItem(
        EItemType ItemType,
        int32& OutIndex
    ) const;

    void ClearAITimers();

private:

    UPROPERTY()
    ABuckshotGameMode* CachedGameMode;

    FTimerHandle DecisionTimerHandle;

    FTimerHandle ItemDecisionTimerHandle;

    TOptional<EBulletType> KnowNextShell;

    int32 KnowLiveCount;

    int32 KnowBlankCount;

    bool bIsThinking;
};