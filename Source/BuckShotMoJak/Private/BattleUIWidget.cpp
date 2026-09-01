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

	if (ShootDealer)
	{
		ShootDealer->OnClicked.AddDynamic(this, &UBattleUIWidget::OnShootOpponentClicked);
	}

	if (ShootME)
	{
		ShootME->OnClicked.AddDynamic(this, &UBattleUIWidget::OnShootSelfClicked);
	}

	ABuckshotGameMode* GM = Cast<ABuckshotGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnShellsLoaded.AddDynamic(this, &UBattleUIWidget::UpdateShells);
		GM->OnShotFired.AddDynamic(this, &UBattleUIWidget::OnShotFiredHandler);
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

void UBattleUIWidget::SpawnNextShellIcon()
{
	if (!ShellContainer || !ShellIconClass) return;

	// 모든 총알 생성이 끝났을 때 -> 2초 뒤 사라지도록 타이머 실행
	if (!CachedMagazine.IsValidIndex(CurrentSpawnIndex))
	{
		GetWorld()->GetTimerManager().ClearTimer(ShellSpawnTimerHandle);

		// ★ 2.0초 후 ClearAllShellIcons 호출
		GetWorld()->GetTimerManager().SetTimer(
			ShellClearTimerHandle,
			this,
			&UBattleUIWidget::ClearAllShellIcons,
			2.0f, // 대기 시간 (2초)
			false
		);
		return;
	}

	EBulletType ShellType = CachedMagazine[CurrentSpawnIndex];
	UShellIcon* NewShell = CreateWidget<UShellIcon>(this, ShellIconClass);
	if (NewShell)
	{
		bool bIsLive = (ShellType == EBulletType::Live);
		NewShell->SetShellType(bIsLive);

		UHorizontalBoxSlot* NewSlot = ShellContainer->AddChildToHorizontalBox(NewShell);
		if (NewSlot)
		{
			// 간격 및 정렬 조절 (좌우 간격 8px)
			NewSlot->SetPadding(FMargin(8.0f, 0.0f));
			NewSlot->HorizontalAlignment = HAlign_Center;
			NewSlot->VerticalAlignment = VAlign_Center;
		}
	}

	CurrentSpawnIndex++;
}

void UBattleUIWidget::ClearAllShellIcons()
{
	if (ShellContainer)
	{
		ShellContainer->ClearChildren();
	}
}

// 매개변수 1개 수신
void UBattleUIWidget::UpdateShells(const TArray<EBulletType>& Magazine)
{
	if (!ShellContainer || !ShellIconClass) return;

	// 기존 아이콘 및 타이머 초기화
	ShellContainer->ClearChildren();
	GetWorld()->GetTimerManager().ClearTimer(ShellSpawnTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ShellClearTimerHandle);

	CachedMagazine = Magazine;
	CurrentSpawnIndex = 0;

	if (CachedMagazine.Num() == 0) return;

	// 0.2초 간격으로 총알 아이콘 순차 생성 시작
	GetWorld()->GetTimerManager().SetTimer(
		ShellSpawnTimerHandle,
		this,
		&UBattleUIWidget::SpawnNextShellIcon,
		0.2f,
		true
	);
}

void UBattleUIWidget::OnShotFiredHandler(EBulletType ShellType, ETargetType Target)
{
}