#include "BattleUIWidget.h"

#include "BuckshotGameMode.h"
#include "ShellIcon.h"
#include "ItemSlotWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/Widget.h"
#include "Components/Image.h"

// ======================================================
// NativeConstruct
// ======================================================

void UBattleUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	// --------------------------------------------------
	// 플레이어 아이템 슬롯 생성
	// --------------------------------------------------

	CreateItemSlots();

	// --------------------------------------------------
	// 딜러 아이템 슬롯 생성
	// --------------------------------------------------

	CreateDealerItemSlots();

	// --------------------------------------------------
	// GameMode Delegate 연결
	// --------------------------------------------------

	GameMode->OnShotFired.AddDynamic(
		this,
		&UBattleUIWidget::OnShotFiredHandler
	);

	GameMode->OnShellEjected.AddDynamic(
		this,
		&UBattleUIWidget::OnShellEjectedHandler
	);

	GameMode->OnShellsLoaded.AddDynamic(
		this,
		&UBattleUIWidget::OnShellsLoadedHandler
	);

	GameMode->OnTurnChanged.AddDynamic(
		this,
		&UBattleUIWidget::OnTurnChangedHandler
	);

	// --------------------------------------------------
	// 초기 UI 갱신
	// --------------------------------------------------

	RefreshItemSlots();

	SetButtonsEnabled(
		GameMode->IsPlayerTurn
	);
}


// ======================================================
// NativeDestruct
// ======================================================

void UBattleUIWidget::NativeDestruct()
{
	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (GameMode)
	{
		GameMode->OnShotFired.RemoveDynamic(
			this,
			&UBattleUIWidget::OnShotFiredHandler
		);

		GameMode->OnShellEjected.RemoveDynamic(
			this,
			&UBattleUIWidget::OnShellEjectedHandler
		);

		GameMode->OnShellsLoaded.RemoveDynamic(
			this,
			&UBattleUIWidget::OnShellsLoadedHandler
		);

		GameMode->OnTurnChanged.RemoveDynamic(
			this,
			&UBattleUIWidget::OnTurnChangedHandler
		);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		GetWorld()->GetTimerManager().ClearTimer(
			ShellClearTimerHandle
		);

		GetWorld()->GetTimerManager().ClearTimer(
			ShellEjectFinishTimerHandle
		);

		GetWorld()->GetTimerManager().ClearTimer(
			UIAnimationTimerHandle
		);

		GetWorld()->GetTimerManager().ClearTimer(
			MagnifierDisplayTimerHandle
		);
	}


	ClearAllShellIcons();

	Super::NativeDestruct();
}


// ======================================================
// 버튼 활성 / 비활성
// ======================================================

void UBattleUIWidget::ShowCurrentMagazine()
{
	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	if (!ShellContainer || !ShellIconClass)
	{
		return;
	}

	if (GameMode->Magazine.Num() <= 0)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(
		MagnifierDisplayTimerHandle
	);

	ClearAllShellIcons();

	const EBulletType CurrentShell =
		GameMode->Magazine[0];

	UShellIcon* NewShellIcon =
		CreateWidget<UShellIcon>(
			GetWorld(),
			ShellIconClass
		);

	if (!NewShellIcon)
	{
		return;
	}

	NewShellIcon->SetShellType(
		CurrentShell
	);

	ShellContainer->AddChild(
		NewShellIcon
	);

	ActiveShellIcons.Add(
		NewShellIcon
	);

	GetWorld()->GetTimerManager().SetTimer(
		MagnifierDisplayTimerHandle,
		this,
		&UBattleUIWidget::ClearAllShellIcons,
		2.0f,
		false
	);

	if (GEngine)
	{
		const TCHAR* ShellText =
			CurrentShell == EBulletType::Live
			? TEXT("LIVE")
			: TEXT("BLANK");

		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Green,
			FString::Printf(
				TEXT("[돋보기] 현재 장전된 탄 -> %s"),
				ShellText
			)
		);
	}
}
void UBattleUIWidget::ShowEjectedShell(EBulletType BulletType)
{
	if (!ShellDisplayImage)
	{
		return;
	}

	// [안전장치 추가] 텍스처 자체가 할당되어 있는지 반드시 확인
	UTexture2D* SelectedTexture = (BulletType == EBulletType::Live) ? LiveShellTexture : BlankShellTexture;
	if (!SelectedTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleUIWidget] LiveShellTexture 또는 BlankShellTexture가 할당되지 않았습니다!"));
		return;
	}

	ShellDisplayImage->SetBrushFromTexture(SelectedTexture);
	ShellDisplayImage->SetVisibility(ESlateVisibility::Visible);

	GetWorld()->GetTimerManager().ClearTimer(ShellImageTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		ShellImageTimerHandle,
		this,
		&UBattleUIWidget::HideShellImage,
		1.0f,
		false
	);
}

