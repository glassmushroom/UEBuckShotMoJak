// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BuckshotGameMode.generated.h"

// ============================================================
// Forward Declaration
// ============================================================


// ============================================================
// Enum
// ============================================================

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None,
	Magnifier,
	Beer,
	Cigarette,
	Saw,
	Handcuffs,
	Phone
};


UENUM(BlueprintType)
enum class EBulletType : uint8
{
	Live,
	Blank
};


UENUM(BlueprintType)
enum class ETargetType : uint8
{
	Self,
	Opponent
};


// ============================================================
// Delegate
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShellsLoaded,
	const TArray<EBulletType>&,
	Magazine
);


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnShotFired,
	EBulletType,
	ShellType,
	ETargetType,
	Target
);

class UHPWidget;
class URoundTransitionWidget;
class UBattleUIWidget;


// ============================================================
// GameMode
// ============================================================

UCLASS()
class BUCKSHOTMOJAK_API ABuckshotGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ABuckshotGameMode();


protected:

	virtual void BeginPlay() override;


	// ========================================================
	// UI Callback
	// ========================================================

	UFUNCTION()
	void OnShootDealerClicked();

	UFUNCTION()
	void OnShootMeClicked();

	UFUNCTION()
	void OnReloadTransitionFinished();

	UFUNCTION()
	void TriggerDealerTurn();

	UFUNCTION()
	void ResetCurrentRound();


public:

	// ========================================================
	// Camera
	// ========================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|Set")
	AActor* MainCameraActor;


	// ========================================================
	// UI
	// ========================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI")
	TSubclassOf<UHPWidget> HPWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|UI")
	TSubclassOf<UBattleUIWidget> BattleUIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI")
	TSubclassOf<URoundTransitionWidget> RoundTransitionWidgetClass;


	// ========================================================
	// Game State
	// ========================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	TArray<EBulletType> Magazine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool IsPlayerTurn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool IsSawOff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool IsCuff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool bIsReloadTransitionPlaying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	int32 CurrentRound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|State")
	int32 PlayerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|State")
	int32 DealerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|State")
	int32 MaxHP;


	// ========================================================
	// Inventory
	// ========================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|Inventory")
	TArray<EItemType> PlayerInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|Inventory")
	TArray<EItemType> DealerInventory;


	// ========================================================
	// Item Texture
	// ========================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI|ItemTextures")
	UTexture2D* SawTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI|ItemTextures")
	UTexture2D* PhoneTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI|ItemTextures")
	UTexture2D* MagnifierTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI|ItemTextures")
	UTexture2D* BeerTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI|ItemTextures")
	UTexture2D* CigaretteTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuckShot|UI|ItemTextures")
	UTexture2D* HandcuffsTexture;


	// ========================================================
	// Magazine
	// ========================================================

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	void LoadMagazine(int32 MaxShells = 8);

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	bool ShootTarget(ETargetType Target);

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	void StartNextRound();


	// ========================================================
	// UI
	// ========================================================

	UFUNCTION(BlueprintCallable, Category = "BuckShot|UI")
	void RefreshHPUI();

	void PlayRoundTransitionUI(int32 RoundToDisplay);


	// ========================================================
	// Delegate
	// ========================================================

	UPROPERTY(BlueprintAssignable, Category = "BuckShot|Event")
	FOnShellsLoaded OnShellsLoaded;

	UPROPERTY(BlueprintAssignable, Category = "BuckShot|Event")
	FOnShotFired OnShotFired;


	// ========================================================
	// Items
	// ========================================================

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

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	void DistributeItems(int32 ItemCount);

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Items")
	void UseItemAtSlot(int32 SlotIndex, bool bIsPlayer);


	// ========================================================
	// Getter
	// ========================================================

	UTexture2D* GetItemTexture(EItemType ItemType) const;

	int32 GetItemCountInInventory(
		EItemType ItemType,
		bool bIsPlayer
	) const;

	int32 GetDealerHP() const
	{
		return DealerHP;
	}

	int32 GetMaxHP() const
	{
		return MaxHP;
	}

	bool GetIsSawOff() const
	{
		return IsSawOff;
	}

	bool GetIsCuff() const
	{
		return IsCuff;
	}

	int32 GetMagazineCount() const
	{
		return Magazine.Num();
	}


private:

	// ========================================================
	// Internal Logic
	// ========================================================

	void SwitchTurn();

	void DistributeItemsToDealer(int32 ItemCount);

	void HandleMagazineEmpty();

	void AddItemToInventorySlot(
		EItemType Item,
		bool bIsPlayer
	);


	// ========================================================
	// UI Instance
	// ========================================================

	UPROPERTY()
	UHPWidget* HPWidgetInstance;

	UPROPERTY()
	URoundTransitionWidget* RoundTransitionWidgetInstance;

	UPROPERTY()
	UBattleUIWidget* BattleUIWidgetInstance;


	// ========================================================
	// Timer
	// ========================================================

	FTimerHandle ReloadTransitionFallbackHandle;

	FTimerHandle RoundTimerHandle;

	FTimerHandle RestartTimerHandle;
};