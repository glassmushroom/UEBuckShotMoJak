#include "BattleUIWidget.h"
#include "BuckshotGameMode.h"
#include "ShellIcon.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

void UBattleUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABuckshotGameMode* GM = Cast<ABuckshotGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnShotFired.RemoveDynamic(this, &UBattleUIWidget::OnShotFiredHandler);
		GM->OnShotFired.AddDynamic(this, &UBattleUIWidget::OnShotFiredHandler);

		GM->OnShellsLoaded.RemoveDynamic(this, &UBattleUIWidget::OnShellsLoadedHandler);
		GM->OnShellsLoaded.AddDynamic(this, &UBattleUIWidget::OnShellsLoadedHandler);
	}
}

void UBattleUIWidget::NativeDestruct()
{
	ClearAllShellIcons();
	Super::NativeDestruct();
}

void UBattleUIWidget::SetButtonsEnabled(bool bInEnable)
{
	// 만약 켜려고(true) 하는데, 현재 장전 연출 중(bIsReloadingAnimation == true)이라면 강제로 무시하고 끕니다.
	bool bFinalState = bInEnable;
	if (bIsReloadingAnimation && bInEnable)
	{
		bFinalState = false;
	}

	if (ShootDealer)
	{
		ShootDealer->SetIsEnabled(bFinalState);
	}
	if (ShootME)
	{
		ShootME->SetIsEnabled(bFinalState);
	}
}

void UBattleUIWidget::RefreshItemSlots()
{
	BP_RefreshItemSlots();
}

// ★ 사격 시: 쏜 탄창 UI 청소 및 처리
void UBattleUIWidget::OnShotFiredHandler(EBulletType ShellType, ETargetType Target)
{
	
}

// ★ 재장전 시: 버튼 비활성화 -> 0.2초 간격 탄 등장 -> 완료 후 버튼 활성화
void UBattleUIWidget::OnShellsLoadedHandler(const TArray<EBulletType>& Magazine)
{
	ClearAllShellIcons();

	// 1. 장전 플래그 ON 지정 후 버튼 즉시 비활성화
	bIsReloadingAnimation = true;
	SetButtonsEnabled(false);

	PendingShellsToSpawn = Magazine;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			ShellSpawnTimerHandle,
			this,
			&UBattleUIWidget::SpawnNextShellIcon,
			0.2f,
			true
		);
	}
}

void UBattleUIWidget::SpawnNextShellIcon()
{
	if (PendingShellsToSpawn.Num() == 0)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(ShellSpawnTimerHandle);

			// 2초 대기 타이머
			GetWorld()->GetTimerManager().SetTimer(
				ShellClearTimerHandle,
				this,
				&UBattleUIWidget::HideShellsAndEnableButtons,
				2.0f,
				false
			);
		}
		return;
	}

	EBulletType NextType = PendingShellsToSpawn[0];
	PendingShellsToSpawn.RemoveAt(0);

	if (ShellContainer && ShellIconClass)
	{
		UShellIcon* NewIcon = CreateWidget<UShellIcon>(this, ShellIconClass);
		if (NewIcon)
		{
			NewIcon->SetShellType(NextType);

			UHorizontalBoxSlot* NewSlot = ShellContainer->AddChildToHorizontalBox(NewIcon);
			if (NewSlot)
			{
				NewSlot->SetPadding(FMargin(5.0f, 0.0f, 5.0f, 0.0f));
			}

			ActiveShellIcons.Add(NewIcon);
		}
	}
}

void UBattleUIWidget::HideShellsAndEnableButtons()
{
	ClearAllShellIcons();

	// 2. 장전 플래그 OFF 전환 후 버튼 활성화
	bIsReloadingAnimation = false;
	SetButtonsEnabled(true);
}

void UBattleUIWidget::ClearAllShellIcons()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ShellSpawnTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ShellClearTimerHandle);
	}

	if (ShellContainer)
	{
		ShellContainer->ClearChildren();
	}

	ActiveShellIcons.Empty();
	PendingShellsToSpawn.Empty();
}