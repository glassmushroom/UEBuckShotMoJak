// Fill out your copyright notice in the Description page of Project Settings.


#include "DealrAIController.h"
#include "BuckshotGameMode.h"
#include "TimerManager.h"


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

	//탄창 카운트 동기화
	for (EBulletType Shell : CachedGameMode->Magazine)
	{
		if (Shell == EBulletType::Live) KnowLiveCount++;
		else KnowBlankCount++;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("[DealerAI] 턴을 받았습니다. 사고 시작..."));
	}

	FTimerHandle AIThinkingHandle;
	GetWorldTimerManager().SetTimer(AIThinkingHandle, this, &ADealrAIController::ShotDecision,1.5f,false);
}

void ADealrAIController::AddItem(EItemType NewItem)
{
	if (Inventory.Num() < 8)
	{
		Inventory.Add(NewItem);
	}
}

void ADealrAIController::ShotDecision()
{
	if (!CachedGameMode || CachedGameMode->IsPlayerTurn) return;

		//HP 판단
		if (CachedGameMode->DealerHP < CachedGameMode->MaxHP)
		{
			CachedGameMode->UseCigarette();
		}

		//사격 대상 결정
		ETargetType DecisionTarget = ETargetType::Opponent;
		int32 TotalShells = CachedGameMode->Magazine.Num();

		if (TotalShells == 0) return;

		if (KnowNextShell.IsSet())
		{
			if (*KnowNextShell == EBulletType::Live)
			{
				DecisionTarget = ETargetType::Opponent;
				CachedGameMode->UseSaw();//실탄 확정이면 톱 사용
			}
			else
			{
				DecisionTarget = ETargetType::Self;//공포탄이면 자신 사격
			}
		}
		else
		{
			float LiveProbability = (float)KnowLiveCount / (float)KnowBlankCount;//확률 계산

			if (LiveProbability > 0.5f)
			{
				DecisionTarget = ETargetType::Opponent;
				if (LiveProbability >= 0.75f) CachedGameMode->UseSaw();
			}
			else if (LiveProbability < 0.5f)
			{
				DecisionTarget = ETargetType::Self; // 공포탄 확률이 더 높아서 자신 쏘기
			}
			else
			{
				DecisionTarget = (FMath::RandRange(0, 1) == 0) ? ETargetType::Opponent : ETargetType::Self;
			}
		}
		KnowNextShell.Reset();
		CachedGameMode->ShootTarget(DecisionTarget);
}

bool ADealrAIController::TryUseItem()
{
	if (!CachedGameMode || Inventory.Num() == 0) return false;

	int32 ItemIdx = INDEX_NONE;

	//아이템
	//담배
	if (CachedGameMode->GetDealerHP() < CachedGameMode->GetMaxHP() && HasItem(EItemType::Cigarette, ItemIdx))
	{
		if (CachedGameMode->UseCigarette())
		{
			Inventory.RemoveAt(ItemIdx);
			return true;
		}
	}
	//돋보기
	if (!KnowNextShell.IsSet() && HasItem(EItemType::Magnifier, ItemIdx))
	{
		KnowNextShell = CachedGameMode->PeekNextShell();
		Inventory.RemoveAt(ItemIdx);
		GEngine->AddOnScreenDebugMessage(-1, -1.f, FColor::Yellow, TEXT("[딜러 AI] 돋보기 사용 - 다음 총알 확인 완료"));
		return true;
	}
	//톱
	if (KnowNextShell.IsSet() && (*KnowNextShell == EBulletType::Live) && !CachedGameMode->GetIsSawOff())
	{
		if (HasItem(EItemType::Saw, ItemIdx))
			Inventory.RemoveAt(ItemIdx);
		    return true;
	}
	//수갑
	if (!CachedGameMode->GetIsCuff() && HasItem(EItemType::Handcuffs, ItemIdx))
	{
		if (CachedGameMode->UseHandcuffs())
		{
			Inventory.RemoveAt(ItemIdx);
			return true;
		}
	}
	//맥주
	if (KnowNextShell.IsSet() && (*KnowNextShell == EBulletType::Blank) && HasItem(EItemType::Beer, ItemIdx))
	{
		int32 FoundIdx = 0;
		CachedGameMode->EjectCurrentShell();
		Inventory.RemoveAt(ItemIdx);
		KnowNextShell.Reset();
		return true;
	}
	// 6. 핸드폰
	if (CachedGameMode->GetMagazineCount() > 1 && HasItem(EItemType::Phone, ItemIdx))
	{
		int32 FoundIdx = 0;
		EBulletType FoundType;
		if (CachedGameMode->UsePhone(FoundIdx, FoundType))
		{
			Inventory.RemoveAt(ItemIdx);
			if (FoundIdx == 0) // 첫 번째 탄 정보였을 경우 기억에 저장
			{
				KnowNextShell = FoundType;
			}
			return true;
		}
	}

	return false; // 사용 가능 아이템 X
}

bool ADealrAIController::HasItem(EItemType ItemType, int32& OutIndex) const
{
	OutIndex = Inventory.Find(ItemType);
	return OutIndex != INDEX_NONE;
}