void UBattleUIWidget::SetButtonsEnabled(
	bool bInEnable
)
{
	if (ShootDealer)
	{
		ShootDealer->SetIsEnabled(
			bInEnable
		);
	}

	if (ShootME)
	{
		ShootME->SetIsEnabled(
			bInEnable
		);
	}
}


// ======================================================
// 플레이어 아이템 슬롯 생성
// ======================================================

void UBattleUIWidget::CreateItemSlots()
{
	if (!ItemContainer)
	{
		return;
	}

	if (!ItemSlotClass)
	{
		return;
	}

	// 이미 만들어져 있으면 중복 생성 방지
	if (ItemSlots.Num() > 0)
	{
		return;
	}

	for (int32 i = 0; i < 6; ++i)
	{
		UItemSlotWidget* NewSlot =
			CreateWidget<UItemSlotWidget>(
				GetWorld(),
				ItemSlotClass
			);

		if (!NewSlot)
		{
			continue;
		}

		// 플레이어 슬롯 설정
		NewSlot->SlotIndex = i;
		NewSlot->bIsPlayerSlot = true;

		ItemSlots.Add(
			NewSlot
		);

		ItemContainer->AddChild(
			NewSlot
		);
	}
}

void UBattleUIWidget::PlayDelayedUIAnimation()
{
	if (!Anim_ShotRecoil)
	{
		return;
	}

	PlayAnimation(
		Anim_ShotRecoil,
		0.0f,
		1,
		EUMGSequencePlayMode::Forward,
		1.0f
	);
}


// ======================================================
// 딜러 아이템 슬롯 생성
// ======================================================

void UBattleUIWidget::CreateDealerItemSlots()
{
	if (!DealerItemContainer)
	{
		return;
	}

	if (!ItemSlotClass)
	{
		return;
	}

	if (DealerItemSlots.Num() > 0)
	{
		return;
	}

	for (int32 i = 0; i < 6; ++i)
	{
		UItemSlotWidget* NewSlot =
			CreateWidget<UItemSlotWidget>(
				GetWorld(),
				ItemSlotClass
			);

		if (!NewSlot)
		{
			continue;
		}

		// 딜러 슬롯 설정
		NewSlot->SlotIndex = i;
		NewSlot->bIsPlayerSlot = false;

		DealerItemSlots.Add(
			NewSlot
		);

		DealerItemContainer->AddChild(
			NewSlot
		);
	}
}


// ======================================================
// 플레이어 아이템 UI 갱신
// ======================================================

void UBattleUIWidget::RefreshItemSlots()
{
	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	if (ItemSlots.Num() != 6)
	{
		CreateItemSlots();
	}

	// --------------------------------------------------
	// 플레이어 슬롯
	// --------------------------------------------------

	for (int32 i = 0; i < ItemSlots.Num(); ++i)
	{
		UItemSlotWidget* SlotWidget =
			ItemSlots[i];

		if (!SlotWidget)
		{
			continue;
		}

		const FItemSlot ItemSlot =
			GameMode->GetPlayerItemSlot(i);

		// --------------------------------------------------
		// ItemSlotWidget.h에 실제로 존재하는 함수 사용
		// --------------------------------------------------

		SlotWidget->SetSlotData(
			ItemSlot.ItemType,
			GameMode->GetItemTexture(
				ItemSlot.ItemType
			),
			ItemSlot.Quantity,
			i,
			true
		);

		// 현재 플레이어 턴이고
		// 아이템 사용 중이 아니며
		// 슬롯에 아이템이 있을 때만 활성화
		const bool bCanUse =
			GameMode->IsPlayerTurn &&
			!GameMode->IsItemUseInProgress() &&
			ItemSlot.ItemType != EItemType::None &&
			ItemSlot.Quantity > 0;

		SlotWidget->SetSlotEnabled(
			bCanUse
		);
	}

	// --------------------------------------------------
	// 딜러 아이템
	// --------------------------------------------------

	RefreshDealerItemSlots();

	// --------------------------------------------------
	// Blueprint 쪽 갱신
	// --------------------------------------------------

	BP_RefreshItemSlots();
}


