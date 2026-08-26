// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleUIWidget.generated.h"

class UButton;
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

	private:
	UFUNCTION()
	void OnShootOpponentClicked();

	UFUNCTION()
	void OnShootSelfClicked();
};
