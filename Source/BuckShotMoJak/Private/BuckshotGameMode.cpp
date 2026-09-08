#include "BuckshotGameMode.h"
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
		case 2:
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

	CurrentRound = 0;
	PlayerHP = 0;
	DealerHP = 0;
	MaxHP = 0;

	HPWidgetInstance = nullptr;
	RoundTransitionWidgetInstance = nullptr;
	BattleUIWidgetInstance = nullptr;

	// --------------------------------------------------
	// 인벤토리는 항상 6칸 고정
	// 아이템 종류도 슬롯 위치에 따라 고정
	// --------------------------------------------------

	PlayerInventory.SetNum(6);
	DealerInventory.SetNum(6);

	InitializePlayerInventory();
	InitializeDealerInventory();
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
		PC->SetViewTargetWithBlend(MainCameraActor, 0.0f);
	}

	// --------------------------------------------------
	// HP UI
	// --------------------------------------------------

	if (HPWidgetClass && PC)
	{
		HPWidgetInstance = CreateWidget<UHPWidget>(
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

			// 게임 시작 시 인벤토리 UI 갱신
			BattleUIWidgetInstance->RefreshItemSlots();
		}
	}

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

	// 슬롯 종류는 항상 고정
	PlayerInventory[0].ItemType = EItemType::Magnifier;
	PlayerInventory[1].ItemType = EItemType::Beer;
	PlayerInventory[2].ItemType = EItemType::Cigarette;
	PlayerInventory[3].ItemType = EItemType::Saw;
	PlayerInventory[4].ItemType = EItemType::Handcuffs;
	PlayerInventory[5].ItemType = EItemType::Phone;

	// 처음에는 수량 0
	for (FItemSlot& Slot : PlayerInventory)
	{
		Slot.Quantity = 0;
	}
}

void ABuckshotGameMode::InitializeDealerInventory()
{
	DealerInventory.SetNum(6);

	// 슬롯 종류는 항상 고정
	DealerInventory[0].ItemType = EItemType::Magnifier;
	DealerInventory[1].ItemType = EItemType::Beer;
	DealerInventory[2].ItemType = EItemType::Cigarette;
	DealerInventory[3].ItemType = EItemType::Saw;
	DealerInventory[4].ItemType = EItemType::Handcuffs;
	DealerInventory[5].ItemType = EItemType::Phone;

	// 처음에는 수량 0
	for (FItemSlot& Slot : DealerInventory)
	{
		Slot.Quantity = 0;
	}
}

// ======================================================
// 특정 아이템을 직접 지급
//
// 랜덤 슬롯에 아이템을 넣는 것이 아니라
// 해당 아이템의 고정 슬롯 Quantity만 증가
// ======================================================

void ABuckshotGameMode::AddItemToInventorySlot(
	EItemType Item,
	bool bIsPlayer
)
{
	TArray<FItemSlot>& Inventory =
		bIsPlayer ? PlayerInventory : DealerInventory;

	for (FItemSlot& Slot : Inventory)
	{
		if (Slot.ItemType == Item)
		{
			Slot.Quantity++;
			return;
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Red,
			TEXT("[아이템 지급 실패] 존재하지 않는 아이템 타입")
		);
	}
}

// ======================================================
// 아이템 랜덤 지급
//
// 이제 이 함수는 "랜덤 슬롯에 넣기"가 아니라
// 랜덤으로 아이템 종류를 선택해서
// 그 아이템의 고정 슬롯 Quantity를 증가시킴
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
			FMath::RandRange(0, AvailableItems.Num() - 1);

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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.f,
			FColor::Green,
			FString::Printf(
				TEXT("[아이템 지급] 플레이어와 딜러에게 각각 %d개 지급 완료"),
				ItemCount
			)
		);
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
			FMath::RandRange(0, AvailableItems.Num() - 1);

		AddItemToInventorySlot(
			AvailableItems[RandomIndex],
			false
		);
	}
}

// ======================================================
// 아이템 사용
// ======================================================