// ======================================================
// 딜러 아이템 UI 갱신
// ======================================================

void UBattleUIWidget::RefreshDealerItemSlots()
{
	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	if (DealerItemSlots.Num() != 6)
	{
		CreateDealerItemSlots();
	}

	for (int32 i = 0; i < DealerItemSlots.Num(); ++i)
	{
		UItemSlotWidget* SlotWidget =
			DealerItemSlots[i];

		if (!SlotWidget)
		{
			continue;
		}

		const FItemSlot ItemSlot =
			GameMode->GetDealerItemSlot(i);

		// ItemSlotWidget.h의 실제 함수 사용
		SlotWidget->SetSlotData(
			ItemSlot.ItemType,
			GameMode->GetItemTexture(
				ItemSlot.ItemType
			),
			ItemSlot.Quantity,
			i,
			false
		);

		// 딜러 아이템은 직접 클릭하지 않음
		SlotWidget->SetSlotEnabled(
			false
		);
	}
}


// ======================================================
// 사격 이벤트
// ======================================================

void UBattleUIWidget::OnShotFiredHandler(
	EBulletType ShellType,
	ETargetType Target
)
{
	// 사격 반동 애니메이션
	if (Anim_ShotRecoil)
	{
		PlayAnimation(
			Anim_ShotRecoil,
			0.0f,
			1,
			EUMGSequencePlayMode::Forward,
			1.0f
		);
	}
}


// ======================================================
// 탄피 배출 이벤트
// ======================================================

void UBattleUIWidget::OnShellEjectedHandler(
	EBulletType ShellType,
	bool bFromPlayer
)
{
	// --------------------------------------------------
	// 탄피 방향
	//
	// 플레이어가 쐈다
	//     → 탄피는 딜러 방향
	//
	// 딜러가 쐈다
	//     → 탄피는 플레이어 방향
	// --------------------------------------------------

	UWidgetAnimation* EjectAnimation = nullptr;

	if (bFromPlayer)
	{
		EjectAnimation = Anim_EjectToDealer;
	}
	else
	{
		EjectAnimation = Anim_EjectToPlayer;
	}


	// --------------------------------------------------
	// 이벤트 자체가 들어왔는지 확인
	// --------------------------------------------------

	if (GEngine)
	{
		const TCHAR* ShooterText =
			bFromPlayer
			? TEXT("PLAYER")
			: TEXT("DEALER");

		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Yellow,
			FString::Printf(
				TEXT("[탄피 이벤트] Shooter = %s"),
				ShooterText
			)
		);
	}


	// --------------------------------------------------
	// 애니메이션 포인터 확인
	// --------------------------------------------------

	if (!EjectAnimation)
	{
		if (GEngine)
		{
			if (bFromPlayer)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					8.0f,
					FColor::Red,
					TEXT("[탄피 실패] Anim_EjectToDealer가 NULL입니다.")
				);
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					8.0f,
					FColor::Red,
					TEXT("[탄피 실패] Anim_EjectToPlayer가 NULL입니다.")
				);
			}
		}

		return;
	}


	// --------------------------------------------------
	// 기존 재생 정지
	// --------------------------------------------------

	StopAnimation(EjectAnimation);


	// --------------------------------------------------
	// 시작 위치를 확실하게 0초로 초기화
	// --------------------------------------------------

	SetAnimationCurrentTime(
		EjectAnimation,
		0.0f
	);


	// --------------------------------------------------
	// 탄피 애니메이션 재생
	// --------------------------------------------------

	UUMGSequencePlayer* SequencePlayer =
		PlayAnimation(
			EjectAnimation,
			0.0f,
			1,
			EUMGSequencePlayMode::Forward,
			1.0f
		);


	// --------------------------------------------------
	// PlayAnimation 결과 확인
	// --------------------------------------------------

	if (!SequencePlayer)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				8.0f,
				FColor::Red,
				TEXT("[탄피 실패] PlayAnimation()이 SequencePlayer를 생성하지 못했습니다.")
			);
		}

		return;
	}


	// --------------------------------------------------
	// 성공
	// --------------------------------------------------

	if (GEngine)
	{
		const TCHAR* DirectionText =
			bFromPlayer
			? TEXT("PLAYER -> DEALER")
			: TEXT("DEALER -> PLAYER");

		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Green,
			FString::Printf(
				TEXT("[탄피 애니메이션 성공] %s"),
				DirectionText
			)
		);
	}
}


