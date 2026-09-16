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
	UFUNCTION(BlueprintCallable, Category = "ShellDisplay")
	void ShowCurrentMagazine();

	// 탄 종류에 따라 이미지를 바꾸고 화면에 띄우는 함수 (C++ 구현)
	UFUNCTION(BlueprintCallable, Category = "ShellDisplay")
	void ShowEjectedShell(EBulletType BulletType);

	// 위젯에 배치할 이미지 컴포넌트 바인딩 (디자이너의 Image 이름과 일치시켜야 함)
	UPROPERTY(meta = (BindWidget))
	class UImage* ShellDisplayImage;

	// 실탄 텍스처
	UPROPERTY(EditDefaultsOnly, Category = "ShellDisplay")
	class UTexture2D* LiveShellTexture;

	// 공포탄 텍스처
	UPROPERTY(EditDefaultsOnly, Category = "ShellDisplay")
	class UTexture2D* BlankShellTexture;

	UPROPERTY(meta = (BindWidget))
	UButton* ShootDealer;

	UPROPERTY(meta = (BindWidget))
	UButton* ShootME;


	// ======================================================
	// 버튼
	// ======================================================

	UFUNCTION(BlueprintCallable)
	void SetButtonsEnabled(bool bInEnable);


	// ======================================================
	// 아이템
	// ======================================================

	UFUNCTION(BlueprintCallable)
	void RefreshItemSlots();

	UFUNCTION(BlueprintCallable)
	void CreateDealerItemSlots();

	UFUNCTION(BlueprintCallable)
	void RefreshDealerItemSlots();


	// ======================================================
	// GameMode Delegate
	// ======================================================

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


	// ======================================================
	// 총 방향
	// ======================================================

	UFUNCTION(BlueprintCallable)
	void PlayTurnShotgunAnimation();

	UFUNCTION(BlueprintCallable)
	float PlayTargetAimAnimation(
		ETargetType Target
	);


protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;


	// ======================================================
	// 탄환 UI
	// ======================================================

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* ShellContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shell")
	TSubclassOf<UShellIcon> ShellIconClass;


	// ======================================================
	// 아이템 UI
	// ======================================================

	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	UHorizontalBox* ItemContainer;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UHorizontalBox* DealerItemContainer;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	UHorizontalBox* PlayerInventoryBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UItemSlotWidget> ItemSlotClass;


	// ======================================================
	// 아이템 슬롯
	// ======================================================

	UPROPERTY()
	TArray<UItemSlotWidget*> ItemSlots;

	UPROPERTY()
	TArray<UItemSlotWidget*> DealerItemSlots;


	// ======================================================
	// UMG Animation
	// ======================================================

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


	// ======================================================
	// Blueprint
	// ======================================================

	UFUNCTION(BlueprintImplementableEvent)
	void BP_RefreshItemSlots();


private:
	FTimerHandle MagnifierDisplayTimerHandle;

	FTimerHandle ShellImageTimerHandle;
	void HideShellImage();

	void ClearAllShellIcons();
	void SpawnNextShellIcon();
	void HideShellsAndEnableButtons();
	void CreateItemSlots();


	// ======================================================
	// 탄환 아이콘
	// ======================================================

	UPROPERTY()
	TArray<UShellIcon*> ActiveShellIcons;

	TArray<EBulletType> PendingShellsToSpawn;

	FTimerHandle ShellSpawnTimerHandle;
	FTimerHandle ShellClearTimerHandle;


	// ======================================================
	// 탄피 애니메이션 진단용
	// ======================================================

	FTimerHandle ShellEjectFinishTimerHandle;


	bool bIsReloadingAnimation = false;

	// UMG 애니메이션 순서 제어용
	FTimerHandle UIAnimationTimerHandle;

	void PlayDelayedUIAnimation();

	float PendingUIAnimationDelay = 1.3f;
};