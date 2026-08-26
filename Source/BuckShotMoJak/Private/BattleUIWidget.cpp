// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleUIWidget.h"
#include "BuckshotGameMode.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UBattleUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ShootDealer)
	{
		ShootDealer->OnClicked.AddDynamic(this, &UBattleUIWidget::OnShootOpponentClicked);
	}

	if (ShootME)
	{
		ShootME->OnClicked.AddDynamic(this, &UBattleUIWidget::OnShootSelfClicked);
	}
}

void UBattleUIWidget::OnShootOpponentClicked()
{
	ABuckshotGameMode* GM = Cast<ABuckshotGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsPlayerTurn)
	{
		GM->ShootTarget(ETargetType::Opponent);
	}
}

void UBattleUIWidget::OnShootSelfClicked()
{
	ABuckshotGameMode* GM = Cast<ABuckshotGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsPlayerTurn)
	{
		GM->ShootTarget(ETargetType::Self);
	}
}
