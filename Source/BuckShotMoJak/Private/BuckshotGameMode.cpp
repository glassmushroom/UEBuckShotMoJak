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
	if(Magazine.Num() == 0)return false;

	EBulletType FiredShell = Magazine.Pop();

	OnShotFired.Broadcast(FiredShell, Target);

	IsSawOff = false;

	if (Target == ETargetType::Self && FiredShell == EBulletType::Blank)
	{

	}
	else
	{
		SwitchTurn();
	}

	if (Magazine.Num() == 0)
	{
		LoadMagazine();
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


