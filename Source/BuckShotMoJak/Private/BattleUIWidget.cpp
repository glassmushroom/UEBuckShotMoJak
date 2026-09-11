#include "BattleUIWidget.h"

#include "BuckshotGameMode.h"
#include "ShellIcon.h"
#include "ItemSlotWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// ======================================================
// NativeConstruct
// ======================================================

void UBattleUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	// --------------------------------------------------
	// 플레이어 아이템 슬롯 생성
	// --------------------------------------------------

	CreateItemSlots();

	// --------------------------------------------------
	// 딜러 아이템 슬롯 생성
	// --------------------------------------------------

	CreateDealerItemSlots();

	// --------------------------------------------------
	// GameMode Delegate 연결
	// --------------------------------------------------

	GameMode->OnShotFired.AddDynamic(
		this,
		&UBattleUIWidget::OnShotFiredHandler
	);

	GameMode->OnShellEjected.AddDynamic(
		this,
		&UBattleUIWidget::OnShellEjectedHandler
	);

	GameMode->OnShellsLoaded.AddDynamic(
		this,
		&UBattleUIWidget::OnShellsLoadedHandler
	);

	GameMode->OnTurnChanged.AddDynamic(
		this,
		&UBattleUIWidget::OnTurnChangedHandler
	);

	// --------------------------------------------------
	// 초기 UI 갱신
	// --------------------------------------------------

	RefreshItemSlots();

	SetButtonsEnabled(
		GameMode->IsPlayerTurn
	);
}


// ======================================================
// NativeDestruct
// ======================================================

void UBattleUIWidget::NativeDestruct()
{
	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (GameMode)
	{
		GameMode->OnShotFired.RemoveDynamic(
			this,
			&UBattleUIWidget::OnShotFiredHandler
		);

		GameMode->OnShellEjected.RemoveDynamic(
			this,
			&UBattleUIWidget::OnShellEjectedHandler
		);

		GameMode->OnShellsLoaded.RemoveDynamic(
			this,
			&UBattleUIWidget::OnShellsLoadedHandler
		);

		GameMode->OnTurnChanged.RemoveDynamic(
			this,
			&UBattleUIWidget::OnTurnChangedHandler
		);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		GetWorld()->GetTimerManager().ClearTimer(
			ShellClearTimerHandle
		);
	}

	ClearAllShellIcons();

	Super::NativeDestruct();
}


// ======================================================
// 버튼 활성 / 비활성
// ======================================================

void UBattleUIWidget::SetButtonsEnabled(
	bool bInEnable
)
{
	if (ShootDealer)
	{
		ShootDealer->SetIsEnabled(
			bInEnable
		);
	}

	if (ShootME)
	{
		ShootME->SetIsEnabled(
			bInEnable
		);
	}
}


// ======================================================
// 플레이어 아이템 슬롯 생성
// ======================================================

void UBattleUIWidget::CreateItemSlots()
{
	if (!ItemContainer)
	{
		return;
	}

	if (!ItemSlotClass)
	{
		return;
	}

	// 이미 만들어져 있으면 중복 생성 방지
	if (ItemSlots.Num() > 0)
	{
		return;
	}

	for (int32 i = 0; i < 6; ++i)
	{
		UItemSlotWidget* NewSlot =
			CreateWidget<UItemSlotWidget>(
				GetWorld(),
				ItemSlotClass
			);

		if (!NewSlot)
		{
			continue;
		}

		// 플레이어 슬롯 설정
		NewSlot->SlotIndex = i;
		NewSlot->bIsPlayerSlot = true;

		ItemSlots.Add(
			NewSlot
		);

		ItemContainer->AddChild(
			NewSlot
		);
	}
}


// ======================================================
// 딜러 아이템 슬롯 생성
// ======================================================

void UBattleUIWidget::CreateDealerItemSlots()
{
	if (!DealerItemContainer)
	{
		return;
	}

	if (!ItemSlotClass)
	{
		return;
	}

	if (DealerItemSlots.Num() > 0)
	{
		return;
	}

	for (int32 i = 0; i < 6; ++i)
	{
		UItemSlotWidget* NewSlot =
			CreateWidget<UItemSlotWidget>(
				GetWorld(),
				ItemSlotClass
			);

		if (!NewSlot)
		{
			continue;
		}

		// 딜러 슬롯 설정
		NewSlot->SlotIndex = i;
		NewSlot->bIsPlayerSlot = false;

		DealerItemSlots.Add(
			NewSlot
		);

		DealerItemContainer->AddChild(
			NewSlot
		);
	}
}


