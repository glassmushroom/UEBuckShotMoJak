#include "ItemSlotWidget.h"
#include "BuckshotGameMode.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"

void UItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UItemSlotWidget::OnSlotClicked);
	}
}

void UItemSlotWidget::SetSlotData(EItemType InType, UTexture2D* InTexture, int32 InCount)
{
	CurrentItemType = InType;

	if (InType == EItemType::None || !InTexture)
	{
		ClearSlot();
		return;
	}

	if (ItemImage)
	{
		ItemImage->SetBrushFromTexture(InTexture);
		ItemImage->SetVisibility(ESlateVisibility::Visible);
	}

	if (QuantityText)
	{
		if (InCount > 1)
		{
			QuantityText->SetText(FText::AsNumber(InCount));
			QuantityText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(bIsPlayerSlot);
	}
}

void UItemSlotWidget::ClearSlot()
{
	CurrentItemType = EItemType::None;

	if (ItemImage)
	{
		ItemImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (QuantityText)
	{
		QuantityText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(false);
	}
}

void UItemSlotWidget::OnSlotClicked()
{
	if (!bIsPlayerSlot || CurrentItemType == EItemType::None) return;

	ABuckshotGameMode* GM = Cast<ABuckshotGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsPlayerTurn)
	{
		GM->UseItemAtSlot(SlotIndex, true);
	}
}