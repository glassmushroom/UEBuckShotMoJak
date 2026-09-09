#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuckshotGameMode.h"
#include "BattleUIWidget.generated.h"

class UHorizontalBox;
class UButton;
class UShellIcon;
class UItemSlotWidget;

UCLASS()
class BUCKSHOTMOJAK_API UBattleUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ==================================================
	// 버튼 활성화 / 비활성화
	// ==================================================

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetButtonsEnabled(bool bInEnable);


	// ==================================================
	// 아이템 슬롯 갱신
	// ==================================================

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshItemSlots();


	// ==================================================
	// 사격 이벤트
	// ==================================================

	UFUNCTION()
	void OnShotFiredHandler(
		EBulletType ShellType,
		ETargetType Target
	);


	// ==================================================
	// 장전 이벤트
	// ==================================================

	UFUNCTION()
	void OnShellsLoadedHandler(
		const TArray<EBulletType>& Magazine
	);


protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;


	// ==================================================
	// 블루프린트 이벤트
	// ==================================================

	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Buckshot|UI"
	)
	void BP_RefreshItemSlots();


	// ==================================================
	// 아이템 슬롯 위젯 클래스
	// ==================================================

	UPROPERTY(
		EditDefaultsOnly,
		Category = "Inventory"
	)
	TSubclassOf<UItemSlotWidget> ItemSlotClass;


	// ==================================================
	// 플레이어 인벤토리 박스
	// ==================================================

	UPROPERTY(
		meta = (BindWidgetOptional),
		BlueprintReadOnly
	)
	UHorizontalBox* PlayerInventoryBox;


private:

	// ==================================================
	// 재장전 애니메이션
	// ==================================================

	bool bIsReloadingAnimation = false;


	// ==================================================
	// 탄 아이콘 관련
	// ==================================================

	void ClearAllShellIcons();

	void SpawnNextShellIcon();

	void HideShellsAndEnableButtons();

	void CreateItemSlots();
public:

	// ==================================================
	// 버튼
	// ==================================================

	UPROPERTY(
		meta = (BindWidget)
	)
	UButton* ShootDealer;

	UPROPERTY(
		meta = (BindWidget)
	)
	UButton* ShootME;


	// ==================================================
	// 탄 UI
	// ==================================================

	UPROPERTY(
		meta = (BindWidget)
	)
	UHorizontalBox* ShellContainer;


	UPROPERTY(
		EditDefaultsOnly,
		Category = "UI"
	)
	TSubclassOf<UShellIcon> ShellIconClass;


	// ==================================================
	// 아이템 UI 컨테이너
	// ==================================================

	UPROPERTY(
		meta = (BindWidget),
		BlueprintReadWrite,
		Category = "UI"
	)
	UHorizontalBox* ItemContainer;


	// ==================================================
	// ★ 실제로 화면에 생성된 아이템 슬롯들
	//
	// 0 = 돋보기
	// 1 = 맥주
	// 2 = 담배
	// 3 = 톱
	// 4 = 수갑
	// 5 = 핸드폰
	// ==================================================

	UPROPERTY()
	TArray<UItemSlotWidget*> ItemSlots;


private:

	// ==================================================
	// 현재 화면에 표시되어 있는 탄 아이콘
	// ==================================================

	UPROPERTY()
	TArray<UShellIcon*> ActiveShellIcons;


	// ==================================================
	// 장전 연출을 위해 아직 표시하지 않은 탄
	// ==================================================

	TArray<EBulletType> PendingShellsToSpawn;


	// ==================================================
	// 탄 생성 타이머
	// ==================================================

	FTimerHandle ShellSpawnTimerHandle;


	// ==================================================
	// 탄 제거 타이머
	// ==================================================

	FTimerHandle ShellClearTimerHandle;
};