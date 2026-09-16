// Fill out your copyright notice in the Description page of Project Settings.

#include "DealrAIController.h"
#include "BuckshotGameMode.h"
#include "TimerManager.h"
#include "Engine/Engine.h"


// ============================================================
// Constructor
// ============================================================

ADealrAIController::ADealrAIController()
{
    CachedGameMode = nullptr;

    KnowLiveCount = 0;
    KnowBlankCount = 0;

    bIsThinking = false;
}


// ============================================================
// AI 타이머 전체 정리
// ============================================================

void ADealrAIController::ClearAITimers()
{
    GetWorldTimerManager().ClearTimer(DecisionTimerHandle);
    GetWorldTimerManager().ClearTimer(ItemDecisionTimerHandle);
}


// ============================================================
// AI 턴 시작
// ============================================================

void ADealrAIController::TakeTurn(ABuckshotGameMode* GameMode)
{
    if (!GameMode)
    {
        return;
    }

    // 이미 AI가 생각 중이면 중복 실행 방지
    if (bIsThinking)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                2.0f,
                FColor::Yellow,
                TEXT("[DealerAI] 이미 AI가 생각 중 -> TakeTurn 무시")
            );
        }

        return;
    }

    // 현재 GameMode 저장
    CachedGameMode = GameMode;

    // 이전에 남아 있던 AI 타이머 정리
    ClearAITimers();

    // AI 상태 시작
    bIsThinking = true;
    bHasUsedItemThisTurn = false;

    // 이전에 알고 있던 정보 초기화
    KnowNextShell.Reset();

    KnowLiveCount = 0;
    KnowBlankCount = 0;


    // ========================================================
    // 현재 탄창 정보 동기화
    // ========================================================

    for (EBulletType Shell : CachedGameMode->Magazine)
    {
        if (Shell == EBulletType::Live)
        {
            ++KnowLiveCount;
        }
        else
        {
            ++KnowBlankCount;
        }
    }


    // ========================================================
    // 디버그
    // ========================================================

    if (GEngine)
    {
        const FString DebugText =
            FString::Printf(
                TEXT("[DealerAI] 턴 시작 -> 1.5초 후 판단 | Live=%d Blank=%d"),
                KnowLiveCount,
                KnowBlankCount
            );

        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Green,
            DebugText
        );
    }


    // ========================================================
    // 1.5초 후 AI 판단
    // ========================================================

    GetWorldTimerManager().SetTimer(
        DecisionTimerHandle,
        this,
        &ADealrAIController::ShotDecision,
        1.5f,
        false
    );
}


// ============================================================
// 아이템 보유 여부 확인
// ============================================================

bool ADealrAIController::HasItem(
    EItemType ItemType,
    int32& OutIndex
) const
{
    OutIndex = INDEX_NONE;

    if (!CachedGameMode)
    {
        return false;
    }

    const TArray<FItemSlot>& DealerInventory =
        CachedGameMode->DealerInventory;


    for (int32 i = 0; i < DealerInventory.Num(); ++i)
    {
        const FItemSlot& Slot =
            DealerInventory[i];

        if (Slot.ItemType == ItemType &&
            Slot.Quantity > 0)
        {
            OutIndex = i;
            return true;
        }
    }

    return false;
}


// ============================================================
// 아이템 사용 판단
// ============================================================

void ADealrAIController::OnDecisionMade(ETargetType DecisionTarget)
{
    if (!CachedGameMode)
    {
        bIsThinking = false;
        return;
    }

    if (CachedGameMode->IsPlayerTurn)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                2.0f,
                FColor::Yellow,
                TEXT("[DealerAI] 턴 변경 감지 -> AI 판단 무시")
            );
        }

        bIsThinking = false;
        return;
    }

    PendingDecisionTarget = DecisionTarget;

    if (GEngine)
    {
        const TCHAR* TargetText =
            DecisionTarget == ETargetType::Opponent
            ? TEXT("Opponent")
            : TEXT("Self");

        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Cyan,
            FString::Printf(
                TEXT("[DealerAI] 최종 판단 -> Target: %s"),
                TargetText
            )
        );
    }

    CachedGameMode->ReceiveDealerDecision(DecisionTarget);

    bIsThinking = false;
}

