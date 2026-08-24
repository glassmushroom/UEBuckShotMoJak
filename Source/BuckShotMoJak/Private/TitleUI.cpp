// Fill out your copyright notice in the Description page of Project Settings.


#include "TitleUI.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UTitleUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (Start)
	{
		Start->OnClicked.AddDynamic(this, &UTitleUI::OnStartButtonClicked);
	}
}

void UTitleUI::OnStartButtonClicked()
{
	if (!MainLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this,MainLevelName);
	}
}
