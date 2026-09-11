#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuckshotGameMode.h"
#include "BattleUIWidget.generated.h"

class UHorizontalBox;
class UButton;
class UShellIcon;
class UItemSlotWidget;
class UWidgetAnimation;
class UTexture2D;

UCLASS()
class BUCKSHOTMOJAK_API UBattleUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ==================================================
// 버튼
// ==================================================

	UPROPERTY(meta = (BindWidget))
	UButton* ShootDealer;

	UPROPERTY(meta = (BindWidget))
	UButton* ShootME;


	// ==================================================
	// 기본 UI
	// ==================================================

	UFUNCTION(BlueprintCallable)
	void SetButtonsEnabled(bool bInEnable);

	// ==================================================
	// 아이템 슬롯
	// ==================================================

	UFUNCTION(BlueprintCallable)
	void RefreshItemSlots();

	UFUNCTION(BlueprintCallable)
	void CreateDealerItemSlots();

	UFUNCTION(BlueprintCallable)
	void RefreshDealerItemSlots();

	// ==================================================
	// GameMode Delegate
	// ==================================================

	UFUNCTION()
	void OnShotFiredHandler(
		EBulletType ShellType,
		ETargetType Target
	);

	UFUNCTION()
	void OnShellEjectedHandler(
		EBulletType ShellType,
		bool bFromPlayer
	);

	UFUNCTION()
	void OnShellsLoadedHandler(
		const TArray<EBulletType>& Magazine
	);

	UFUNCTION()
	void OnTurnChangedHandler(
		bool bPlayerTurn
	);

	// ==================================================
	// 애니메이션
	// ==================================================

	UFUNCTION(BlueprintCallable)
	void PlayTurnShotgunAnimation();

protected:

	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

	// ==================================================
	// 탄환 UI
	// ==================================================

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* ShellContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shell")
	TSubclassOf<UShellIcon> ShellIconClass;

	// ==================================================
	// 플레이어 아이템 UI
	// ==================================================

	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	UHorizontalBox* ItemContainer;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UHorizontalBox* DealerItemContainer;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UHorizontalBox* PlayerInventoryBox;

	// ==================================================
	// 아이템 슬롯
	// ==================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UItemSlotWidget> ItemSlotClass;

	UPROPERTY()
	TArray<UItemSlotWidget*> ItemSlots;

	UPROPERTY()
	TArray<UItemSlotWidget*> DealerItemSlots;

	// ==================================================
	// UMG Animation
	// ==================================================

	UPROPERTY(
		Transient,
		meta = (BindWidgetAnim)
	)
	UWidgetAnimation* Anim_TurnShotgun;

	UPROPERTY(
		Transient,
		meta = (BindWidgetAnim)
	)
	UWidgetAnimation* Anim_ShotRecoil;

	UPROPERTY(
		Transient,
		meta = (BindWidgetAnim)
	)
	UWidgetAnimation* Anim_EjectToPlayer;

	UPROPERTY(
		Transient,
		meta = (BindWidgetAnim)
	)
	UWidgetAnimation* Anim_EjectToDealer;

	// ==================================================
	// Blueprint Event
	// ==================================================

	UFUNCTION(BlueprintImplementableEvent)
	void BP_RefreshItemSlots();

private:

	// ==================================================
	// 탄환 UI
	// ==================================================

	void ClearAllShellIcons();

	void SpawnNextShellIcon();

	void HideShellsAndEnableButtons();

	// ==================================================
	// 아이템 슬롯
	// ==================================================

	void CreateItemSlots();

	// ==================================================
	// 탄환 상태
	// ==================================================

	UPROPERTY()
	TArray<UShellIcon*> ActiveShellIcons;

	TArray<EBulletType> PendingShellsToSpawn;

	// ==================================================
	// Timer
	// ==================================================

	FTimerHandle ShellSpawnTimerHandle;

	FTimerHandle ShellClearTimerHandle;

	// ==================================================
	// 상태
	// ==================================================

	bool bIsReloadingAnimation = false;
};