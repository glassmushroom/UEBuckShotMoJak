#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BuckshotGameMode.generated.h"

class UHPWidget;
class URoundTransitionWidget;

// Enum 선언부
UENUM(BlueprintType)
enum class EItemType : uint8
{
	None, Magnifier, Beer, Cigarette, Saw, Handcuffs, Phone
};

UENUM(BlueprintType)
enum class EBulletType : uint8
{
	Live, Blank
};

UENUM(BlueprintType)
enum class ETargetType : uint8
{
	Self, Opponent
};

// ★ 1개의 인자(TArray)를 넘기는 dynamic delegate로 변경
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShellsLoaded, const TArray<EBulletType>&, Magazine);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShotFired, EBulletType, ShellType, ETargetType, Target);

UCLASS()
class BUCKSHOTMOJAK_API ABuckshotGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABuckshotGameMode();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|Set")
	AActor* MainCameraActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buckshot|UI")
	TSubclassOf<UHPWidget> HPWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|UI")
	TSubclassOf<class UBattleUIWidget> BattleUIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buckshot|UI")
	TSubclassOf<URoundTransitionWidget> RoundTransitionWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	TArray<EBulletType> Magazine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool IsPlayerTurn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool IsSawOff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool IsCuff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool bIsReloadTransitionPlaying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	int32 CurrentRound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|State")
	int32 PlayerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|State")
	int32 DealerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|State")
	int32 MaxHP;

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	void LoadMagazine(int32 MaxShells = 8);

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	bool ShootTarget(ETargetType Target);

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	void StartNextRound();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|UI")
	void RefreshHPUI();

	void PlayRoundTransitionUI(int32 RoundToDisplay);

	UFUNCTION()
	void OnReloadTransitionFinished();

	UPROPERTY(BlueprintAssignable, Category = "BuckShot|Event")
	FOnShellsLoaded OnShellsLoaded;

	UPROPERTY(BlueprintAssignable, Category = "BuckShot|Event")
	FOnShotFired OnShotFired;

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	EBulletType PeekNextShell();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	EBulletType EjectCurrentShell();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	bool UseCigarette();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	void UseSaw();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	bool UseHandcuffs();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	bool UsePhone(int32& OutIndex, EBulletType& OutType);

	int32 GetDealerHP() const { return DealerHP; }
	int32 GetMaxHP() const { return MaxHP; }
	bool GetIsSawOff() const { return IsSawOff; }
	bool GetIsCuff() const { return IsCuff; }
	int32 GetMagazineCount() const { return Magazine.Num(); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	TArray<EItemType> PlayerInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	TArray<EItemType> DealerInventory;

	UFUNCTION(BlueprintCallable, Category = "Buckshot|Items")
	void DistributeItems(int32 ItemCount);

private:
	void SwitchTurn();
	void ResetCurrentRound();
	void TriggerDealerTurn();
	void DistributeItemsToDealer(int32 ItemCount);
	void HandleMagazineEmpty();

	UPROPERTY()
	UHPWidget* HPWidgetInstance;

	UPROPERTY()
	URoundTransitionWidget* RoundTransitionWidgetInstance;

	FTimerHandle ReloadTransitionFallbackHandle;
	FTimerHandle RoundTimerHandle;
	FTimerHandle RestartTimerHandle;
};