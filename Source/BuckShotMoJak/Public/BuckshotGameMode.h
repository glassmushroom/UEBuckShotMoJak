// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BuckshotGameMode.generated.h"

class UHPWidget;
class URoundTransitionWidget;
class ADealrAIController;

// 아이템 종류
UENUM(BlueprintType)
enum class EItemType : uint8
{
	None        UMETA(DisplayName = "None"),
	Magnifier   UMETA(DisplayName = "Magnifier"), // 돋보기
	Beer        UMETA(DisplayName = "Beer"),      // 맥주
	Cigarette   UMETA(DisplayName = "Cigarette"), // 담배
	Saw         UMETA(DisplayName = "Saw"),       // 톱
	Handcuffs   UMETA(DisplayName = "Handcuffs"), // 수갑
	Phone       UMETA(DisplayName = "Phone")      // 핸드폰
};

// 총알 종류
UENUM(BlueprintType)
enum class EBulletType : uint8
{
	Live,
	Blank
};

// 대상
UENUM(BlueprintType)
enum class ETargetType : uint8
{
	Self,
	Opponent
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShellsLoaded, int32, LiveCount, int32, BlankCount);
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
	// 카메라
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuckShot|Set")
	AActor* MainCameraActor;

	// UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buckshot|UI")
	TSubclassOf<UHPWidget> HPWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|UI")
	TSubclassOf<class UUserWidget> BattleUIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buckshot|UI")
	TSubclassOf<URoundTransitionWidget> RoundTransitionWidgetClass;

	// 게임 상태
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

	// HP
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	int32 CurrentRound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|State")
	int32 PlayerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|State")
	int32 DealerHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buckshot|State")
	int32 MaxHP;

	// 라운드 및 게임 로직
	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	void LoadMagazine(int32 MaxShells = 8);

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	bool ShootTarget(ETargetType Target);

	UFUNCTION(BlueprintCallable, Category = "BuckShot|Logic")
	void StartNextRound();

	UFUNCTION(BlueprintCallable, Category = "BuckShot|UI")
	void RefreshHPUI();

	// 라운드 전환 UI 제어
	void PlayRoundTransitionUI(int32 RoundToDisplay);

	UFUNCTION()
	void OnReloadTransitionFinished();

	// 애니메이션 및 이벤트
	UPROPERTY(BlueprintAssignable, Category = "BuckShot|Event")
	FOnShellsLoaded OnShellsLoaded;

	UPROPERTY(BlueprintAssignable, Category = "BuckShot|Event")
	FOnShotFired OnShotFired;

	// 아이템 효과
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

	// AI 참고용 Getter
	int32 GetDealerHP() const { return DealerHP; }
	int32 GetMaxHP() const { return MaxHP; }
	bool GetIsSawOff() const { return IsSawOff; }
	bool GetIsCuff() const { return IsCuff; }
	int32 GetMagazineCount() const { return Magazine.Num(); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	TArray<EItemType> PlayerInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	TArray<EItemType> DealerInventory;

	// 아이템 지급
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
};