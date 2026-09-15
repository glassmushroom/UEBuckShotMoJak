// Fill out your copyright notice in the Description page of Project Settings.

#include "BuckshotGameMode.h"
#include "EndingWidget.h"
#include "BattleUIWidget.h"
#include "DealrAIController.h"
#include "HPWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Algo/RandomShuffle.h"
#include "RoundTransitionWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

namespace
{
	int32 GetItemCountForRound(int32 Round)
	{
		switch (Round)
		{
		case 1:
			// 1라운드에는 아이템을 지급하지 않는다.
			return 0;

		case 2:
			// 2라운드부터 지급
			return 2;

		case 3:
			return 4;

		default:
			return 0;
		}
	}
}

ABuckshotGameMode::ABuckshotGameMode()
{
	IsPlayerTurn = false;
	IsSawOff = false;
	IsCuff = false;

	bIsReloadTransitionPlaying = false;
	bIsEndingPlaying = false;

	CurrentRound = 0;

	PlayerHP = 0;
	DealerHP = 0;
	MaxHP = 0;

	HPWidgetInstance = nullptr;
	RoundTransitionWidgetInstance = nullptr;
	EndingWidgetInstance = nullptr;
	BattleUIWidgetInstance = nullptr;
	bIsPlayerShotPending = false;
	PendingPlayerShotTarget = ETargetType::Opponent;
	bPendingActionIsPlayer = true;
}

void ABuckshotGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC =
		UGameplayStatics::GetPlayerController(this, 0);

	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}

	// --------------------------------------------------
	// Main Camera
	// --------------------------------------------------

	if (!MainCameraActor)
	{
		TArray<AActor*> FoundActors;

		UGameplayStatics::GetAllActorsWithTag(
			GetWorld(),
			FName("MainCamera"),
			FoundActors
		);

		if (FoundActors.Num() > 0)
		{
			MainCameraActor = FoundActors[0];
		}
	}

	if (PC && MainCameraActor)
	{
		PC->SetViewTargetWithBlend(
			MainCameraActor,
			0.0f
		);
	}

	// --------------------------------------------------
	// HP UI
	// --------------------------------------------------

	if (HPWidgetClass && PC)
	{
		HPWidgetInstance =
			CreateWidget<UHPWidget>(
				PC,
				HPWidgetClass
			);

		if (HPWidgetInstance)
		{
			HPWidgetInstance->AddToViewport(9999);
		}
	}

	// --------------------------------------------------
	// Round Transition UI
	// --------------------------------------------------

	if (RoundTransitionWidgetClass && PC)
	{
		RoundTransitionWidgetInstance =
			CreateWidget<URoundTransitionWidget>(
				PC,
				RoundTransitionWidgetClass
			);

		if (RoundTransitionWidgetInstance)
		{
			RoundTransitionWidgetInstance->AddToViewport(10000);

			RoundTransitionWidgetInstance->OnTransitionFinished.AddDynamic(
				this,
				&ABuckshotGameMode::OnReloadTransitionFinished
			);
		}
	}

	// --------------------------------------------------
	// Ending UI
	// --------------------------------------------------

	if (EndingWidgetClass && PC)
	{
		EndingWidgetInstance =
			CreateWidget<UEndingWidget>(
				PC,
				EndingWidgetClass
			);

		if (EndingWidgetInstance)
		{
			EndingWidgetInstance->AddToViewport(11000);
		}
	}

	// --------------------------------------------------
	// Battle UI
	// --------------------------------------------------

	if (BattleUIClass && PC)
	{
		BattleUIWidgetInstance =
			CreateWidget<UBattleUIWidget>(
				PC,
				BattleUIClass
			);

		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->AddToViewport();

			if (BattleUIWidgetInstance->ShootDealer)
			{
				BattleUIWidgetInstance->ShootDealer->OnClicked.AddDynamic(
					this,
					&ABuckshotGameMode::OnShootDealerClicked
				);
			}

			if (BattleUIWidgetInstance->ShootME)
			{
				BattleUIWidgetInstance->ShootME->OnClicked.AddDynamic(
					this,
					&ABuckshotGameMode::OnShootMeClicked
				);
			}

			BattleUIWidgetInstance->RefreshItemSlots();
		}
	}

	// --------------------------------------------------
	// 게임 시작
	// --------------------------------------------------

	StartNextRound();
}

