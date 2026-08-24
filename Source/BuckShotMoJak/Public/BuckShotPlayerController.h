// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BuckShotPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BUCKSHOTMOJAK_API ABuckShotPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
private:
	void SetCinematicCameraView();
};
