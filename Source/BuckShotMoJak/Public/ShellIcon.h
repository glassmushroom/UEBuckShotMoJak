// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuckshotGameMode.h" // EBulletType Enum 사용을 위해 포함
#include "ShellIcon.generated.h"

class UImage;
class UWidgetAnimation;

UCLASS()
class BUCKSHOTMOJAK_API UShellIcon : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	UImage* ShellImage;

	// 디테일 패널에서 지정할 텍스처
	UPROPERTY(EditDefaultsOnly, Category = "Shell")
	UTexture2D* LiveShellTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Shell")
	UTexture2D* BlankShellTexture;

	// UMG에 만들어둔 팝업 애니메이션과 바인딩
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* Anim_PopUp;

	// 탄종 설정 및 이미지 교체 함수 (EBulletType으로 변경)
	UFUNCTION(BlueprintCallable, Category = "Shell")
	void SetShellType(EBulletType ShellType);
};