// ======================================================
// 탄창 장전 이벤트
// ======================================================

void UBattleUIWidget::OnShellsLoadedHandler(
	const TArray<EBulletType>& Magazine
)
{
	// 기존 탄환 아이콘 제거
	ClearAllShellIcons();

	// 재장전 애니메이션 상태
	bIsReloadingAnimation = true;

	// 버튼 잠금
	SetButtonsEnabled(false);

	// 표시할 탄 종류 저장
	PendingShellsToSpawn =
		Magazine;

	// 하나씩 생성
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


// ======================================================
// 탄환 하나씩 생성
// ======================================================

void UBattleUIWidget::SpawnNextShellIcon()
{
	if (!GetWorld())
	{
		return;
	}

	if (!ShellContainer)
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		return;
	}

	if (!ShellIconClass)
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		HideShellsAndEnableButtons();

		return;
	}

	// --------------------------------------------------
	// 남은 탄환이 없음
	// --------------------------------------------------

	if (PendingShellsToSpawn.Num() == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		// 탄환을 잠시 보여준 뒤 숨김
		GetWorld()->GetTimerManager().SetTimer(
			ShellClearTimerHandle,
			this,
			&UBattleUIWidget::HideShellsAndEnableButtons,
			2.0f,
			false
		);

		return;
	}

	// --------------------------------------------------
	// 첫 번째 탄환 가져오기
	// --------------------------------------------------

	const EBulletType ShellType =
		PendingShellsToSpawn[0];

	PendingShellsToSpawn.RemoveAt(0);

	// --------------------------------------------------
	// 아이콘 생성
	// --------------------------------------------------

	UShellIcon* NewShellIcon =
		CreateWidget<UShellIcon>(
			GetWorld(),
			ShellIconClass
		);

	if (!NewShellIcon)
	{
		return;
	}

	NewShellIcon->SetShellType(
		ShellType
	);

	ShellContainer->AddChild(
		NewShellIcon
	);

	ActiveShellIcons.Add(
		NewShellIcon
	);
}


// ======================================================
// 탄환 UI 숨기기 + 버튼 활성화
// ======================================================

void UBattleUIWidget::HideShellsAndEnableButtons()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(
			ShellSpawnTimerHandle
		);

		GetWorld()->GetTimerManager().ClearTimer(
			ShellClearTimerHandle
		);
	}

	ClearAllShellIcons();

	PendingShellsToSpawn.Empty();

	bIsReloadingAnimation = false;

	ABuckshotGameMode* GameMode =
		Cast<ABuckshotGameMode>(
			UGameplayStatics::GetGameMode(this)
		);

	if (!GameMode)
	{
		return;
	}

	if (
		GameMode->IsPlayerTurn &&
		!GameMode->IsItemUseInProgress()
		)
	{
		SetButtonsEnabled(true);
	}
	else
	{
		SetButtonsEnabled(false);
	}

	RefreshItemSlots();
}


// ======================================================
// 탄환 아이콘 전체 삭제
// ======================================================