// ======================================================
// HP UI
// ======================================================

void ABuckshotGameMode::RefreshHPUI()
{
	if (HPWidgetInstance)
	{
		HPWidgetInstance->UpdateHPUI(
			CurrentRound,
			PlayerHP,
			DealerHP
		);
	}
}

// ======================================================
// 인벤토리 초기화
// ======================================================

void ABuckshotGameMode::InitializePlayerInventory()
{
	PlayerInventory.SetNum(6);

	PlayerInventory[0].ItemType = EItemType::Magnifier;
	PlayerInventory[1].ItemType = EItemType::Beer;
	PlayerInventory[2].ItemType = EItemType::Cigarette;
	PlayerInventory[3].ItemType = EItemType::Saw;
	PlayerInventory[4].ItemType = EItemType::Handcuffs;
	PlayerInventory[5].ItemType = EItemType::Phone;

	for (FItemSlot& Slot : PlayerInventory)
	{
		Slot.Quantity = 0;
	}
}

void ABuckshotGameMode::InitializeDealerInventory()
{
	DealerInventory.SetNum(6);

	DealerInventory[0].ItemType = EItemType::Magnifier;
	DealerInventory[1].ItemType = EItemType::Beer;
	DealerInventory[2].ItemType = EItemType::Cigarette;
	DealerInventory[3].ItemType = EItemType::Saw;
	DealerInventory[4].ItemType = EItemType::Handcuffs;
	DealerInventory[5].ItemType = EItemType::Phone;

	for (FItemSlot& Slot : DealerInventory)
	{
		Slot.Quantity = 0;
	}
}

// ======================================================
// 특정 아이템 지급
// ======================================================

void ABuckshotGameMode::AddItemToInventorySlot(
	EItemType Item,
	bool bIsPlayer
)
{
	TArray<FItemSlot>& Inventory =
		bIsPlayer
		? PlayerInventory
		: DealerInventory;

	for (FItemSlot& Slot : Inventory)
	{
		if (Slot.ItemType == Item)
		{
			Slot.Quantity++;
			return;
		}
	}
}

// ======================================================
// 아이템 랜덤 지급
// ======================================================

void ABuckshotGameMode::DistributeItems(int32 ItemCount)
{
	const TArray<EItemType> AvailableItems =
	{
		EItemType::Magnifier,
		EItemType::Beer,
		EItemType::Cigarette,
		EItemType::Saw,
		EItemType::Handcuffs,
		EItemType::Phone
	};

	for (int32 i = 0; i < ItemCount; ++i)
	{
		const int32 RandomIndex =
			FMath::RandRange(
				0,
				AvailableItems.Num() - 1
			);

		AddItemToInventorySlot(
			AvailableItems[RandomIndex],
			true
		);
	}

	DistributeItemsToDealer(ItemCount);

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->RefreshItemSlots();
	}
}

// ======================================================
// 딜러 아이템 지급
// ======================================================

void ABuckshotGameMode::DistributeItemsToDealer(int32 ItemCount)
{
	const TArray<EItemType> AvailableItems =
	{
		EItemType::Magnifier,
		EItemType::Beer,
		EItemType::Cigarette,
		EItemType::Saw,
		EItemType::Handcuffs,
		EItemType::Phone
	};

	for (int32 i = 0; i < ItemCount; ++i)
	{
		const int32 RandomIndex =
			FMath::RandRange(
				0,
				AvailableItems.Num() - 1
			);

		AddItemToInventorySlot(
			AvailableItems[RandomIndex],
			false
		);
	}
}

// ======================================================
// 아이템 사용 시작
// ======================================================

