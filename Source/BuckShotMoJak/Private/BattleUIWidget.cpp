#include "BattleUIWidget.h"

#include "BuckshotGameMode.h"
#include "ShellIcon.h"
#include "ItemSlotWidget.h"

#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

#include "Kismet/GameplayStatics.h"


// ==================================================
// NativeConstruct
// ==================================================

void UBattleUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABuckshotGameMode* GM =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GM)
	{
		return;
	}

	// 아이템 슬롯 6개 생성
	CreateItemSlots();

	// 사격 이벤트 연결
	GM->OnShotFired.AddDynamic(
		this,
		&UBattleUIWidget::OnShotFiredHandler
	);

	// 장전 이벤트 연결
	GM->OnShellsLoaded.AddDynamic(
		this,
		&UBattleUIWidget::OnShellsLoadedHandler
	);

	RefreshItemSlots();
}


// ==================================================
// NativeDestruct
// ==================================================

void UBattleUIWidget::NativeDestruct()
{
	ClearAllShellIcons();

	ItemSlots.Empty();

	Super::NativeDestruct();
}


// ==================================================
// 버튼 활성화 / 비활성화
// ==================================================

void UBattleUIWidget::SetButtonsEnabled(bool bInEnable)
{
	bool bFinalState = bInEnable;


	// 재장전 연출 중이면 무조건 비활성화
	if (bIsReloadingAnimation && bInEnable)
	{
		bFinalState = false;
	}


	if (ShootDealer)
	{
		ShootDealer->SetIsEnabled(
			bFinalState
		);
	}


	if (ShootME)
	{
		ShootME->SetIsEnabled(
			bFinalState
		);
	}
}


// ==================================================
// 아이템 슬롯 갱신
// ==================================================

void UBattleUIWidget::RefreshItemSlots()
{
	ABuckshotGameMode* GM =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GM)
	{
		return;
	}

	if (ItemSlots.Num() != 6)
	{
		CreateItemSlots();
	}

	for (int32 i = 0; i < ItemSlots.Num(); ++i)
	{
		UItemSlotWidget* SlotWidget = ItemSlots[i];

		if (!SlotWidget)
		{
			continue;
		}

		const FItemSlot ItemSlot =
			GM->GetPlayerItemSlot(i);

		const EItemType ItemType =
			ItemSlot.ItemType;

		const int32 Quantity =
			ItemSlot.Quantity;

		UTexture2D* ItemTexture =
			GM->GetItemTexture(ItemType);

		SlotWidget->SetSlotData(
			ItemType,
			ItemTexture,
			Quantity,
			i,
			true
		);

		SlotWidget->SetSlotEnabled(
			GM->IsPlayerTurn && Quantity > 0
		);
	}
}


// ==================================================
// 사격 이벤트
// ==================================================

void UBattleUIWidget::OnShotFiredHandler(
	EBulletType ShellType,
	ETargetType Target
)
{
	// 현재는 별도의 처리 없음
}


// ==================================================
// 장전 이벤트
// ==================================================

void UBattleUIWidget::OnShellsLoadedHandler(
	const TArray<EBulletType>& Magazine
)
{
	ClearAllShellIcons();


	// 장전 연출 시작
	bIsReloadingAnimation = true;

	SetButtonsEnabled(false);


	PendingShellsToSpawn = Magazine;


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


// ==================================================
// 탄 하나 생성
// ==================================================

void UBattleUIWidget::SpawnNextShellIcon()
{
	if (PendingShellsToSpawn.Num() == 0)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(
				ShellSpawnTimerHandle
			);


			// 탄을 전부 보여준 뒤 2초 대기
			GetWorld()->GetTimerManager().SetTimer(
				ShellClearTimerHandle,
				this,
				&UBattleUIWidget::HideShellsAndEnableButtons,
				2.0f,
				false
			);
		}

		return;
	}


	// 다음 탄
	EBulletType NextType =
		PendingShellsToSpawn[0];


	PendingShellsToSpawn.RemoveAt(0);


	if (ShellContainer && ShellIconClass)
	{
		UShellIcon* NewIcon =
			CreateWidget<UShellIcon>(
				this,
				ShellIconClass
			);


		if (NewIcon)
		{
			NewIcon->SetShellType(
				NextType
			);


			UHorizontalBoxSlot* NewSlot =
				ShellContainer->AddChildToHorizontalBox(
					NewIcon
				);


			if (NewSlot)
			{
				NewSlot->SetPadding(
					FMargin(
						5.0f,
						0.0f,
						5.0f,
						0.0f
					)
				);
			}


			ActiveShellIcons.Add(
				NewIcon
			);
		}
	}
}


// ==================================================
// 탄 숨기기 + 버튼 활성화
// ==================================================

void UBattleUIWidget::HideShellsAndEnableButtons()
{
	ClearAllShellIcons();


	bIsReloadingAnimation = false;


	SetButtonsEnabled(true);
}

void UBattleUIWidget::CreateItemSlots()
{
	if (!ItemContainer)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[BattleUI] ItemContainer가 없습니다.")
		);
		return;
	}

	if (!ItemSlotClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[BattleUI] ItemSlotClass가 설정되지 않았습니다.")
		);
		return;
	}

	ABuckshotGameMode* GM =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GM)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[BattleUI] BuckshotGameMode를 찾을 수 없습니다.")
		);
		return;
	}

	// 기존 UI 슬롯 삭제
	ItemContainer->ClearChildren();
	ItemSlots.Empty();

	// 인벤토리는 항상 6칸
	constexpr int32 InventorySlotCount = 6;

	for (int32 i = 0; i < InventorySlotCount; ++i)
	{
		UItemSlotWidget* SlotWidget =
			CreateWidget<UItemSlotWidget>(
				GetOwningPlayer(),
				ItemSlotClass
			);

		if (!SlotWidget)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[BattleUI] 슬롯 생성 실패 / Index=%d"),
				i
			);
			continue;
		}

		const FItemSlot ItemSlot =
			GM->GetPlayerItemSlot(i);

		SlotWidget->SetSlotData(
			ItemSlot.ItemType,
			GM->GetItemTexture(ItemSlot.ItemType),
			ItemSlot.Quantity,
			i,
			true
		);

		ItemSlots.Add(SlotWidget);

		ItemContainer->AddChild(SlotWidget);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[BattleUI] CreateItemSlots 완료 / 슬롯 수=%d"),
		ItemSlots.Num()
	);
}


// ==================================================
// 탄 아이콘 전체 삭제
// ==================================================

void UBattleUIWidget::ClearAllShellIcons()
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


	if (ShellContainer)
	{
		ShellContainer->ClearChildren();
	}


	ActiveShellIcons.Empty();

	PendingShellsToSpawn.Empty();
}