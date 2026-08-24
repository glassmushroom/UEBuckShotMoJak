	// Fill out your copyright notice in the Description page of Project Settings.


	#include "TitleUI.h"
	#include "Components/Button.h"
	#include "Kismet/GameplayStatics.h"
	#include "Kismet/KismetSystemLibrary.h"

	void UTitleUI::NativeConstruct()
	{
		Super::NativeConstruct();

		if (Start)
		{
			Start->OnClicked.AddDynamic(this, &UTitleUI::OnStartButtonClicked);
		}

		if (Exit)
		{
			Exit->OnClicked.AddDynamic(this, &UTitleUI::OnExitButtonClicked);
		}
	}

	void UTitleUI::OnStartButtonClicked()
	{
		if (!MainLevelName.IsNone())
		{
			UE_LOG(LogTemp, Log, TEXT("시작"));
			UGameplayStatics::OpenLevel(this,MainLevelName);
		}
	}

	void UTitleUI::OnExitButtonClicked()
	{
		APlayerController* PC = GetOwningPlayer();
		UE_LOG(LogTemp, Log, TEXT("나가기 성공"));
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
