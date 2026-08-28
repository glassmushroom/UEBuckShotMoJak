// Fill out your copyright notice in the Description page of Project Settings.
#include "DealrAIController.h"
#include "BuckshotGameMode.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

ADealrAIController::ADealrAIController()
{
	CachedGameMode = nullptr;
	KnowLiveCount = 0;
	KnowBlankCount = 0;
}

void ADealrAIController::TakeTurn(ABuckshotGameMode* GameMode)
{
	if (!GameMode) return;

	CachedGameMode = GameMode;

	// 탄창 카운트 동기화
	KnowLiveCount = 0;
	KnowBlankCount = 0;
	for (EBulletType Shell : CachedGameMode->Magazine)
	{
		if (Shell == EBulletType::Live) KnowLiveCount++;
		else KnowBlankCount++;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(FMath::Rand(), 12.0f, FColor::Green, TEXT("[DealerAI] 턴을 받았습니다. 사고 시작..."));
	}

	FTimerHandle AIThinkingHandle;
	GetWorldTimerManager().SetTimer(AIThinkingHandle, this, &ADealrAIController::ShotDecision, 1.5f, false);
}

void ADealrAIController::AddItem(EItemType NewItem)
{
	if (Inventory.Num() < 8)
	{
		Inventory.Add(NewItem);
	}
}

bool ADealrAIController::HasItem(EItemType ItemType, int32& OutIndex) const
{
	OutIndex = Inventory.Find(ItemType);
	return OutIndex != INDEX_NONE;
}

bool ADealrAIController::TryUseItem()
{
	if (!CachedGameMode || Inventory.Num() == 0) return false;

	int32 ItemIdx = INDEX_NONE;

	// 1. 담배 (HP 회복)
	if (CachedGameMode->GetDealerHP() < CachedGameMode->GetMaxHP() && HasItem(EItemType::Cigarette, ItemIdx))
	{
		if (CachedGameMode->UseCigarette())
		{
			Inventory.RemoveAt(ItemIdx);
			return true;
		}
	}

	// 2. 돋보기 (다음 총알 확인)
	if (!KnowNextShell.IsSet() && HasItem(EItemType::Magnifier, ItemIdx))
	{
		KnowNextShell = CachedGameMode->PeekNextShell();
		Inventory.RemoveAt(ItemIdx);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(FMath::Rand(), 12.f, FColor::Yellow, TEXT("[딜러 AI] 돋보기 사용 - 다음 총알 확인 완료"));
		}
		return true;
	}

	// 3. 톱 (실탄 확정 시 데미지 2배) -> [수정] UseSaw() 직접 호출 추가
	if (KnowNextShell.IsSet() && (*KnowNextShell == EBulletType::Live) && !CachedGameMode->GetIsSawOff())
	{
		if (HasItem(EItemType::Saw, ItemIdx))
		{
			CachedGameMode->UseSaw();
			Inventory.RemoveAt(ItemIdx);
			return true;
		}
	}

	// 4. 수갑 (상대 턴 건너뜀)
	if (!CachedGameMode->GetIsCuff() && HasItem(EItemType::Handcuffs, ItemIdx))
	{
		if (CachedGameMode->UseHandcuffs())
		{
			Inventory.RemoveAt(ItemIdx);
			return true;
		}
	}

	// 5. 맥주 (공포탄 배출)
	if (KnowNextShell.IsSet() && (*KnowNextShell == EBulletType::Blank) && HasItem(EItemType::Beer, ItemIdx))
	{
		CachedGameMode->EjectCurrentShell();
		Inventory.RemoveAt(ItemIdx);
		KnowNextShell.Reset();
		return true;
	}

	// 6. 핸드폰 (랜덤 위치 총알 확인)
	if (CachedGameMode->GetMagazineCount() > 1 && HasItem(EItemType::Phone, ItemIdx))
	{
		int32 FoundIdx = 0;
		EBulletType FoundType;
		if (CachedGameMode->UsePhone(FoundIdx, FoundType))
		{
			Inventory.RemoveAt(ItemIdx);
			if (FoundIdx == 0)
			{
				KnowNextShell = FoundType;
			}
			return true;
		}
	}

	return false; // 사용할 수 있는 아이템 없음
}

void ADealrAIController::ShotDecision()
{
	if (!CachedGameMode || CachedGameMode->IsPlayerTurn) return;

	// 1. 아이템 사용 시도 (아이템을 보유하고 있을 때만 정당하게 사용)
	bool bUsedItem = TryUseItem();

	// 아이템을 사용했다면 1.2초 후 행동 재판단 (아이템 연속 사용 또는 사격)
	if (bUsedItem)
	{
		FTimerHandle ItemTimerHandle;
		GetWorldTimerManager().SetTimer(ItemTimerHandle, [this]()
			{
				this->ShotDecision();
			}, 1.2f, false);
		return;
	}

	// 2. 사격 대상 결정 (더 이상 쓸 아이템이 없을 때 진행)
	ETargetType DecisionTarget = ETargetType::Opponent;
	int32 TotalShells = CachedGameMode->Magazine.Num();

	if (TotalShells == 0) return;

	if (KnowNextShell.IsSet())
	{
		if (*KnowNextShell == EBulletType::Live)
		{
			DecisionTarget = ETargetType::Opponent;
		}
		else
		{
			DecisionTarget = ETargetType::Self; // 공포탄이면 자신 사격
		}
	}
	else
	{
		float LiveProbability = 0.5f;
		if (KnowBlankCount > 0)
		{
			LiveProbability = (float)KnowLiveCount / (float)(KnowLiveCount + KnowBlankCount);
		}

		if (LiveProbability > 0.5f)
		{
			DecisionTarget = ETargetType::Opponent;
		}
		else if (LiveProbability < 0.5f)
		{
			DecisionTarget = ETargetType::Self;
		}
		else
		{
			DecisionTarget = (FMath::RandRange(0, 1) == 0) ? ETargetType::Opponent : ETargetType::Self;
		}
	}

	KnowNextShell.Reset();
	CachedGameMode->ShootTarget(DecisionTarget);
}