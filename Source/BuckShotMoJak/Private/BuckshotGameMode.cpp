// Fill out your copyright notice in the Description page of Project Settings.

#include "BuckshotGameMode.h"
#include "HPWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Algo/RandomShuffle.h"

ABuckshotGameMode::ABuckshotGameMode()
{
	IsPlayerTurn = false;
	IsSawOff = false;
	IsCuff = false;
	CurrentRound = 0;
	PlayerHP = 0;
	DealerHP = 0;
	MaxHP = 0;
	HPWidgetInstance = nullptr;
}

void ABuckshotGameMode::BeginPlay() 
{
	Super::BeginPlay();

	if (!MainCameraActor)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainCamera"), FoundActors);
		if (FoundActors.Num() > 0)
		{
			MainCameraActor = FoundActors[0];
		}
	}
	//플레이어에게 카메라 고정
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && MainCameraActor)
	{
		PC->SetViewTargetWithBlend(MainCameraActor, 0.0f);
	}

	if (HPWidgetClass && PC)
	{
		HPWidgetInstance = CreateWidget<UHPWidget>(PC, HPWidgetClass);
		if (HPWidgetInstance)
		{
			HPWidgetInstance->AddToViewport(9999);
		}
	}
	//1라 시작
	StartNextRound();
}

void ABuckshotGameMode::RefreshHPUI()
{
	if (HPWidgetInstance)
	{
		HPWidgetInstance->UpdateHPUI(CurrentRound, PlayerHP, DealerHP);
	}
}

void ABuckshotGameMode::LoadMagazine(int32 MaxShells)
{
	Magazine.Empty();

	int32 TotalShells = FMath::RandRange(2, MaxShells);
	int32 LiveCount = FMath::RandRange(1, TotalShells - 1);
	int32 BlankCount = TotalShells - LiveCount;

	for (int32 i = 0; i < LiveCount; ++i)Magazine.Add(EBulletType::Live);
	for (int32 i = 0; i < BlankCount; ++i)Magazine.Add(EBulletType::Blank);

	Algo::RandomShuffle(Magazine);

	OnShellsLoaded.Broadcast(LiveCount, BlankCount);
}

bool ABuckshotGameMode::ShootTarget(ETargetType Target)
{
	if (Magazine.Num() == 0) return false;

	EBulletType CurrentShell = Magazine[0];
	Magazine.RemoveAt(0);

	int32 Damage = IsSawOff ? 2 : 1;
	IsSawOff = false; // 사격 후 톱 효과 해제

	if (CurrentShell == EBulletType::Live)
	{
		// 실탄 사격
		if ((IsPlayerTurn && Target == ETargetType::Opponent) || (!IsPlayerTurn && Target == ETargetType::Self))
		{
			DealerHP = FMath::Max(0, DealerHP - Damage);
		}
		else
		{
			PlayerHP = FMath::Max(0, PlayerHP - Damage);
		}

		RefreshHPUI();
		// 1. 플레이어 HP가 0이 되면 라운드와 상관없이 즉시 패배 (딜러 승리)
		if (PlayerHP <= 0)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GAME OVER - 딜러 승리!"));
			}
			return true;
		}

		// 2. 딜러 HP가 0이 되었을 때
		if (DealerHP <= 0)
		{
			if (CurrentRound == 3)
			{
				// 3라운드에서 딜러 HP 0 -> 플레이어 최종 승리
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("VICTORY - 플레이어 최종 승리!"));
				}
			}
			else
			{
				// 1~2라운드 -> 2초 뒤 다음 라운드로 진행
				FTimerHandle RoundTimerHandle;
				GetWorldTimerManager().SetTimer(RoundTimerHandle, this, &ABuckshotGameMode::StartNextRound, 2.0f, false);
			}
			return true;
		}

		SwitchTurn();
	}
	else
	{
		// 공포탄 사격
		if (Target == ETargetType::Opponent)
		{
			SwitchTurn();
		}
	}

	if (OnShotFired.IsBound())
	{
		OnShotFired.Broadcast(CurrentShell, Target);
	}

	// 탄창이 비었으면 재장전
	if (Magazine.Num() == 0 && PlayerHP > 0 && DealerHP > 0)
	{
		LoadMagazine(CurrentRound == 1 ? 4 : 8);
	}
	return true;
}
void ABuckshotGameMode::StartNextRound()
{
	CurrentRound++;

	if (CurrentRound > 3)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("모든 라운드 클리어!"));
		}
		return;
	}
	//라운드별 HP계산
	MaxHP = CurrentRound * 2;
	PlayerHP = MaxHP;
	DealerHP = MaxHP;
	IsPlayerTurn = true;

	//재장전
	LoadMagazine(CurrentRound == 1 ? 4 : 8);
	RefreshHPUI();

	if (GEngine)
	{
		FString RoundMsg = FString::Printf(TEXT("=== ROUND %d START ==="), CurrentRound);
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, RoundMsg);
	}
}
void ABuckshotGameMode::SwitchTurn()
{
	if (IsCuff)
	{
		IsCuff = false;
		return;
	}

	IsPlayerTurn = !IsPlayerTurn;
}
//돋보기
EBulletType ABuckshotGameMode::PeekNextShell()
{
	if (Magazine.Num() > 0)
	{
		return Magazine[0];
	}
	return EBulletType::Blank;
}
//맥주
EBulletType ABuckshotGameMode::EjectCurrentShell()
{
	if (Magazine.Num() > 0)
	{
		EBulletType Ejected = Magazine[0];
		Magazine.RemoveAt(0);
		// 마지막 총알 배출했으면 재장전
		if (Magazine.Num() == 0 && PlayerHP > 0 && DealerHP > 0)
		{
			LoadMagazine(CurrentRound == 1 ? 4 : 8);
		}
		return Ejected;
	}
	return EBulletType::Blank;
}
//담배
bool ABuckshotGameMode::UseCigarette()
{
	int32& CurrentHP = IsPlayerTurn ? PlayerHP : DealerHP;
	if (CurrentHP < MaxHP)
	{
		CurrentHP++;
		RefreshHPUI();//회복하면 UI갱신
		return true;
	}
	return false; // HP최대치면 사용X
}
//톱
void ABuckshotGameMode::UseSaw()
{
	IsSawOff = true;
}
//수갑
bool ABuckshotGameMode::UseHandcuffs()
{
	if (!IsCuff)
	{
		IsCuff = true;
		return true;
	}
	return false; // 이미 수갑 상태면 사용 불가
}

// 핸드폰
bool ABuckshotGameMode::UsePhone(int32& OutIndex, EBulletType& OutType)
{
	if (Magazine.Num() <= 1) return false; // 1발 이하면 효과X

	// 무작위 위치 선택
	OutIndex = FMath::RandRange(1, Magazine.Num() - 1);
	OutType = Magazine[OutIndex];
	return true;
}

