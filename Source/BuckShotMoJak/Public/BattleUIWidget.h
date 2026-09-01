#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuckshotGameMode.h"
#include "BattleUIWidget.generated.h"

class UButton;
class UHorizontalBox;
class UShellIcon;

UCLASS()
class BUCKSHOTMOJAK_API UBattleUIWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* ShootDealer;

	UPROPERTY(meta = (BindWidget))
	UButton* ShootME;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* ShellContainer;

public:
	//GameMode의 FOnShellsLoaded 시그니처와 맞춤
	UFUNCTION()
	void UpdateShells(const TArray<EBulletType>& Magazine);

	UFUNCTION()
	void OnShotFiredHandler(EBulletType ShellType, ETargetType Target);

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UShellIcon> ShellIconClass;

	UFUNCTION()
	void OnShootOpponentClicked();

	UFUNCTION()
	void OnShootSelfClicked();

	// 순차 생성을 위한 변수 및 타이머
	TArray<EBulletType> CachedMagazine;
	int32 CurrentSpawnIndex = 0;
	FTimerHandle ShellSpawnTimerHandle;
	FTimerHandle ShellClearTimerHandle;

	void SpawnNextShellIcon();
	void ClearAllShellIcons();
};