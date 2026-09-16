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

void ADealrAIController::StopForEnding()
{
    ClearAITimers();
    bIsThinking = false;
    StopMovement();
    SetActorTickEnabled(false);
}


// ============================================================
// AI 턴 시작
// ============================================================

void ADealrAIController::TakeTurn(ABuckshotGameMode* GameMode)
{
    if (!GameMode || GameMode->bIsEndingPlaying)
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
    if (!CachedGameMode || CachedGameMode->bIsEndingPlaying)
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
    if (!CachedGameMode || CachedGameMode->bIsEndingPlaying)
    {
        return false;
    }


    // 기존 AI의 아이템 우선순위
    const TArray<EItemType> ItemPriority =
    {
        EItemType::Cigarette,
        EItemType::Magnifier,
        EItemType::Saw,
        EItemType::Handcuffs,
        EItemType::Beer,
        EItemType::Phone
    };


    // ========================================================
    // 우선순위 순서대로 아이템 확인
    // ========================================================

    for (EItemType ItemType : ItemPriority)
    {
        int32 ItemIndex = INDEX_NONE;

        if (!HasItem(ItemType, ItemIndex))
        {
            continue;
        }


        // ----------------------------------------------------
        // 담배
        // ----------------------------------------------------

        if (ItemType == EItemType::Cigarette)
        {
            if (CachedGameMode->GetDealerHP() >=
                CachedGameMode->GetMaxHP())
            {
                continue;
            }
        }


        // ----------------------------------------------------
        // 돋보기
        // ----------------------------------------------------

        if (ItemType == EItemType::Magnifier)
        {
            if (KnowNextShell.IsSet())
            {
                continue;
            }

            /*
             * 기존 AI의 돋보기 정보 확인.
             *
             * 실제 아이템 소모/지연 처리는
             * GameMode의 UseItemByType()에 맡긴다.
             */
        }


        // ----------------------------------------------------
        // 톱
        // ----------------------------------------------------

        if (ItemType == EItemType::Saw)
        {
            if (CachedGameMode->GetIsSawOff())
            {
                continue;
            }

            /*
             * 기존 AI에서는 다음 상태가 확인된 경우
             * 톱 사용을 우선하도록 되어 있었다.
             *
             * 여기서는 GameMode의 아이템 처리 구조와
             * 충돌하지 않도록 UseItemByType()을 사용한다.
             */
        }


        // ----------------------------------------------------
        // 수갑
        // ----------------------------------------------------

        if (ItemType == EItemType::Handcuffs)
        {
            if (CachedGameMode->GetIsCuff())
            {
                continue;
            }
        }


        // ----------------------------------------------------
        // 맥주
        // ----------------------------------------------------

        if (ItemType == EItemType::Beer)
        {
            /*
             * 기존 코드에서는 다음 상태를 알고 있고
             * 특정 조건일 때 사용하는 구조였다.
             *
             * 실제 처리는 GameMode에 맡긴다.
             */
        }


        // ----------------------------------------------------
        // 핸드폰
        // ----------------------------------------------------

        if (ItemType == EItemType::Phone)
        {
            if (CachedGameMode->GetMagazineCount() <= 1)
            {
                continue;
            }
        }


        // ====================================================
        // GameMode를 통해 아이템 사용
        // ====================================================

        if (CachedGameMode->UseItemByType(
            ItemType,
            false))
        {
            if (GEngine)
            {
                FString DebugText =
                    FString::Printf(
                        TEXT("[DealerAI] 아이템 사용 성공 -> %d"),
                        static_cast<int32>(ItemType)
                    );

                GEngine->AddOnScreenDebugMessage(
                    -1,
                    3.0f,
                    FColor::Yellow,
                    DebugText
                );
            }

            return true;
        }
    }


    // 사용할 아이템 없음
    return false;
}


// ============================================================
// AI 행동 판단
// ============================================================

void ADealrAIController::ShotDecision()
{
    if (!CachedGameMode || CachedGameMode->bIsEndingPlaying)
    {
        ClearAITimers();
        bIsThinking = false;
        return;
    }

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