// ======================================================
// 플레이어 아이템 UI 갱신
// ======================================================

void UBattleUIWidget::RefreshItemSlots()
{
	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	if (ItemSlots.Num() != 6)
	{
		CreateItemSlots();
	}

	// --------------------------------------------------
	// 플레이어 슬롯
	// --------------------------------------------------

	for (int32 i = 0; i < ItemSlots.Num(); ++i)
	{
		UItemSlotWidget* SlotWidget =
			ItemSlots[i];

		if (!SlotWidget)
		{
			continue;
		}

		const FItemSlot ItemSlot =
			GameMode->GetPlayerItemSlot(i);

		// --------------------------------------------------
		// ItemSlotWidget.h에 실제로 존재하는 함수 사용
		// --------------------------------------------------

		SlotWidget->SetSlotData(
			ItemSlot.ItemType,
			GameMode->GetItemTexture(
				ItemSlot.ItemType
			),
			ItemSlot.Quantity,
			i,
			true
		);

		// 현재 플레이어 턴이고
		// 아이템 사용 중이 아니며
		// 슬롯에 아이템이 있을 때만 활성화
		const bool bCanUse =
			GameMode->IsPlayerTurn &&
			!GameMode->IsItemUseInProgress() &&
			ItemSlot.ItemType != EItemType::None &&
			ItemSlot.Quantity > 0;

		SlotWidget->SetSlotEnabled(
			bCanUse
		);
	}

	// --------------------------------------------------
	// 딜러 아이템
	// --------------------------------------------------

	RefreshDealerItemSlots();

	// --------------------------------------------------
	// Blueprint 쪽 갱신
	// --------------------------------------------------

	BP_RefreshItemSlots();
}


// ======================================================
// 딜러 아이템 UI 갱신
// ======================================================

void UBattleUIWidget::RefreshDealerItemSlots()
{
	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	if (DealerItemSlots.Num() != 6)
	{
		CreateDealerItemSlots();
	}

	for (int32 i = 0; i < DealerItemSlots.Num(); ++i)
	{
		UItemSlotWidget* SlotWidget =
			DealerItemSlots[i];

		if (!SlotWidget)
		{
			continue;
		}

		const FItemSlot ItemSlot =
			GameMode->GetDealerItemSlot(i);

		// ItemSlotWidget.h의 실제 함수 사용
		SlotWidget->SetSlotData(
			ItemSlot.ItemType,
			GameMode->GetItemTexture(
				ItemSlot.ItemType
			),
			ItemSlot.Quantity,
			i,
			false
		);

		// 딜러 아이템은 직접 클릭하지 않음
		SlotWidget->SetSlotEnabled(
			false
		);
	}
}


// ======================================================
// 사격 이벤트
// ======================================================

void UBattleUIWidget::OnShotFiredHandler(
	EBulletType ShellType,
	ETargetType Target
)
{
	// 사격 반동 애니메이션
	if (Anim_ShotRecoil)
	{
		PlayAnimation(
			Anim_ShotRecoil,
			0.0f,
			1,
			EUMGSequencePlayMode::Forward,
			1.0f
		);
	}
}


// ======================================================
// 탄피 배출 이벤트
// ======================================================

void UBattleUIWidget::OnShellEjectedHandler(
	EBulletType ShellType,
	bool bFromPlayer
)
{
	// --------------------------------------------------
	// 플레이어가 쏨
	// --------------------------------------------------

	if (bFromPlayer)
	{
		if (Anim_EjectToPlayer)
		{
			PlayAnimation(
				Anim_EjectToPlayer,
				0.0f,
				1,
				EUMGSequencePlayMode::Forward,
				1.0f
			);
		}
	}

	// --------------------------------------------------
	// 딜러가 쏨
	// --------------------------------------------------

	else
	{
		if (Anim_EjectToDealer)
		{
			PlayAnimation(
				Anim_EjectToDealer,
				0.0f,
				1,
				EUMGSequencePlayMode::Forward,
				1.0f
			);
		}
	}
}


// ======================================================
// 탄창 장전 이벤트
// ======================================================