bool ABuckshotGameMode::UseItemAtSlot(int32 SlotIndex,bool bIsPlayer)
{
	TArray<FItemSlot>& Inventory =
		bIsPlayer ? PlayerInventory : DealerInventory;

	if (!Inventory.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FItemSlot& UsedSlot = Inventory[SlotIndex];

	if (UsedSlot.ItemType == EItemType::None ||
		UsedSlot.Quantity <= 0)
	{
		return false;
	}

	const EItemType UsedItem = UsedSlot.ItemType;
	bool bSuccess = false;

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
		PeekNextShell();
		bSuccess = true;
		break;

	case EItemType::Phone:
	{
		int32 RevealedIndex = INDEX_NONE;
		EBulletType RevealedShell = EBulletType::Blank;
		bSuccess = UsePhone(RevealedIndex, RevealedShell);
		break;
	}

	default:
		return false;
	}

	if (!bSuccess)
	{
		return false;
	}

	--UsedSlot.Quantity;

	if (UsedSlot.Quantity <= 0)
	{
		UsedSlot.Quantity = 0;
	}

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->RefreshItemSlots();
	}

	return true;
}

bool ABuckshotGameMode::UseItemByType(EItemType ItemType, bool bIsPlayer)
{
	TArray<FItemSlot>& Inventory =
		bIsPlayer ? PlayerInventory : DealerInventory;

	for (int32 Index = 0; Index < Inventory.Num(); ++Index)
	{
		const FItemSlot& ItemSlot = Inventory[Index];

		if (ItemSlot.ItemType == ItemType &&
			ItemSlot.Quantity > 0)
		{
			return UseItemAtSlot(Index, bIsPlayer);
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

int32 ABuckshotGameMode::GetItemCountInInventory(EItemType ItemType,bool bIsPlayer) const
{
	const TArray<FItemSlot>& Inventory =
		bIsPlayer ? PlayerInventory : DealerInventory;

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

FItemSlot ABuckshotGameMode::GetPlayerItemSlot(int32 SlotIndex) const
{
	if (!PlayerInventory.IsValidIndex(SlotIndex))
	{
		return FItemSlot{};
	}

	return PlayerInventory[SlotIndex];
}

FItemSlot ABuckshotGameMode::GetDealerItemSlot(int32 SlotIndex) const
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
	if (bIsReloadTransitionPlaying ||
		PlayerHP <= 0 ||
		DealerHP <= 0)
	{
		return;
	}

	bIsReloadTransitionPlaying = true;

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
			FMath::Clamp(RoundToDisplay, 1, 3)
		);

		GetWorldTimerManager().SetTimer(
			ReloadTransitionFallbackHandle,
			this,
			&ABuckshotGameMode::OnReloadTransitionFinished,
			2.5f,
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
		CurrentRound == 1 ? 4 : 8
	);

	if (IsPlayerTurn)
	{
		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->SetButtonsEnabled(true);
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
		FMath::RandRange(2, MaxShells);

	const int32 LiveCount =
		FMath::RandRange(1, TotalShells - 1);

	const int32 BlankCount =
		TotalShells - LiveCount;

	for (int32 i = 0; i < LiveCount; ++i)
	{
		Magazine.Add(EBulletType::Live);
	}

	for (int32 i = 0; i < BlankCount; ++i)
	{
		Magazine.Add(EBulletType::Blank);
	}

	TArray<EBulletType> DisplayMagazine = Magazine;

	if (OnShellsLoaded.IsBound())
	{
		OnShellsLoaded.Broadcast(DisplayMagazine);
	}

	Algo::RandomShuffle(Magazine);

	if (GEngine)
	{
		const FString ReloadMsg =
			FString::Printf(
				TEXT("[재장전 완료] 총 %d발 (실탄: %d, 공포탄: %d)"),
				TotalShells,
				LiveCount,
				BlankCount
			);

		GEngine->AddOnScreenDebugMessage(
			-1,
			4.f,
			FColor::Emerald,
			ReloadMsg
		);
	}
}

// ======================================================
// 딜러 턴
// ======================================================

void ABuckshotGameMode::TriggerDealerTurn()
{
	TArray<AActor*> FoundPawns;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		APawn::StaticClass(),
		FoundPawns
	);

	for (AActor* Actor : FoundPawns)
	{
		APawn* Pawn = Cast<APawn>(Actor);

		if (Pawn && !Pawn->IsPlayerControlled())
		{
			ADealrAIController* DealerAI =
				Cast<ADealrAIController>(
					Pawn->GetController()
				);

			if (DealerAI)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						2.f,
						FColor::Yellow,
						TEXT("[딜러 턴 진행 중...]")
					);
				}

				DealerAI->TakeTurn(this);
				return;
			}
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
	if (bIsReloadTransitionPlaying ||
		Magazine.Num() == 0)
	{
		return false;
	}

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->SetButtonsEnabled(false);
	}

	EBulletType CurrentShell = Magazine[0];

	Magazine.RemoveAt(0);

	const int32 Damage =
		IsSawOff ? 2 : 1;

	IsSawOff = false;

	const FString ShooterStr =
		IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");

	const FString TargetStr =
		(Target == ETargetType::Self)
		? TEXT("자신")
		: TEXT("상대방");

	const FString ShellStr =
		(CurrentShell == EBulletType::Live)
		? TEXT("실탄")
		: TEXT("공포탄");

	if (GEngine)
	{
		const FString ShotLog =
			FString::Printf(
				TEXT("[%s]가 [%s]에게 사격! -> 탄 종류: [%s] (데미지: %d)"),
				*ShooterStr,
				*TargetStr,
				*ShellStr,
				Damage
			);

		const FColor LogColor =
			(CurrentShell == EBulletType::Live)
			? FColor::Red
			: FColor::Silver;

		GEngine->AddOnScreenDebugMessage(
			-1,
			3.5f,
			LogColor,
			ShotLog
		);
	}

	if (OnShotFired.IsBound())
	{
		OnShotFired.Broadcast(
			CurrentShell,
			Target
		);
	}

	if (CurrentShell == EBulletType::Live)
	{
		if (
			(IsPlayerTurn &&
				Target == ETargetType::Opponent)
			||
			(!IsPlayerTurn &&
				Target == ETargetType::Self)
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
			GetWorldTimerManager().SetTimer(
				RestartTimerHandle,
				this,
				&ABuckshotGameMode::ResetCurrentRound,
				2.0f,
				false
			);

			return true;
		}

		if (DealerHP <= 0)
		{
			GetWorldTimerManager().SetTimer(
				RoundTimerHandle,
				this,
				&ABuckshotGameMode::StartNextRound,
				2.0f,
				false
			);

			return true;
		}
	}

	if (Magazine.Num() == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Yellow,
				TEXT("탄창이 완전히 비었습니다. 재장전을 진행합니다.")
			);
		}

		HandleMagazineEmpty();

		return true;
	}

	if (CurrentShell == EBulletType::Live)
	{
		SwitchTurn();
	}
	else
	{
		if (Target == ETargetType::Opponent)
		{
			SwitchTurn();
		}
		else
		{
			if (IsPlayerTurn)
			{
				if (BattleUIWidgetInstance)
				{
					BattleUIWidgetInstance->SetButtonsEnabled(true);
				}
			}
			else
			{
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

	// ==================================================
	// HP 초기화
	// ==================================================

	MaxHP = CurrentRound * 2;

	PlayerHP = MaxHP;
	DealerHP = MaxHP;

	// ==================================================
	// 상태 초기화
	// ==================================================

	IsPlayerTurn = true;
	IsSawOff = false;
	IsCuff = false;

	// ==================================================
	// 인벤토리 초기화
	// 슬롯 위치는 유지하고 수량만 0
	// ==================================================

	InitializePlayerInventory();
	InitializeDealerInventory();

	const int32 ItemCount = GetItemCountForRound(CurrentRound);

	if (ItemCount > 0)
	{
		DistributeItems(ItemCount);
	}

	RefreshHPUI();

	// ==================================================
	// 라운드 시작 연출
	// ==================================================

	PlayRoundTransitionUI(CurrentRound);
}

// ======================================================
// 턴 변경
// ======================================================

void ABuckshotGameMode::SwitchTurn()
{
	if (IsCuff)
	{
		IsCuff = false;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Purple,
				TEXT("[수갑 효과] 상대방의 턴이 건너뛰어졌습니다!")
			);
		}
	}
	else
	{
		IsPlayerTurn = !IsPlayerTurn;
	}

	if (GEngine)
	{
		const FString TurnStr =
			IsPlayerTurn
			? TEXT(">>> [플레이어 턴] <<<")
			: TEXT(">>> [딜러 턴] <<<");

		GEngine->AddOnScreenDebugMessage(
			-1,
			2.5f,
			FColor::Blue,
			TurnStr
		);
	}

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->SetButtonsEnabled(IsPlayerTurn);
		BattleUIWidgetInstance->RefreshItemSlots();
	}

	if (!IsPlayerTurn)
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
// 현재 라운드 리셋
// ======================================================