bool ADealrAIController::TryUseItem()
{
    if (!CachedGameMode)
    {
        return false;
    }

    if (bHasUsedItemThisTurn)
    {
        return false;
    }

    if (CachedGameMode->IsPlayerTurn)
    {
        return false;
    }

    if (CachedGameMode->IsItemUseInProgress())
    {
        return false;
    }

    const int32 DealerHP =
        CachedGameMode->GetDealerHP();

    const int32 MaxHP =
        CachedGameMode->GetMaxHP();

    const int32 PlayerHP =
        CachedGameMode->PlayerHP;

    const int32 MagazineCount =
        CachedGameMode->GetMagazineCount();

    if (MagazineCount <= 0)
    {
        return false;
    }

    const int32 TotalShells =
        KnowLiveCount + KnowBlankCount;

    float LiveProbability = 0.5f;

    if (TotalShells > 0)
    {
        LiveProbability =
            static_cast<float>(KnowLiveCount) /
            static_cast<float>(TotalShells);
    }


    // ========================================================
    // 1. 담배
    // ========================================================

    if (DealerHP < MaxHP)
    {
        const bool bLowHP =
            DealerHP <= 1;

        const bool bWorthHealing =
            bLowHP ||
            DealerHP < MaxHP / 2;

        if (bWorthHealing)
        {
            int32 ItemIndex = INDEX_NONE;

            if (HasItem(EItemType::Cigarette, ItemIndex))
            {
                if (CachedGameMode->UseItemByType(
                    EItemType::Cigarette,
                    false))
                {
                    bHasUsedItemThisTurn = true;

                    //if (GEngine)
                    //{
                    //    GEngine->AddOnScreenDebugMessage(
                    //        -1,
                    //        3.0f,
                    //        FColor::Yellow,
                    //        TEXT("[DealerAI] 상황 판단 -> 체력이 낮아 회복 아이템 사용")
                    //    );
                    //}

                    return true;
                }
            }
        }
    }


    // ========================================================
    // 2. 돋보기
    // ========================================================

    if (!KnowNextShell.IsSet())
    {
        const bool bUncertainSituation =
            FMath::Abs(KnowLiveCount - KnowBlankCount) <= 1;

        if (bUncertainSituation)
        {
            int32 ItemIndex = INDEX_NONE;

            if (HasItem(EItemType::Magnifier, ItemIndex))
            {
                if (CachedGameMode->UseItemByType(
                    EItemType::Magnifier,
                    false))
                {
                    bHasUsedItemThisTurn = true;

                    //if (GEngine)
                    //{
                    //    GEngine->AddOnScreenDebugMessage(
                    //        -1,
                    //        3.0f,
                    //        FColor::Yellow,
                    //        TEXT("[DealerAI] 상황 판단 -> 탄창 정보가 불확실해 확인 아이템 사용")
                    //    );
                    //}

                    return true;
                }
            }
        }
    }


    // ========================================================
    // 3. 수갑
    // ========================================================

    if (!CachedGameMode->GetIsCuff())
    {
        const bool bPlayerHasAdvantage =
            PlayerHP > DealerHP;

        const bool bEnoughShells =
            MagazineCount >= 3;

        if (bPlayerHasAdvantage && bEnoughShells)
        {
            int32 ItemIndex = INDEX_NONE;

            if (HasItem(EItemType::Handcuffs, ItemIndex))
            {
                if (CachedGameMode->UseItemByType(
                    EItemType::Handcuffs,
                    false))
                {
                    bHasUsedItemThisTurn = true;

                    //if (GEngine)
                    //{
                    //    GEngine->AddOnScreenDebugMessage(
                    //        -1,
                    //        3.0f,
                    //        FColor::Yellow,
                    //        TEXT("[DealerAI] 상황 판단 -> 불리한 상황이라 제어 아이템 사용")
                    //    );
                    //}

                    return true;
                }
            }
        }
    }


    // ========================================================
    // 4. 톱
    // ========================================================

    if (!CachedGameMode->GetIsSawOff())
    {
        const bool bStrongSituation =
            LiveProbability >= 0.6f;

        const bool bEnoughShells =
            MagazineCount >= 2;

        if (bStrongSituation && bEnoughShells)
        {
            int32 ItemIndex = INDEX_NONE;

            if (HasItem(EItemType::Saw, ItemIndex))
            {
                if (CachedGameMode->UseItemByType(
                    EItemType::Saw,
                    false))
                {
                    bHasUsedItemThisTurn = true;

                    //if (GEngine)
                    //{
                    //    GEngine->AddOnScreenDebugMessage(
                    //        -1,
                    //        3.0f,
                    //        FColor::Yellow,
                    //        TEXT("[DealerAI] 상황 판단 -> 유리한 탄창 상황에서 강화 아이템 사용")
                    //    );
                    //}

                    return true;
                }
            }
        }
    }


    // ========================================================
    // 5. 맥주
    // ========================================================

    if (MagazineCount >= 4)
    {
        int32 ItemIndex = INDEX_NONE;

        if (HasItem(EItemType::Beer, ItemIndex))
        {
            if (CachedGameMode->UseItemByType(
                EItemType::Beer,
                false))
            {
                bHasUsedItemThisTurn = true;

                //if (GEngine)
                //{
                //    GEngine->AddOnScreenDebugMessage(
                //        -1,
                //        3.0f,
                //        FColor::Yellow,
                //        TEXT("[DealerAI] 상황 판단 -> 탄창이 길어 정리 아이템 사용")
                //    );
                //}

                return true;
            }
        }
    }


    // ========================================================
    // 6. 핸드폰
    // ========================================================

    if (MagazineCount >= 3)
    {
        const bool bInformationNeeded =
            !KnowNextShell.IsSet() &&
            FMath::Abs(KnowLiveCount - KnowBlankCount) <= 1;

        if (bInformationNeeded)
        {
            int32 ItemIndex = INDEX_NONE;

            if (HasItem(EItemType::Phone, ItemIndex))
            {
                if (CachedGameMode->UseItemByType(
                    EItemType::Phone,
                    false))
                {
                    bHasUsedItemThisTurn = true;

                    //if (GEngine)
                    //{
                    //    GEngine->AddOnScreenDebugMessage(
                    //        -1,
                    //        3.0f,
                    //        FColor::Yellow,
                    //        TEXT("[DealerAI] 상황 판단 -> 정보가 부족해 확인 아이템 사용")
                    //    );
                    //}

                    return true;
                }
            }
        }
    }


    return false;
}


