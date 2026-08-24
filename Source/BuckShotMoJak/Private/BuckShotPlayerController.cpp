// Fill out your copyright notice in the Description page of Project Settings.


#include "BuckShotPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "CineCameraActor.h"

void ABuckShotPlayerController::BeginPlay()
{

	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	SetCinematicCameraView();
}

void ABuckShotPlayerController::SetCinematicCameraView()
{
	TArray<AActor*>FoundCamera;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACineCameraActor::StaticClass(),FoundCamera);

	if (FoundCamera.Num() > 0)
	{
		SetViewTargetWithBlend(FoundCamera[0], 0.0f);
	}
}
