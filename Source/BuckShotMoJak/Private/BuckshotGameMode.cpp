// Fill out your copyright notice in the Description page of Project Settings.

#include "BuckshotGameMode.h"
#include "DealrAIController.h"
#include "HPWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Algo/RandomShuffle.h"
#include "RoundTransitionWidget.h"
#include "Blueprint/UserWidget.h"

ABuckshotGameMode::ABuckshotGameMode()
{
	IsPlayerTurn = false;
	IsSawOff = false;
	IsCuff = false;
	bIsReloadTransitionPlaying = false;

	CurrentRound = 0;
	PlayerHP = 0;
	DealerHP = 0;
	MaxHP = 0;

	HPWidgetInstance = nullptr;
	RoundTransitionWidgetInstance = nullptr;
}

void ABuckshotGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}

	if (!MainCameraActor)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainCamera"), FoundActors);
		if (FoundActors.Num() > 0)
		{
			MainCameraActor = FoundActors[0];
		}
	}

	// 플레이어에게 카메라 고정
	if (PC && MainCameraActor)
	{
		PC->SetViewTargetWithBlend(MainCameraActor, 0.0f);
	}

	// HP UI 생성
	if (HPWidgetClass && PC)
	{
		HPWidgetInstance = CreateWidget<UHPWidget>(PC, HPWidgetClass);
		if (HPWidgetInstance)
		{
			HPWidgetInstance->AddToViewport(9999);
		}
	}

	// 라운드 전환 UI 생성 및 이벤트 바인딩
	if (RoundTransitionWidgetClass && PC)
	{
		RoundTransitionWidgetInstance =
			CreateWidget<URoundTransitionWidget>(PC, RoundTransitionWidgetClass);

		if (RoundTransitionWidgetInstance)
		{
			RoundTransitionWidgetInstance->AddToViewport(10000);

			RoundTransitionWidgetInstance->OnTransitionFinished.AddDynamic(
				this,
				&ABuckshotGameMode::OnReloadTransitionFinished
			);
		}
	}

	// 배틀 UI 생성
	if (BattleUIClass && PC)
	{
		UUserWidget* BattleUIInstance = CreateWidget<UUserWidget>(PC, BattleUIClass);
		if (BattleUIInstance)
		{
			BattleUIInstance->AddToViewport();
		}
	}

	// 1라운드 시작
	StartNextRound();
}

void ABuckshotGameMode::RefreshHPUI()
{
	if (HPWidgetInstance)
	{
		HPWidgetInstance->UpdateHPUI(CurrentRound, PlayerHP, DealerHP);
	}
}

void ABuckshotGameMode::HandleMagazineEmpty()
{
	if (bIsReloadTransitionPlaying)
	{
		return;
	}

	if (PlayerHP <= 0 || DealerHP <= 0)
	{
		return;
	}

	PlayRoundTransitionUI(CurrentRound);
}

void ABuckshotGameMode::PlayRoundTransitionUI(int32 RoundToDisplay)
{
	bIsReloadTransitionPlaying = true;

	if (RoundTransitionWidgetInstance)
	{
		RoundTransitionWidgetInstance->PlayReloadTransition(FMath::Clamp(RoundToDisplay, 1, 3));

		GetWorldTimerManager().SetTimer(
			ReloadTransitionFallbackHandle,
			this,
			&ABuckshotGameMode::OnReloadTransitionFinished,
			5.0f,
			false
		);
	}
	else
	{
		OnReloadTransitionFinished();
	}
}

void ABuckshotGameMode::OnReloadTransitionFinished()
{
	if (!bIsReloadTransitionPlaying)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReloadTransitionFallbackHandle);
	bIsReloadTransitionPlaying = false;

	if (Magazine.Num() == 0)
	{
		LoadMagazine(CurrentRound == 1 ? 4 : 8);
	}
}

void ABuckshotGameMode::LoadMagazine(int32 MaxShells)
{
	Magazine.Empty();

	int32 TotalShells = FMath::RandRange(2, MaxShells);
	int32 LiveCount = FMath::RandRange(1, TotalShells - 1);
	int32 BlankCount = TotalShells - LiveCount;

	for (int32 i = 0; i < LiveCount; ++i) Magazine.Add(EBulletType::Live);
	for (int32 i = 0; i < BlankCount; ++i) Magazine.Add(EBulletType::Blank);

	Algo::RandomShuffle(Magazine);

	if (GEngine)
	{
		FString ReloadMsg = FString::Printf(TEXT("[재장전 완료] 총 %d발 (실탄: %d, 공탄: %d)"), TotalShells, LiveCount, BlankCount);
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Emerald, ReloadMsg);
	}

	OnShellsLoaded.Broadcast(LiveCount, BlankCount);
}

