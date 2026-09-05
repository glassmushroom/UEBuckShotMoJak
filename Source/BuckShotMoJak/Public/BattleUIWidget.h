#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuckShotTypes.h"
#include "BattleUIWidget.generated.h"

class UHorizontalBox;
class UButton;
class UShellIcon;

UCLASS()
class BUCKSHOTMOJAK_API UBattleUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 버튼 활성화 / 비활성화
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetButtonsEnabled(bool bInEnable);

	// 아이템 슬롯 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshItemSlots();

	// 사격 이벤트
	UFUNCTION()
	void OnShotFiredHandler(EBulletType ShellType, ETargetType Target);

	// 장전 이벤트
	UFUNCTION()
	void OnShellsLoadedHandler(const TArray<EBulletType>& Magazine);


protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Buckshot|UI")
	void BP_RefreshItemSlots();

private:
	bool bIsReloadingAnimation = false;

	// 탄 아이콘 전체 삭제
	void ClearAllShellIcons();

	// 탄 아이콘 하나 생성
	void SpawnNextShellIcon();

	void HideShellsAndEnableButtons();
public:

	// =========================
	// 버튼
	// =========================

	UPROPERTY(meta = (BindWidget))
	UButton* ShootDealer;

	UPROPERTY(meta = (BindWidget))
	UButton* ShootME;


	// =========================
	// 탄 UI
	// =========================

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* ShellContainer;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UShellIcon> ShellIconClass;

private:

	// 현재 화면에 표시되어 있는 탄 아이콘
	UPROPERTY()
	TArray<UShellIcon*> ActiveShellIcons;

	// 장전 연출을 위해 아직 표시하지 않은 탄
	TArray<EBulletType> PendingShellsToSpawn;

	// 탄 생성 타이머
	FTimerHandle ShellSpawnTimerHandle;

	// 탄 제거 타이머
	FTimerHandle ShellClearTimerHandle;
};