bool ABuckshotGameMode::UseItemAtSlot(
	int32 SlotIndex,
	bool bIsPlayer
)
{
	if (bIsItemUseInProgress)
	{
		return false;
	}

	TArray<FItemSlot>& Inventory =
		bIsPlayer
		? PlayerInventory
		: DealerInventory;

	if (!Inventory.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FItemSlot& ItemSlot = Inventory[SlotIndex];

	if (
		ItemSlot.ItemType == EItemType::None ||
		ItemSlot.Quantity <= 0
		)
	{
		return false;
	}

	bIsItemUseInProgress = true;

	PendingItemSlotIndex = SlotIndex;
	bPendingItemIsPlayer = bIsPlayer;

	bPendingActionIsPlayer = bIsPlayer;

	if (bIsPlayer && BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->SetButtonsEnabled(false);
	}

	GetWorldTimerManager().SetTimer(
		ItemUseTimerHandle,
		this,
		&ABuckshotGameMode::ExecutePendingItemUse,
		1.5f,
		false
	);

	return true;
}

bool ABuckshotGameMode::UseItemByType(EItemType ItemType, bool bIsPlayer)
{
	TArray<FItemSlot>& Inventory =
		bIsPlayer
		? PlayerInventory
		: DealerInventory;

	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		if (
			Inventory[i].ItemType == ItemType &&
			Inventory[i].Quantity > 0
			)
		{
			return UseItemAtSlot(
				i,
				bIsPlayer
			);
		}
	}

	return false;
}

// ======================================================
// 아이템 텍스처
// ======================================================

UTexture2D* ABuckshotGameMode::GetItemTexture(
	EItemType ItemType
) const
{
	switch (ItemType)
	{
	case EItemType::Saw:
		return SawTexture;

	case EItemType::Phone:
		return PhoneTexture;

	case EItemType::Magnifier:
		return MagnifierTexture;

	case EItemType::Beer:
		return BeerTexture;

	case EItemType::Cigarette:
		return CigaretteTexture;

	case EItemType::Handcuffs:
		return HandcuffsTexture;

	default:
		return nullptr;
	}
}

// ======================================================
// 특정 아이템 전체 수량
// ======================================================

int32 ABuckshotGameMode::GetItemCountInInventory(
	EItemType ItemType,
	bool bIsPlayer
) const
{
	const TArray<FItemSlot>& Inventory =
		bIsPlayer
		? PlayerInventory
		: DealerInventory;

	int32 TotalQuantity = 0;

	for (const FItemSlot& ItemSlot : Inventory)
	{
		if (ItemSlot.ItemType == ItemType)
		{
			TotalQuantity += ItemSlot.Quantity;
		}
	}

	return TotalQuantity;
}

FItemSlot ABuckshotGameMode::GetPlayerItemSlot(
	int32 SlotIndex
) const
{
	if (!PlayerInventory.IsValidIndex(SlotIndex))
	{
		return FItemSlot{};
	}

	return PlayerInventory[SlotIndex];
}

FItemSlot ABuckshotGameMode::GetDealerItemSlot(
	int32 SlotIndex
) const
{
	if (!DealerInventory.IsValidIndex(SlotIndex))
	{
		return FItemSlot{};
	}

	return DealerInventory[SlotIndex];
}

// ======================================================
// 탄창 비었을 때
// ======================================================

void ABuckshotGameMode::HandleMagazineEmpty()
{
	if (
		bIsReloadTransitionPlaying ||
		PlayerHP <= 0 ||
		DealerHP <= 0
		)
	{
		return;
	}

	bIsReloadTransitionPlaying = true;

	const int32 ReloadItemCount =
		GetItemCountForRound(CurrentRound);

	if (ReloadItemCount > 0)
	{
		DistributeItems(ReloadItemCount);
	}

	if (RoundTransitionWidgetInstance)
	{
		GetWorldTimerManager().SetTimer(
			ReloadTransitionFallbackHandle,
			this,
			&ABuckshotGameMode::OnReloadTransitionFinished,
			1.5f,
			false
		);
	}
	else
	{
		OnReloadTransitionFinished();
	}
}

// ======================================================
// 승리
// ======================================================

void ABuckshotGameMode::PlayVictoryEnding()
{
	if (bIsEndingPlaying)
	{
		return;
	}

	bIsEndingPlaying = true;

	if (EndingWidgetInstance)
	{
		EndingWidgetInstance->PlayVictoryEnding();
	}
}