void ABuckshotGameMode::ResetCurrentRound()
{
	GetWorldTimerManager().ClearTimer(
		RestartTimerHandle
	);

	Magazine.Empty();

	IsPlayerTurn = true;
	IsSawOff = false;
	IsCuff = false;

	MaxHP = CurrentRound * 2;

	PlayerHP = MaxHP;
	DealerHP = MaxHP;

	// 슬롯 구조 유지 + 수량 초기화
	InitializePlayerInventory();
	InitializeDealerInventory();

	const int32 ItemCount = GetItemCountForRound(CurrentRound);

	if (ItemCount > 0)
	{
		DistributeItems(ItemCount);
	}

	RefreshHPUI();

	PlayRoundTransitionUI(CurrentRound);
}

// ======================================================
// 다음 탄 확인
// ======================================================

EBulletType ABuckshotGameMode::PeekNextShell()
{
	if (Magazine.Num() > 0)
	{
		return Magazine[0];
	}

	return EBulletType::Blank;
}

// ======================================================
// 현재 탄 배출
// ======================================================

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

		if (OnShotFired.IsBound())
		{
			OnShotFired.Broadcast(
				Ejected,
				ETargetType::Self
			);
		}

		if (GEngine)
		{
			const FString EjectStr =
				(Ejected == EBulletType::Live)
				? TEXT("실탄")
				: TEXT("공포탄");

			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Orange,
				FString::Printf(
					TEXT("[맥주 사용] 배출된 총알: %s"),
					*EjectStr
				)
			);
		}

		if (
			Magazine.Num() == 0 &&
			PlayerHP > 0 &&
			DealerHP > 0
			)
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
	const FString UserStr =
		IsPlayerTurn
		? TEXT("플레이어")
		: TEXT("딜러");

	int32& CurrentHP =
		IsPlayerTurn
		? PlayerHP
		: DealerHP;

	if (CurrentHP < MaxHP)
	{
		CurrentHP++;

		RefreshHPUI();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Orange,
				FString::Printf(
					TEXT("[%s] 담배 사용 -> HP 1 회복!"),
					*UserStr
				)
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
		const FString UserStr =
			IsPlayerTurn
			? TEXT("플레이어")
			: TEXT("딜러");

		GEngine->AddOnScreenDebugMessage(
			-1,
			3.f,
			FColor::Orange,
			FString::Printf(
				TEXT("[%s] 톱 사용 -> 다음 총알 데미지 2배!"),
				*UserStr
			)
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
			const FString UserStr =
				IsPlayerTurn
				? TEXT("플레이어")
				: TEXT("딜러");

			GEngine->AddOnScreenDebugMessage(
				-1,
				3.f,
				FColor::Orange,
				FString::Printf(
					TEXT("[%s] 수갑 사용 -> 상대 턴 1회 건너뜀"),
					*UserStr
				)
			);
		}

		return true;
	}

	return false;
}

// ======================================================
// 핸드폰
// ======================================================

bool ABuckshotGameMode::UsePhone(
	int32& OutIndex,
	EBulletType& OutType
)
{
	if (Magazine.Num() <= 1)
	{
		return false;
	}

	OutIndex =
		FMath::RandRange(
			1,
			Magazine.Num() - 1
		);

	OutType = Magazine[OutIndex];

	if (GEngine)
	{
		const FString ShellStr =
			(OutType == EBulletType::Live)
			? TEXT("실탄")
			: TEXT("공포탄");

		GEngine->AddOnScreenDebugMessage(
			-1,
			3.f,
			FColor::Orange,
			FString::Printf(
				TEXT("[핸드폰 사용] %d번째 위치의 총알은 [%s]입니다."),
				OutIndex + 1,
				*ShellStr
			)
		);
	}

	return true;
}

// ======================================================
// 사격 버튼
// ======================================================

void ABuckshotGameMode::OnShootDealerClicked()
{
	if (IsPlayerTurn)
	{
		ShootTarget(
			ETargetType::Opponent
		);
	}
}

void ABuckshotGameMode::OnShootMeClicked()
{
	if (IsPlayerTurn)
	{
		ShootTarget(
			ETargetType::Self
		);
	}
}