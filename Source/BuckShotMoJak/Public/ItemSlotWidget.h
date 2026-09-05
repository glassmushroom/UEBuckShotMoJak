#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuckshotGameMode.h"
#include "ItemSlotWidget.generated.h" // ★ 반드시 마지막 #include

class UImage;
class UTextBlock;
class UButton;
class UTexture2D;

UCLASS()
class BUCKSHOTMOJAK_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* ItemImage;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* QuantityText;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* SlotButton;

public:
	int32 SlotIndex = -1;
	bool bIsPlayerSlot = true;
	EItemType CurrentItemType = EItemType::None;

	void SetSlotData(EItemType InType, UTexture2D* InTexture, int32 InCount);
	void ClearSlot();

private:
	UFUNCTION()
	void OnSlotClicked();
};