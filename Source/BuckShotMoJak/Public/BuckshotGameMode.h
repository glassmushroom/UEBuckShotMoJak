#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BuckshotGameMode.generated.h"

class UEndingWidget;
class UHPWidget;
class URoundTransitionWidget;
class UBattleUIWidget;


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
// Item Slot
// ============================================================
//
// 슬롯 하나가
//
// [Beer x2]
// [Saw x1]
// [None]
// ...
//
// 이런 식으로 아이템 종류와 개수를 같이 가지고 있음.
// ============================================================

USTRUCT(BlueprintType)
struct FItemSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType = EItemType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	// 빈 슬롯 생성용
	static FItemSlot None()
	{
		FItemSlot Slot;
		Slot.ItemType = EItemType::None;
		Slot.Quantity = 0;
		return Slot;
	}
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
	// Inventory
	// ========================================================

	// 플레이어 인벤토리
	//
	// 예:
	// [Beer x2]
	// [Saw x1]
	// [None]
	// [None]
	// [None]
	// [None]
	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|Inventory")
	TArray<FItemSlot> PlayerInventory;


	// 딜러 인벤토리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|Inventory")
	TArray<FItemSlot> DealerInventory;


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

	UPROPERTY()
	UEndingWidget* EndingWidgetInstance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buckshot|UI")
	TSubclassOf<UEndingWidget> EndingWidgetClass;

	// 게임 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	TArray<EBulletType> Magazine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool IsPlayerTurn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool IsSawOff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool IsCuff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	bool bIsReloadTransitionPlaying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool bIsEndingPlaying;

	// HP
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuckShot|State")
	int32 CurrentRound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|State")
	int32 PlayerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|State")
	int32 DealerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|State")
	int32 MaxHP;


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
	bool UseItemAtSlot(int32 SlotIndex, bool bIsPlayer);

	UFUNCTION(BlueprintCallable)
	bool UseItemByType(EItemType ItemType, bool bIsPlayer);

	// ========================================================
	// Inventory Initialization
	// ========================================================

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Inventory")
	void InitializePlayerInventory();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Inventory")
	void InitializeDealerInventory();


	// ========================================================
	// Getter
	// ========================================================

	UFUNCTION(BlueprintCallable, Category = "BuckShot|UI")
	UTexture2D* GetItemTexture(EItemType ItemType) const;

	UFUNCTION(BlueprintCallable, Category = "BuckShot|UI")
	int32 GetItemCountInInventory(EItemType ItemType, bool bIsPlayer) const;

	UFUNCTION(BlueprintPure, Category = "BuckShot|Inventory")
	FItemSlot GetPlayerItemSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "BuckShot|Inventory")
	FItemSlot GetDealerItemSlot(int32 SlotIndex) const;

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

	void PlayVictoryEnding();
	void PlayDefeatEnding();
	void AddItemToInventorySlot(EItemType Item, bool bIsPlayer);


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