// ======================================================
// 패배
// ======================================================

void ABuckshotGameMode::PlayDefeatEnding()
{
	if (bIsEndingPlaying)
	{
		return;
	}

	bIsEndingPlaying = true;

	if (EndingWidgetInstance)
	{
		EndingWidgetInstance->PlayDefeatEnding();
	}
}

// ======================================================
// 라운드 전환 UI
// ======================================================

void ABuckshotGameMode::PlayRoundTransitionUI(
	int32 RoundToDisplay
)
{
	bIsReloadTransitionPlaying = true;

	if (RoundTransitionWidgetInstance)
	{
		RoundTransitionWidgetInstance->PlayReloadTransition(
			FMath::Clamp(
				RoundToDisplay,
				1,
				3
			)
		);

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

// ======================================================
// 재장전 완료
// ======================================================

void ABuckshotGameMode::OnReloadTransitionFinished()
{
	GetWorldTimerManager().ClearTimer(
		ReloadTransitionFallbackHandle
	);

	bIsReloadTransitionPlaying = false;

	LoadMagazine(
		CurrentRound == 1
		? 4
		: 8
	);

	if (IsPlayerTurn)
	{
		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->RefreshItemSlots();
			OnTurnChanged.Broadcast(IsPlayerTurn);
		}
	}
	else
	{
		FTimerHandle DealerTurnTimer;

		GetWorldTimerManager().SetTimer(
			DealerTurnTimer,
			this,
			&ABuckshotGameMode::TriggerDealerTurn,
			1.0f,
			false
		);
	}
}

// ======================================================
// 탄창 생성
// ======================================================

void ABuckshotGameMode::LoadMagazine(int32 MaxShells)
{
	Magazine.Empty();

	const int32 TotalShells =
		FMath::RandRange(
			2,
			MaxShells
		);

	const int32 LiveCount =
		FMath::RandRange(
			1,
			TotalShells - 1
		);

	const int32 BlankCount =
		TotalShells - LiveCount;

	for (int32 i = 0; i < LiveCount; ++i)
	{
		Magazine.Add(
			EBulletType::Live
		);
	}

	for (int32 i = 0; i < BlankCount; ++i)
	{
		Magazine.Add(
			EBulletType::Blank
		);
	}

	TArray<EBulletType> DisplayMagazine = Magazine;

	if (OnShellsLoaded.IsBound())
	{
		OnShellsLoaded.Broadcast(
			DisplayMagazine
		);
	}

	Algo::RandomShuffle(Magazine);
}

void ABuckshotGameMode::ReceiveDealerDecision(ETargetType Target)
{
	if (IsPlayerTurn)
	{
		return;
	}

	if (bIsReloadTransitionPlaying ||
		bIsEndingPlaying ||
		bIsItemUseInProgress)
	{
		return;
	}

	PendingDealerDecisionTarget = Target;
	bIsDealerDecisionPending = true;

	OnDealerDecisionReceived.Broadcast(Target);

	ProcessDealerDecision();
}

// ======================================================
// 딜러 턴
// ======================================================

void ABuckshotGameMode::TriggerDealerTurn()
{
	if (
		bIsEndingPlaying ||
		bIsReloadTransitionPlaying ||
		IsPlayerTurn ||
		bIsDealerAimPending
		)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundPawns;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		APawn::StaticClass(),
		FoundPawns
	);

	for (AActor* Actor : FoundPawns)
	{
		APawn* Pawn = Cast<APawn>(Actor);

		if (
			Pawn &&
			!Pawn->IsPlayerControlled()
			)
		{
			ADealrAIController* DealerAI =
				Cast<ADealrAIController>(
					Pawn->GetController()
				);

			if (!DealerAI)
			{
				continue;
			}

			bIsDealerAimPending = true;

			if (BattleUIWidgetInstance)
			{
				BattleUIWidgetInstance->SetButtonsEnabled(false);

				const float AimDuration =
					BattleUIWidgetInstance->PlayTargetAimAnimation(
						ETargetType::Opponent
					);

				FTimerDelegate DealerAimDelegate;

				DealerAimDelegate.BindLambda(
					[this, DealerAI]()
					{
						bIsDealerAimPending = false;

						if (
							!IsValid(DealerAI) ||
							bIsEndingPlaying ||
							bIsReloadTransitionPlaying ||
							IsPlayerTurn
							)
						{
							return;
						}

						DealerAI->TakeTurn(this);
					}
				);

				GetWorld()->GetTimerManager().SetTimer(
					DealerAimTimerHandle,
					DealerAimDelegate,
					FMath::Max(AimDuration, 0.01f),
					false
				);
			}
			else
			{
				bIsDealerAimPending = false;
				DealerAI->TakeTurn(this);
			}

			return;
		}
	}
}

