// Fill out your copyright notice in the Description page of Project Settings.

#include "BuckshotGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Algo/RandomShuffle.h"

ABuckshotGameMode::ABuckshotGameMode()
{
	IsPlayerTurn = false;
	IsSawOff = false;
	IsCuff = false;
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

	LoadMagazine();
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

	return true;
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

