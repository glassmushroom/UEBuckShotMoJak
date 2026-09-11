#include "ItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "BuckshotGameMode.h"


void UItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(
			this,
			&UItemSlotWidget::OnSlotClicked
		);

		SlotButton->OnClicked.AddDynamic(
			this,
			&UItemSlotWidget::OnSlotClicked
		);
	}

	ClearSlot();
}

void UItemSlotWidget::SetSlotEnabled(bool bEnabled)
{
	if (SlotButton)
	{
		SlotButton->SetIsEnabled(bEnabled);
	}
}


// ==================================================
// 슬롯 데이터 설정
// ==================================================

void UItemSlotWidget::SetSlotData(EItemType InType,UTexture2D* InTexture,int32 InCount,int32 InSlotIndex,bool bInIsPlayer)
{
	CurrentItemType = InType;
	SlotIndex = InSlotIndex;
	bIsPlayerSlot = bInIsPlayer;

	// ==================================================
	// 완전히 비어 있는 슬롯
	// ==================================================

	if (InType == EItemType::None)
	{
		ClearSlot();
		return;
	}

	// ==================================================
	// 아이템 이미지
	// ==================================================

	if (ItemImage)
	{
		if (InTexture)
		{
			ItemImage->SetBrushFromTexture(InTexture);
			ItemImage->SetVisibility(
				ESlateVisibility::Visible
			);
		}
		else
		{
			ItemImage->SetVisibility(
				ESlateVisibility::Collapsed
			);
		}
	}

	// ==================================================
	// 수량
	// ==================================================

	if (QuantityText)
	{
		QuantityText->SetText(
			FText::AsNumber(
				FMath::Max(0, InCount)
			)
		);

		QuantityText->SetVisibility(
			InCount > 0
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden
		);
	}

	// ==================================================
	// 버튼
	// ==================================================

	if (SlotButton)
	{
		SlotButton->SetVisibility(
			ESlateVisibility::Visible
		);

		SlotButton->SetIsEnabled(
			bInIsPlayer && InCount > 0
		);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[ItemSlot] Slot=%d / Player=%s / Type=%d / Count=%d"
		),
		SlotIndex,
		bIsPlayerSlot ? TEXT("TRUE") : TEXT("FALSE"),
		static_cast<int32>(InType),
		InCount
	);
}


// ==================================================
// 슬롯 초기화
// ==================================================

void UItemSlotWidget::ClearSlot()
{
	CurrentItemType = EItemType::None;


	// -----------------------------------------
	// 이미지
	// -----------------------------------------

	if (ItemImage)
	{
		ItemImage->SetBrushFromTexture(nullptr);

		ItemImage->SetVisibility(
			ESlateVisibility::Collapsed
		);
	}


	// -----------------------------------------
	// 수량
	// -----------------------------------------

	if (QuantityText)
	{
		QuantityText->SetText(
			FText::FromString(TEXT(""))
		);

		QuantityText->SetVisibility(
			ESlateVisibility::Collapsed
		);
	}


	// -----------------------------------------
	// 버튼
	// -----------------------------------------

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(false);
	}
}


// ==================================================
// 슬롯 클릭
// ==================================================

void UItemSlotWidget::OnSlotClicked()
{
	if (CurrentItemType == EItemType::None)
	{
		return;
	}

	ABuckshotGameMode* GM =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GM)
	{
		return;
	}

	// 딜러 턴에는 플레이어 아이템을 사용할 수 없다.
	if (!bIsPlayerSlot || !GM->IsPlayerTurn)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Yellow,
				TEXT("지금은 플레이어 턴이 아닙니다.")
			);
		}

		return;
	}

	const bool bUsed =
		GM->UseItemByType(CurrentItemType, true);

	if (!bUsed && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Red,
			TEXT("아이템을 지금 사용할 수 없습니다.")
		);
	}
}