// ======================================================
// 사격
// ======================================================

bool ABuckshotGameMode::ShootTarget(
	ETargetType Target
)
{
	if (bIsEndingPlaying)
	{
		return false;
	}

	if (bIsReloadTransitionPlaying)
	{
		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->SetButtonsEnabled(false);
		}

		return false;
	}

	if (Magazine.Num() == 0)
	{
		return false;
	}

	const bool bShooterIsPlayer =
		IsPlayerTurn;

	const EBulletType CurrentShell =
		Magazine[0];

	Magazine.RemoveAt(0);

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->ShowEjectedShell(CurrentShell);
	}

	const int32 Damage =
		IsSawOff
		? 2
		: 1;

	IsSawOff = false;

	OnShotFired.Broadcast(
		CurrentShell,
		Target
	);

	OnShellEjected.Broadcast(
		CurrentShell,
		bShooterIsPlayer
	);

	if (CurrentShell == EBulletType::Live)
	{
		if (
			(
				IsPlayerTurn &&
				Target == ETargetType::Opponent
				)
			||
			(
				!IsPlayerTurn &&
				Target == ETargetType::Self
				)
			)
		{
			DealerHP =
				FMath::Max(
					0,
					DealerHP - Damage
				);
		}
		else
		{
			PlayerHP =
				FMath::Max(
					0,
					PlayerHP - Damage
				);
		}

		RefreshHPUI();

		if (PlayerHP <= 0)
		{
			if (CurrentRound == 3)
			{
				PlayDefeatEnding();
			}
			else
			{
				GetWorldTimerManager().SetTimer(
					RestartTimerHandle,
					this,
					&ABuckshotGameMode::ResetCurrentRound,
					2.0f,
					false
				);
			}

			return true;
		}

		if (DealerHP <= 0)
		{
			if (CurrentRound == 3)
			{
				PlayVictoryEnding();
			}
			else
			{
				GetWorldTimerManager().SetTimer(
					RoundTimerHandle,
					this,
					&ABuckshotGameMode::StartNextRound,
					2.0f,
					false
				);
			}

			return true;
		}
	}

	// 탄창이 비었는지 확인
	if (Magazine.Num() == 0)
	{
		HandleMagazineEmpty();
		return true;
	}

	// 턴 전환 여부 판단 수정
	// - 실탄이면 무조건 턴 종료
	// - 상대방을 쏘았으면(실탄이든 공포탄이든) 턴 종료
	const bool bShouldSwitchTurn =
		(CurrentShell == EBulletType::Live) ||
		(Target == ETargetType::Opponent);

	if (bShouldSwitchTurn)
	{
		SwitchTurn();
	}
	else
	{
		// 턴이 유지되는 경우 (예: 플레이어가 자신에게 공포탄을 쏜 경우)
		if (bShooterIsPlayer)
		{
			// 플레이어 턴 유지이므로 버튼을 다시 활성화하여 연속 행동 가능하게 함
			if (BattleUIWidgetInstance)
			{
				BattleUIWidgetInstance->SetButtonsEnabled(true);
			}
		}
		else
		{
			// 딜러 턴 유지인 경우 AI 턴 재개
			FTimerHandle DealerContinueTimer;
			GetWorldTimerManager().SetTimer(
				DealerContinueTimer,
				this,
				&ABuckshotGameMode::TriggerDealerTurn,
				1.0f,
				false
			);
		}
	}

	return true;
}

