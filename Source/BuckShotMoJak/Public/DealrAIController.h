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

	// Dealer 인벤토리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dealer|Item")
	TArray<EItemType> Inventory;

	// 아이템 추가
	void AddItem(EItemType NewItem);

	// 현재 인벤토리 개수
	int32 GetInventoryCount() const
	{
		return Inventory.Num();
	}

	// 인벤토리 비우기
	void ClearInventory()
	{
		Inventory.Empty();
	}

private:

	// 사격 판단
	void ShotDecision();

	// 아이템 사용 판단 및 실행
	bool TryUseItem();

	// 특정 아이템 보유 여부
	bool HasItem(EItemType ItemType, int32& OutIndex) const;

	UPROPERTY()
	ABuckshotGameMode* CachedGameMode;

	// 다음 탄환을 알고 있는 경우
	TOptional<EBulletType> KnowNextShell;

	// AI가 알고 있는 실탄/공포탄 개수
	int32 KnowLiveCount = 0;
	int32 KnowBlankCount = 0;
};