#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTransitionWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundTransitionFinished);

UCLASS()
class BUCKSHOTMOJAK_API URoundTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// C++이 블루프린트에게
	// "라운드 전환 연출 재생해"라고 요청하는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Round Transition")
	void PlayReloadTransition(int32 RoundIndex);

	// 블루프린트 애니메이션이 끝났을 때 호출
	UFUNCTION(BlueprintCallable, Category = "Round Transition")
	void NotifyTransitionFinished();

	// C++ GameMode가 이 이벤트를 기다림
	UPROPERTY(BlueprintAssignable, Category = "Round Transition")
	FOnRoundTransitionFinished OnTransitionFinished;
};