// ======================================================
// 다음 라운드
// ======================================================

void ABuckshotGameMode::StartNextRound()
{
	GetWorldTimerManager().ClearTimer(
		RoundTimerHandle
	);

	GetWorldTimerManager().ClearTimer(
		RestartTimerHandle
	);

	GetWorldTimerManager().ClearTimer(
		ReloadTransitionFallbackHandle
	);

	++CurrentRound;

	if (CurrentRound > 3)
	{
		return;
	}

	MaxHP =
		CurrentRound * 2;

	PlayerHP =
		MaxHP;

	DealerHP =
		MaxHP;

	IsPlayerTurn = true;
	bIsPlayerShotPending = false;
	GetWorldTimerManager().ClearTimer(PlayerShotTimerHandle);

	IsSawOff = false;
	IsCuff = false;

	Magazine.Empty();

	InitializePlayerInventory();
	InitializeDealerInventory();

	const int32 ItemCount =
		GetItemCountForRound(CurrentRound);

	if (ItemCount > 0)
	{
		DistributeItems(ItemCount);
	}

	RefreshHPUI();

	OnTurnChanged.Broadcast(
		IsPlayerTurn
	);

	PlayRoundTransitionUI(
		CurrentRound
	);
}

// ======================================================
// 턴 변경 및 결정 처리
// ======================================================

void ABuckshotGameMode::ProcessDealerDecision()
{
	if (!bIsDealerDecisionPending)
	{
		return;
	}

	if (IsPlayerTurn ||
		bIsReloadTransitionPlaying ||
		bIsEndingPlaying ||
		bIsItemUseInProgress)
	{
		bIsDealerDecisionPending = false;
		return;
	}

	const ETargetType DecisionTarget =
		PendingDealerDecisionTarget;

	bIsDealerDecisionPending = false;

	ShootTarget(DecisionTarget);
}

void ABuckshotGameMode::SwitchTurn()
{
	if (IsCuff)
	{
		IsCuff = false;
	}
	else
	{
		IsPlayerTurn =
			!IsPlayerTurn;

		OnTurnChanged.Broadcast(
			IsPlayerTurn
		);
	}

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->SetButtonsEnabled(false);
		BattleUIWidgetInstance->RefreshItemSlots();
	}

	if (!IsPlayerTurn)
	{
		bIsPlayerShotPending = false;

		GetWorldTimerManager().ClearTimer(
			PlayerShotTimerHandle
		);

		FTimerHandle DealerTurnTimer;

		GetWorldTimerManager().SetTimer(
			DealerTurnTimer,
			this,
			&ABuckshotGameMode::TriggerDealerTurn,
			1.0f,
			false
		);
	}
}

// ======================================================
// 현재 라운드 리셋
// ======================================================

void ABuckshotGameMode::ResetCurrentRound()
{
	GetWorldTimerManager().ClearTimer(
		RestartTimerHandle
	);

	Magazine.Empty();

	IsPlayerTurn = true;
	bIsPlayerShotPending = false;
	GetWorldTimerManager().ClearTimer(PlayerShotTimerHandle);

	IsSawOff = false;
	IsCuff = false;

	MaxHP =
		CurrentRound * 2;

	PlayerHP =
		MaxHP;

	DealerHP =
		MaxHP;

	InitializePlayerInventory();
	InitializeDealerInventory();

	const int32 ItemCount =
		GetItemCountForRound(CurrentRound);

	if (ItemCount > 0)
	{
		DistributeItems(ItemCount);
	}

	RefreshHPUI();

	OnTurnChanged.Broadcast(
		IsPlayerTurn
	);

	PlayRoundTransitionUI(
		CurrentRound
	);
}

// ======================================================
// 아이템 사용 실행
// ======================================================

