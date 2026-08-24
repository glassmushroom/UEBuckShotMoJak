// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleUI.generated.h"

class UButton;

UCLASS()
class BUCKSHOTMOJAK_API UTitleUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* Start;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Ui")
	FName MainLevelName = FName("MainLevel");

private:
	UFUNCTION()
	void OnStartButtonClicked();
};
