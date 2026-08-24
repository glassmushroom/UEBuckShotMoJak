// Fill out your copyright notice in the Description page of Project Settings.


#include "TitlePlayerController.h"
#include "Blueprint/UserWidget.h"

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// UI 위젯 생성 및 화면 추가
	if (MainMenuWidgetClass)
	{
		UUserWidget* MainMenu = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
		if (MainMenu)
		{
			MainMenu->AddToViewport();
		}
	}

	// 마우스 커서 표시 및 UI 입력 모드 설정
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
}