// ============================================================
// AI 행동 판단
// ============================================================

void ADealrAIController::ShotDecision()
{
    // 현재 판단 타이머 정리
    GetWorldTimerManager().ClearTimer(
        DecisionTimerHandle
    );


    // ========================================================
    // GameMode 유효성 확인
    // ========================================================

    if (!CachedGameMode)
    {
        bIsThinking = false;
        return;
    }


    // ========================================================
    // 플레이어 턴으로 바뀌었으면
    // 이전 AI 판단은 무효
    // ========================================================

    if (CachedGameMode->IsPlayerTurn)
    {
        bIsThinking = false;

        ClearAITimers();

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                2.0f,
                FColor::Yellow,
                TEXT("[DealerAI] 이미 플레이어 턴 -> AI 판단 취소")
            );
        }

        return;
    }


    // ========================================================
    // 1. 아이템 사용 시도
    // ========================================================

    const bool bUsedItem =
        TryUseItem();


    if (bUsedItem)
    {
        /*
         * GameMode의 아이템 처리 시간이 1.5초이므로
         * 그보다 조금 뒤에 다시 판단한다.
         */

        GetWorldTimerManager().ClearTimer(
            ItemDecisionTimerHandle
        );


        GetWorldTimerManager().SetTimer(
            ItemDecisionTimerHandle,
            [this]()
            {
                // GameMode가 사라졌으면 종료
                if (!this->CachedGameMode)
                {
                    this->bIsThinking = false;
                    return;
                }


                // 플레이어 턴으로 바뀌었다면 종료
                if (this->CachedGameMode->IsPlayerTurn)
                {
                    this->bIsThinking = false;

                    this->ClearAITimers();

                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(
                            -1,
                            2.0f,
                            FColor::Yellow,
                            TEXT("[DealerAI] 아이템 처리 중 턴 변경 -> AI 종료")
                        );
                    }

                    return;
                }


                // 아이템 처리 후 다시 판단
                this->ShotDecision();
            },
            1.7f,
            false
        );


        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                2.0f,
                FColor::Yellow,
                TEXT("[DealerAI] 아이템 사용 완료 -> 1.7초 후 재판단")
            );
        }

        return;
    }


    // ========================================================
    // 2. 탄창 확인
    // ========================================================

    const int32 TotalShells =
        CachedGameMode->Magazine.Num();


    if (TotalShells <= 0)
    {
        bIsThinking = false;

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                2.0f,
                FColor::Yellow,
                TEXT("[DealerAI] 탄창이 비어 있음 -> AI 판단 종료")
            );
        }

        return;
    }


    // ========================================================
    // 3. 탄창 정보 재동기화
    // ========================================================

    KnowLiveCount = 0;
    KnowBlankCount = 0;


    for (EBulletType Shell :
    CachedGameMode->Magazine)
    {
        if (Shell == EBulletType::Live)
        {
            ++KnowLiveCount;
        }
        else
        {
            ++KnowBlankCount;
        }
    }


    // ========================================================
    // 4. 행동 대상 판단
    // ========================================================

    ETargetType DecisionTarget =
        ETargetType::Opponent;


    // --------------------------------------------------------
    // 이미 다음 상태를 알고 있는 경우
    // --------------------------------------------------------

    if (KnowNextShell.IsSet())
    {
        if (*KnowNextShell == EBulletType::Live)
        {
            DecisionTarget =
                ETargetType::Opponent;
        }
        else
        {
            DecisionTarget =
                ETargetType::Self;
        }
    }


    // --------------------------------------------------------
    // 다음 상태를 모르는 경우
    // --------------------------------------------------------

    else
    {
        const int32 TotalKnownShells =
            KnowLiveCount +
            KnowBlankCount;


        float LiveProbability = 0.5f;


        if (TotalKnownShells > 0)
        {
            LiveProbability =
                static_cast<float>(KnowLiveCount) /
                static_cast<float>(TotalKnownShells);
        }


        // 실탄 쪽 확률이 높은 경우
        if (LiveProbability > 0.5f)
        {
            DecisionTarget =
                ETargetType::Opponent;
        }


        // 공포탄 쪽 확률이 높은 경우
        else if (LiveProbability < 0.5f)
        {
            DecisionTarget =
                ETargetType::Self;
        }


        // 정확히 50:50이면 랜덤
        else
        {
            DecisionTarget =
                (FMath::RandRange(0, 1) == 0)
                ? ETargetType::Opponent
                : ETargetType::Self;
        }
    }


    // ========================================================
    // 판단 결과 출력
    // ========================================================

    if (GEngine)
    {
        const TCHAR* TargetText =
            DecisionTarget == ETargetType::Opponent
            ? TEXT("Opponent")
            : TEXT("Self");


        const FString DebugText =
            FString::Printf(
                TEXT("[DealerAI] 최종 판단 대상 = %s | Live=%d Blank=%d"),
                TargetText,
                KnowLiveCount,
                KnowBlankCount
            );


        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Green,
            DebugText
        );
    }


    // ========================================================
    // AI 판단 상태 초기화
    // ========================================================

    KnowNextShell.Reset();


    // ========================================================
   // 6. 최종 판단 전달
   // ========================================================

    if (!CachedGameMode)
    {
        bIsThinking = false;
        return;
    }

    if (CachedGameMode->IsPlayerTurn)
    {
        bIsThinking = false;
        return;
    }

    OnDecisionMade(DecisionTarget);
}