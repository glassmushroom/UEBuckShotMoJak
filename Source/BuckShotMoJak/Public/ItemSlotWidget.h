#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuckshotGameMode.h"
#include "ItemSlotWidget.generated.h"

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

public:
	UFUNCTION(BlueprintCallable, Category = "ItemSlot")
	void SetSlotEnabled(bool bEnabled);

	// --------------------------------------------------
	// UI 슬롯 번호
	// --------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	int32 SlotIndex = -1;

	// 플레이어 슬롯인지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	bool bIsPlayerSlot = true;


	// --------------------------------------------------
	// 현재 UI 슬롯에 표시되고 있는 아이템
	// --------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Slot")
	EItemType CurrentItemType = EItemType::None;


	// --------------------------------------------------
	// UI 구성요소
	// --------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UImage* ItemImage;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UTextBlock* QuantityText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UButton* SlotButton;


	// --------------------------------------------------
	// 슬롯 데이터 설정
	// --------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "ItemSlot")
	void SetSlotData(
		EItemType InType,
		UTexture2D* InTexture,
		int32 InCount,
		int32 InSlotIndex = -1,
		bool bInIsPlayer = true
	);


	// --------------------------------------------------
	// 슬롯 비우기
	// --------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "ItemSlot")
	void ClearSlot();


	// --------------------------------------------------
	// 버튼 클릭
	// --------------------------------------------------

	UFUNCTION()
	void OnSlotClicked();
};