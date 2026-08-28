// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BuckshotGameMode.h"
#include "DealrAIController.generated.h"

UCLASS()
class BUCKSHOTMOJAK_API ADealrAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ADealrAIController();

	void TakeTurn(ABuckshotGameMode* GameMode);

	//인벤토리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dealer | Item")
	TArray<EItemType>Inventory;
	//아이템 추가
	void AddItem(EItemType NewItem);
	//현재 인벤토리 수 반환
	int32 GetInventoryCount()const { return Inventory.Num(); }

	void ClearInventory() { Inventory.Empty(); }

private:
	void ShotDecision();
	bool TryUseItem(); // AI 아이템 판단&실행
	bool HasItem(EItemType ItemType, int32& OutIndex)const;

	UPROPERTY()
	ABuckshotGameMode* CachedGameMode;

	TOptional<EBulletType>KnowNextShell;
	UPROPERTY()
	int32 KnowLiveCount;
	UPROPERTY()
	int32 KnowBlankCount;
};
