#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndingWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UVerticalBox;

UCLASS()
class BUCKSHOTMOJAK_API UEndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 3라운드 최종 승리 연출
	UFUNCTION(BlueprintNativeEvent, Category = "Ending")
	void PlayVictoryEnding();
	virtual void PlayVictoryEnding_Implementation();

	// 3라운드 최종 패배 연출
	UFUNCTION(BlueprintNativeEvent, Category = "Ending")
	void PlayDefeatEnding();
	virtual void PlayDefeatEnding_Implementation();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	UButton* CreateEndingButton(const TCHAR* Label);
	void UpdateDefeatFade();
	void OpenEndingLevel(FName LevelName);
	UFUNCTION() void Replay();
	UFUNCTION() void ReturnToMenu();

	UPROPERTY(Transient) TObjectPtr<UImage> FadeImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ResultText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ButtonBox;
	UPROPERTY(Transient) TObjectPtr<UButton> ReplayButton;

	FTimerHandle EndingTimerHandle;
	double FadeStartTime = 0.0;
	bool bEndingStarted = false;
	bool bDefeatButtonsReady = false;
	bool bTravelRequested = false;
};