void ABuckshotGameMode::ExecutePendingItemUse()
{


	const int32 SlotIndex = PendingItemSlotIndex;
	const bool bIsPlayer = bPendingItemIsPlayer;

	bIsItemUseInProgress = false;
	PendingItemSlotIndex = INDEX_NONE;

	TArray<FItemSlot>& Inventory = bIsPlayer ? PlayerInventory : DealerInventory;

	if (!Inventory.IsValidIndex(SlotIndex))
	{
		return;
	}

	FItemSlot& UsedSlot = Inventory[SlotIndex];

	if (UsedSlot.ItemType == EItemType::None || UsedSlot.Quantity <= 0)
	{
		return;
	}

	const EItemType UsedItem = UsedSlot.ItemType;
	bool bSuccess = false;

	// 로그용 이름 정의 (EItemType 값에 정확히 매칭)
	FString ItemNameText = TEXT("");
	switch (UsedItem)
	{
	case EItemType::Magnifier: ItemNameText = TEXT("돋보기"); break;
	case EItemType::Beer: ItemNameText = TEXT("맥주"); break;
	case EItemType::Cigarette: ItemNameText = TEXT("담배"); break;
	case EItemType::Saw: ItemNameText = TEXT("톱"); break;
	case EItemType::Handcuffs: ItemNameText = TEXT("수갑"); break;
	case EItemType::Phone: ItemNameText = TEXT("핸드폰"); break;
	default: ItemNameText = TEXT("알 수 없는 아이템"); break;
	}

	switch (UsedItem)
	{
	case EItemType::Beer:
		EjectCurrentShell();
		bSuccess = true;
		break;

	case EItemType::Cigarette:
		bSuccess = UseCigarette();
		break;

	case EItemType::Saw:
		UseSaw();
		bSuccess = true;
		break;

	case EItemType::Handcuffs:
		bSuccess = UseHandcuffs();
		break;

	case EItemType::Magnifier:
		if (Magazine.Num() > 0)
		{
			PeekNextShell();
			bSuccess = true;
		}
		break;

	case EItemType::Phone:
	{
		int32 RevealedIndex = INDEX_NONE;
		EBulletType RevealedShell = EBulletType::Blank;
		bSuccess = UsePhone(RevealedIndex, RevealedShell);
		break;
	}

	default:
		break;
	}

	if (bSuccess)
	{
		UsedSlot.Quantity = FMath::Max(0, UsedSlot.Quantity - 1);

		// ★ 아이템 사용 성공 통합 로그 출력
		if (GEngine)
		{
			const FString UserStr = bIsPlayer ? TEXT("플레이어") : TEXT("딜러");
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Orange,
				FString::Printf(TEXT("[%s]가 [%s] 아이템을 사용했습니다."), *UserStr, *ItemNameText)
			);
		}
	}

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->RefreshItemSlots();

		if (bIsPlayer && IsPlayerTurn && !bIsReloadTransitionPlaying)
		{
			BattleUIWidgetInstance->SetButtonsEnabled(true);
		}
	}
}

// ======================================================
// 다음 탄 확인 (돋보기)
// ======================================================
EBulletType ABuckshotGameMode::PeekNextShell()
{
	if (Magazine.Num() == 0)
	{
		return EBulletType::Blank;
	}

	const EBulletType NextShell = Magazine[0];

	if (GEngine)
	{
		const TCHAR* ShellText = (NextShell == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[돋보기 효과] 다음 탄은 [%s]입니다."), ShellText)
		);
	}

	return NextShell;
}

// ======================================================
// 현재 탄 배출 (맥주)
// ======================================================
EBulletType ABuckshotGameMode::EjectCurrentShell()
{
	if (bIsReloadTransitionPlaying)
	{
		return EBulletType::Blank;
	}

	if (Magazine.Num() > 0)
	{
		const EBulletType Ejected = Magazine[0];
		Magazine.RemoveAt(0);

		// C++에서 위젯 함수 직접 호출
		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->ShowEjectedShell(Ejected);
		}

		// 기존 멀티캐스트 브로드캐스트 (필요 시 유지)
		OnShellEjected.Broadcast(Ejected, IsPlayerTurn);

		if (Magazine.Num() == 0 && PlayerHP > 0 && DealerHP > 0)
		{
			HandleMagazineEmpty();
		}

		return Ejected;
	}

	return EBulletType::Blank;
}