void UBattleUIWidget::HideShellImage()
{
	if (ShellDisplayImage)
	{
		ShellDisplayImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBattleUIWidget::ClearAllShellIcons()
{
	if (ShellContainer)
	{
		ShellContainer->ClearChildren();
	}

	ActiveShellIcons.Empty();
}


// ======================================================
// 플레이어 턴 변경
// ======================================================

void UBattleUIWidget::OnTurnChangedHandler(
	bool bPlayerTurn
)
{
	RefreshItemSlots();

	if (!GetWorld())
	{
		SetButtonsEnabled(false);
		return;
	}

	// 턴이 바뀌는 순간 발사 버튼 잠금
	SetButtonsEnabled(false);

	if (bIsReloadingAnimation)
	{
		return;
	}

	// --------------------------------------------------
	// 플레이어 턴
	// --------------------------------------------------

	if (bPlayerTurn)
	{
		// 플레이어 턴 시작 시 기본적으로 딜러 방향을 바라본다.
		// Self를 선택하면 PlayTargetAimAnimation(Self)가
		// 반대 방향으로 돌린다.

		if (Anim_TurnShotgun)
		{
			StopAnimation(Anim_TurnShotgun);

			const float AnimationLength =
				FMath::Max(
					Anim_TurnShotgun->GetEndTime(),
					0.01f
				);

			SetAnimationCurrentTime(
				Anim_TurnShotgun,
				0.0f
			);

			PlayAnimation(
				Anim_TurnShotgun,
				0.0f,
				1,
				EUMGSequencePlayMode::Forward,
				1.0f
			);

			FTimerHandle PlayerAimFinishTimer;

			GetWorld()->GetTimerManager().SetTimer(
				PlayerAimFinishTimer,
				[this]()
				{
					ABuckshotGameMode* GameMode =
						Cast<ABuckshotGameMode>(
							UGameplayStatics::GetGameMode(this)
						);

					if (
						GameMode &&
						GameMode->IsPlayerTurn &&
						!GameMode->IsItemUseInProgress() &&
						!GameMode->bIsReloadTransitionPlaying &&
						!GameMode->bIsEndingPlaying
						)
					{
						SetButtonsEnabled(true);
					}
				},
				AnimationLength,
				false
			);
		}
		else
		{
			SetButtonsEnabled(true);
		}
	}

	// --------------------------------------------------
	// 딜러 턴
	// --------------------------------------------------

	else
	{
		// 실제 총 방향 전환은 GameMode::TriggerDealerTurn()
		// 에서 처리한다.
		SetButtonsEnabled(false);
	}
}


float UBattleUIWidget::PlayTargetAimAnimation(ETargetType Target)
{
	if (!Anim_TurnShotgun)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Red,
				TEXT("[총 방향 오류] Anim_TurnShotgun이 바인딩되지 않았습니다.")
			);
		}

		return 0.01f;
	}

	const float AnimationLength =
		FMath::Max(
			Anim_TurnShotgun->GetEndTime(),
			0.01f
		);

	StopAnimation(Anim_TurnShotgun);

	// --------------------------------------------------
	// Self = 애니메이션 끝 → 시작
	// Opponent = 애니메이션 시작 → 끝
	// --------------------------------------------------

	if (Target == ETargetType::Self)
	{
		// Reverse 시작점을 명시적으로 끝으로 설정
		SetAnimationCurrentTime(
			Anim_TurnShotgun,
			AnimationLength
		);

		PlayAnimation(
			Anim_TurnShotgun,
			AnimationLength,
			1,
			EUMGSequencePlayMode::Reverse,
			1.0f
		);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				1.5f,
				FColor::Cyan,
				TEXT("[총 방향] Self → Reverse 재생")
			);
		}
	}
	else
	{
		// Forward 시작점을 명시적으로 0으로 설정
		SetAnimationCurrentTime(
			Anim_TurnShotgun,
			0.0f
		);

		PlayAnimation(
			Anim_TurnShotgun,
			0.0f,
			1,
			EUMGSequencePlayMode::Forward,
			1.0f
		);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				1.5f,
				FColor::Cyan,
				TEXT("[총 방향] Opponent → Forward 재생")
			);
		}
	}

	return AnimationLength;
}


// ======================================================
// 외부에서 플레이어 방향 총 애니메이션 실행
// ======================================================

void UBattleUIWidget::PlayTurnShotgunAnimation()
{
	if (!Anim_TurnShotgun)
	{
		return;
	}

	PlayAnimation(
		Anim_TurnShotgun,
		0.0f,
		1,
		EUMGSequencePlayMode::Forward,
		1.0f
	);
}