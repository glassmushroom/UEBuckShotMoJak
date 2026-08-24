// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BuckshotGameMode.generated.h"

//총알 종류
UENUM(BlueprintType)
enum class EBulletType : uint8
{
	Live,
	Blank
};

//대상
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
	//카메라
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "BuckShot|Set")
	AActor* MainCameraActor;

	//게임 상태
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Buckshot|State")
	TArray<EBulletType>Magazine;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool IsPlayerTurn;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool IsSawOff;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buckshot|State")
	bool IsCuff;

	//핵심 함수
	UFUNCTION(BlueprintCallable, Category = "BuckShot | Logic")
	void LoadMagazine(int32 MaxShells = 8);
	UFUNCTION(BlueprintCallable, Category = "BuckShot | Logic")
	bool ShootTarget(ETargetType Target);


	UPROPERTY(BlueprintAssignable, Category = "BuckShot | Event")
	FOnShellsLoaded OnShellsLoaded;
	UPROPERTY(BlueprintAssignable, Category = "BuckShot | Event")
	FOnShotFired OnShotFired;

private:
	void SwitchTurn();
};