// ======================================================
// 담배
// ======================================================
bool ABuckshotGameMode::UseCigarette()
{
	int32& CurrentHP = IsPlayerTurn ? PlayerHP : DealerHP;

	if (CurrentHP < MaxHP)
	{
		CurrentHP++;
		RefreshHPUI();

		if (GEngine)
		{
			const FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Green,
				FString::Printf(TEXT("[%s] HP가 1 회복되었습니다. (현재 HP: %d)"), *UserStr, CurrentHP)
			);
		}
		return true;
	}

	return false;
}

// ======================================================
// 톱
// ======================================================
void ABuckshotGameMode::UseSaw()
{
	IsSawOff = true;

	if (GEngine)
	{
		const FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Red,
			FString::Printf(TEXT("[%s] 톱 사용! 다음 총격 데미지가 2배로 증가합니다."), *UserStr)
		);
	}
}

// ======================================================
// 수갑
// ======================================================
bool ABuckshotGameMode::UseHandcuffs()
{
	if (!IsCuff)
	{
		IsCuff = true;

		if (GEngine)
		{
			const FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Purple,
				FString::Printf(TEXT("[%s] 수갑 사용! 상대방의 다음 턴이 건너뛰어집니다."), *UserStr)
			);
		}
		return true;
	}

	return false;
}

// ======================================================
// 핸드폰
// ======================================================
bool ABuckshotGameMode::UsePhone(int32& OutIndex, EBulletType& OutType)
{
	if (Magazine.Num() <= 1)
	{
		return false;
	}

	OutIndex = FMath::RandRange(1, Magazine.Num() - 1);
	OutType = Magazine[OutIndex];

	if (GEngine)
	{
		const FString ShellStr = (OutType == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[핸드폰 효과] %d번째 탄알은 [%s]입니다."), OutIndex + 1, *ShellStr)
		);
	}

	return true;
}

void ABuckshotGameMode::StartPlayerShot(ETargetType Target)
{
	if (
		!IsPlayerTurn ||
		bIsReloadTransitionPlaying ||
		bIsEndingPlaying ||
		bIsItemUseInProgress ||
		bIsPlayerShotPending
		)
	{
		return;
	}

	if (Magazine.Num() == 0)
	{
		return;
	}

	bIsPlayerShotPending = true;
	PendingPlayerShotTarget = Target;

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->SetButtonsEnabled(false);

		const float AimDuration =
			BattleUIWidgetInstance->PlayTargetAimAnimation(Target);

		GetWorldTimerManager().SetTimer(
			PlayerShotTimerHandle,
			this,
			&ABuckshotGameMode::ExecutePendingPlayerShot,
			FMath::Max(AimDuration, 0.01f),
			false
		);
	}
	else
	{
		ExecutePendingPlayerShot();
	}
}

void ABuckshotGameMode::ExecutePendingPlayerShot()
{
	GetWorldTimerManager().ClearTimer(
		PlayerShotTimerHandle
	);

	if (!bIsPlayerShotPending)
	{
		return;
	}

	if (!IsPlayerTurn ||
		bIsReloadTransitionPlaying ||
		bIsEndingPlaying ||
		bIsItemUseInProgress)
	{
		bIsPlayerShotPending = false;
		return;
	}

	const ETargetType Target =
		PendingPlayerShotTarget;

	bIsPlayerShotPending = false;

	ShootTarget(Target);
}

// ======================================================
// 사격 버튼
// ======================================================

void ABuckshotGameMode::OnShootDealerClicked()
{
	if (!IsPlayerTurn || bIsReloadTransitionPlaying || bIsEndingPlaying || bIsItemUseInProgress)
	{
		return;
	}

	StartPlayerShot(ETargetType::Opponent);
}

void ABuckshotGameMode::OnShootMeClicked()
{
	if (!IsPlayerTurn || bIsReloadTransitionPlaying || bIsEndingPlaying || bIsItemUseInProgress)
	{
		return;
	}

	StartPlayerShot(ETargetType::Self);
}