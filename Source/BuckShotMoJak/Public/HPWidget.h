// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HPWidget.generated.h"

class UImage;

UCLASS()
class BUCKSHOTMOJAK_API UHPWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateHPUI(int32 CurrentRound, int32 PlayerHP, int32 DealerHP);

protected:
	virtual void NativeConstruct() override;

private:
	//플레이어
	UPROPERTY(meta = (BindWidget))
	UImage* PlayerHp1;
	UPROPERTY(meta = (BindWidget))
	UImage* PlayerHp2;
	UPROPERTY(meta = (BindWidget))
	UImage* PlayerHp3;
	UPROPERTY(meta = (BindWidget))
	UImage* PlayerHp4;
	UPROPERTY(meta = (BindWidget))
	UImage* PlayerHp5;
	UPROPERTY(meta = (BindWidget))
	UImage* PlayerHp6;

	//딜러
	UPROPERTY(meta = (BindWidget))
	UImage* DealerHp1;
	UPROPERTY(meta = (BindWidget))
	UImage* DealerHp2;
	UPROPERTY(meta = (BindWidget))
	UImage* DealerHp3;
	UPROPERTY(meta = (BindWidget))
	UImage* DealerHp4;
	UPROPERTY(meta = (BindWidget))
	UImage* DealerHp5;
	UPROPERTY(meta = (BindWidget))
	UImage* DealerHp6;

	TArray<UImage*> PlayerHpArray;
	TArray<UImage*> DealerHpArray;

	void InitHPArrays();
};
