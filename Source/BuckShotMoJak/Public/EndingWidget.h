#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndingWidget.generated.h"

UCLASS()
class BUCKSHOTMOJAK_API UEndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 3라운드 최종 승리 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Ending")
	void PlayVictoryEnding();

	// 3라운드 최종 패배 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Ending")
	void PlayDefeatEnding();
};