void ABuckshotGameMode::TriggerDealerTurn()
{
	TArray<AActor*> FoundPawns;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundPawns);

	for (AActor* Actor : FoundPawns)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (Pawn && !Pawn->IsPlayerControlled())
		{
			ADealrAIController* DealerAI = Cast<ADealrAIController>(Pawn->GetController());
			if (DealerAI)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("[딜러 턴 진행 중...]"));
				}
				DealerAI->TakeTurn(this);
				return;
			}
		}
	}
}


void ABuckshotGameMode::DistributeItems(int32 ItemCount)
{
	const TArray<EItemType> AvailableItems = {
		EItemType::Magnifier,
		EItemType::Beer,
		EItemType::Cigarette,
		EItemType::Saw,
		EItemType::Handcuffs,
		EItemType::Phone
	};

	// 플레이어 아이템 지급 (최대 8개)
	for (int32 i = 0; i < ItemCount; ++i)
	{
		if (PlayerInventory.Num() >= 8) break;

		int32 RandomIndex = FMath::RandRange(0, AvailableItems.Num() - 1);
		PlayerInventory.Add(AvailableItems[RandomIndex]);
	}

	// 딜러 아이템 지급
	DistributeItemsToDealer(ItemCount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("[아이템 지급] 플레이어와 딜러에게 각각 %d개 지급 완료"), ItemCount));
	}
}

void ABuckshotGameMode::DistributeItemsToDealer(int32 ItemCount)
{
	const TArray<EItemType> AvailableItems = {
		EItemType::Magnifier,
		EItemType::Beer,
		EItemType::Cigarette,
		EItemType::Saw,
		EItemType::Handcuffs,
		EItemType::Phone
	};

	// 딜러 아이템 지급 (인벤토리 최대 8개 제한)
	for (int32 i = 0; i < ItemCount; ++i)
	{
		if (DealerInventory.Num() >= 8) break;

		int32 RandomIndex = FMath::RandRange(0, AvailableItems.Num() - 1);
		DealerInventory.Add(AvailableItems[RandomIndex]);
	}
}

void ABuckshotGameMode::ResetCurrentRound()
{
	MaxHP = CurrentRound * 2;
	PlayerHP = MaxHP;
	DealerHP = MaxHP;
	IsPlayerTurn = true;
	IsSawOff = false;
	IsCuff = false;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Orange, FString::Printf(TEXT("Round %d 에서 사망하셨습니다. HP(%d) 원상복구 후 재시작!"), CurrentRound, MaxHP));
	}

	LoadMagazine(CurrentRound == 1 ? 4 : 8);
	RefreshHPUI();
}