void UBattleUIWidget::OnShellsLoadedHandler(
	const TArray<EBulletType>& Magazine
)
{
	// 기존 탄환 아이콘 제거
	ClearAllShellIcons();

	// 재장전 애니메이션 상태
	bIsReloadingAnimation = true;

	// 버튼 잠금
	SetButtonsEnabled(false);

	// 표시할 탄 종류 저장
	PendingShellsToSpawn =
		Magazine;

	// 하나씩 생성
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			ShellSpawnTimerHandle,
			this,
			&UBattleUIWidget::SpawnNextShellIcon,
			0.2f,
			true
		);
	}
}


// ======================================================
// 탄환 하나씩 생성
// ======================================================

void UBattleUIWidget::SpawnNextShellIcon()
{
	if (!GetWorld())
	{
		return;
	}

	if (!ShellContainer)
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		return;
	}

	if (!ShellIconClass)
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		HideShellsAndEnableButtons();

		return;
	}

	// --------------------------------------------------
	// 남은 탄환이 없음
	// --------------------------------------------------

	if (PendingShellsToSpawn.Num() == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		// 탄환을 잠시 보여준 뒤 숨김
		GetWorld()->GetTimerManager().SetTimer(
			ShellClearTimerHandle,
			this,
			&UBattleUIWidget::HideShellsAndEnableButtons,
			2.0f,
			false
		);

		return;
	}

	// --------------------------------------------------
	// 첫 번째 탄환 가져오기
	// --------------------------------------------------

	const EBulletType ShellType =
		PendingShellsToSpawn[0];

	PendingShellsToSpawn.RemoveAt(0);

	// --------------------------------------------------
	// 아이콘 생성
	// --------------------------------------------------

	UShellIcon* NewShellIcon =
		CreateWidget<UShellIcon>(
			GetWorld(),
			ShellIconClass
		);

	if (!NewShellIcon)
	{
		return;
	}

	NewShellIcon->SetShellType(
		ShellType
	);

	ShellContainer->AddChild(
		NewShellIcon
	);

	ActiveShellIcons.Add(
		NewShellIcon
	);
}


// ======================================================
// 탄환 UI 숨기기 + 버튼 활성화
// ======================================================

void UBattleUIWidget::HideShellsAndEnableButtons()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		GetWorld()->GetTimerManager().ClearTimer(
			ShellClearTimerHandle
		);
	}

	ClearAllShellIcons();

	PendingShellsToSpawn.Empty();

	bIsReloadingAnimation = false;

	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	if (
		GameMode->IsPlayerTurn &&
		!GameMode->IsItemUseInProgress()
		)
	{
		SetButtonsEnabled(true);
	}
	else
	{
		SetButtonsEnabled(false);
	}

	RefreshItemSlots();
}


// ======================================================
// 탄환 아이콘 전체 삭제
// ======================================================

void UBattleUIWidget::ClearAllShellIcons()
{
	if (ShellContainer)
	{
		ShellContainer->ClearChildren();
	}

	ActiveShellIcons.Empty();
}


// ======================================================
// 플레이어 턴 변경
// ======================================================

void UBattleUIWidget::OnTurnChangedHandler(
	bool bPlayerTurn
)
{
	RefreshItemSlots();

	// --------------------------------------------------
	// 재장전 연출 중이면 총 방향 애니메이션은 대기
	// --------------------------------------------------

	if (bIsReloadingAnimation)
	{
		SetButtonsEnabled(false);
		return;
	}

	// --------------------------------------------------
	// 플레이어 턴
	// --------------------------------------------------

	if (bPlayerTurn)
	{
		// 총을 플레이어 방향으로 돌림
		if (Anim_TurnShotgun)
		{
			PlayAnimation(
				Anim_TurnShotgun,
				0.0f,
				1,
				EUMGSequencePlayMode::Forward,
				1.0f
			);
		}

		// 플레이어 버튼 활성
		ABuckshotGameMode* GameMode =
			Cast<ABuckshotGameMode>(
				UGameplayStatics::GetGameMode(this)
			);

		if (
			GameMode &&
			!GameMode->IsItemUseInProgress()
			)
		{
			SetButtonsEnabled(true);
		}
		else
		{
			SetButtonsEnabled(false);
		}
	}
	else
	{
		// 딜러 턴
		SetButtonsEnabled(false);
	}
}


// ======================================================
// 외부에서 플레이어 방향 총 애니메이션 실행
// ======================================================

void UBattleUIWidget::PlayTurnShotgunAnimation()
{
	if (!Anim_TurnShotgun)
	{
		return;
	}

	PlayAnimation(
		Anim_TurnShotgun,
		0.0f,
		1,
		EUMGSequencePlayMode::Forward,
		1.0f
	);
}