bool ABuckshotGameMode::ShootTarget(ETargetType Target)
{
	if (bIsReloadTransitionPlaying)
	{
		return false;
	}

	if (Magazine.Num() == 0) return false;

	EBulletType CurrentShell = Magazine[0];
	Magazine.RemoveAt(0);

	int32 Damage = IsSawOff ? 2 : 1;
	IsSawOff = false;

	FString ShooterStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
	FString TargetStr = (Target == ETargetType::Self) ? TEXT("자신") : TEXT("상대방");
	FString ShellStr = (CurrentShell == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");

	if (GEngine)
	{
		FString ShotLog = FString::Printf(TEXT("[%s]가 [%s]에게 사격! -> 탄 종류: [%s] (데미지: %d)"), *ShooterStr, *TargetStr, *ShellStr, Damage);
		FColor LogColor = (CurrentShell == EBulletType::Live) ? FColor::Red : FColor::Silver;
		GEngine->AddOnScreenDebugMessage(-1, 3.5f, LogColor, ShotLog);
	}

	if (CurrentShell == EBulletType::Live)
	{
		if ((IsPlayerTurn && Target == ETargetType::Opponent) || (!IsPlayerTurn && Target == ETargetType::Self))
		{
			DealerHP = FMath::Max(0, DealerHP - Damage);
		}
		else
		{
			PlayerHP = FMath::Max(0, PlayerHP - Damage);
		}

		RefreshHPUI();

		if (PlayerHP <= 0)
		{
			if (CurrentRound < 3)
			{
				FTimerHandle RestartTimer;
				GetWorldTimerManager().SetTimer(RestartTimer, this, &ABuckshotGameMode::ResetCurrentRound, 2.0f, false);
			}
			else
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GAME OVER - 플레이어 사망!"));
				}
			}
			return true;
		}

		if (DealerHP <= 0)
		{
			if (CurrentRound < 3)
			{
				FTimerHandle RoundTimerHandle;
				GetWorldTimerManager().SetTimer(RoundTimerHandle, this, &ABuckshotGameMode::StartNextRound, 2.0f, false);
			}
			else
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("VICTORY - 딜러를 처단하고 플레이어가 승리했습니다!"));
				}
			}
			return true;
		}

		SwitchTurn();
	}
	else
	{
		if (Target == ETargetType::Opponent)
		{
			SwitchTurn();
		}
		else if (!IsPlayerTurn)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, TEXT("[딜러] 자신에게 공포탄 사격 완료 - 턴 계속 진행"));
			}

			FTimerHandle DealerContinueTimer;
			GetWorldTimerManager().SetTimer(DealerContinueTimer, this, &ABuckshotGameMode::TriggerDealerTurn, 1.0f, false);
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, TEXT("[플레이어] 자신에게 공포탄 사격 완료 - 턴 유지"));
			}
		}
	}

	if (OnShotFired.IsBound())
	{
		OnShotFired.Broadcast(CurrentShell, Target);
	}

	if (Magazine.Num() == 0 && PlayerHP > 0 && DealerHP > 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("탄창이 완전히 비었습니다. 재장전을 진행합니다."));
		}

		HandleMagazineEmpty();
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

	MaxHP = CurrentRound * 2;
	PlayerHP = MaxHP;
	DealerHP = MaxHP;
	IsPlayerTurn = true;
	IsSawOff = false;
	IsCuff = false;

	RefreshHPUI();

	// 라운드 시작 시 전환 UI 연출 실행
	PlayRoundTransitionUI(CurrentRound);

	// 라운드별 아이템 지급 개수 설정 (1라운드: 0개, 2라운드: 2개, 3라운드: 4개)
	int32 ItemsToGive = 0;
	if (CurrentRound == 2)
	{
		ItemsToGive = 2;
	}
	else if (CurrentRound == 3)
	{
		ItemsToGive = 4;
	}

	if (ItemsToGive > 0)
	{
		DistributeItems(ItemsToGive);
	}

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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple, TEXT("[수갑 효과] 상대방의 턴이 건너뛰어졌습니다!"));
		}
	}
	else
	{
		IsPlayerTurn = !IsPlayerTurn;
	}

	if (GEngine)
	{
		FString TurnStr = IsPlayerTurn ? TEXT(">>> [플레이어 턴] <<<") : TEXT(">>> [딜러 턴] <<<");
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Blue, TurnStr);
	}

	if (!IsPlayerTurn)
	{
		FTimerHandle DealerTurnTimer;
		GetWorldTimerManager().SetTimer(DealerTurnTimer, this, &ABuckshotGameMode::TriggerDealerTurn, 1.0f, false);
	}
}

EBulletType ABuckshotGameMode::PeekNextShell()
{
	if (Magazine.Num() > 0)
	{
		return Magazine[0];
	}
	return EBulletType::Blank;
}

EBulletType ABuckshotGameMode::EjectCurrentShell()
{
	if (bIsReloadTransitionPlaying)
	{
		return EBulletType::Blank;
	}

	if (Magazine.Num() > 0)
	{
		EBulletType Ejected = Magazine[0];
		Magazine.RemoveAt(0);

		if (GEngine)
		{
			FString EjectStr = (Ejected == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[맥주 사용] 배출된 총알: %s"), *EjectStr));
		}

		if (Magazine.Num() == 0 && PlayerHP > 0 && DealerHP > 0)
		{
			HandleMagazineEmpty();
		}
		return Ejected;
	}
	return EBulletType::Blank;
}

bool ABuckshotGameMode::UseCigarette()
{
	FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");

	int32& CurrentHP = IsPlayerTurn ? PlayerHP : DealerHP;
	if (CurrentHP < MaxHP)
	{
		CurrentHP++;
		RefreshHPUI();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[%s] 담배 사용 -> HP 1 회복!"), *UserStr));
		}
		return true;
	}
	return false;
}

void ABuckshotGameMode::UseSaw()
{
	IsSawOff = true;
	if (GEngine)
	{
		FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[%s] 톱 사용 -> 다음 총알 데미지 2배!"), *UserStr));
	}
}

bool ABuckshotGameMode::UseHandcuffs()
{
	if (!IsCuff)
	{
		IsCuff = true;
		if (GEngine)
		{
			FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[%s] 수갑 사용 -> 상대 턴 1회 건너뜀"), *UserStr));
		}
		return true;
	}
	return false;
}

bool ABuckshotGameMode::UsePhone(int32& OutIndex, EBulletType& OutType)
{
	if (Magazine.Num() <= 1) return false;

	OutIndex = FMath::RandRange(1, Magazine.Num() - 1);
	OutType = Magazine[OutIndex];

	if (GEngine)
	{
		FString ShellStr = (OutType == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[핸드폰 사용] %d번째 위치의 총알은 [%s]입니다."), OutIndex + 1, *ShellStr));
	}

	return true;
}