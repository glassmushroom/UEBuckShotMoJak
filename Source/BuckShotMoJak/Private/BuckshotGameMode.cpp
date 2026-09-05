#include "BuckshotGameMode.h"
#include "BattleUIWidget.h"
#include "DealrAIController.h"
#include "HPWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Algo/RandomShuffle.h"
#include "RoundTransitionWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

// 생성자: 기본 상태값 및 인벤토리 크기 초기화
ABuckshotGameMode::ABuckshotGameMode()
{
	IsPlayerTurn = false;
	IsSawOff = false;
	IsCuff = false;
	bIsReloadTransitionPlaying = false;

	CurrentRound = 0;
	PlayerHP = 0;
	DealerHP = 0;
	MaxHP = 0;

	HPWidgetInstance = nullptr;
	RoundTransitionWidgetInstance = nullptr;
	BattleUIWidgetInstance = nullptr;

	PlayerInventory.Init(EItemType::None, 8);
	DealerInventory.Init(EItemType::None, 8);
}

// 게임 시작: 카메라인스턴스 설정 및 각 UI 위젯 생성, 첫 라운드 시작
void ABuckshotGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}

	// 메인 카메라 액터 탐색
	if (!MainCameraActor)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MainCamera"), FoundActors);
		if (FoundActors.Num() > 0)
		{
			MainCameraActor = FoundActors[0];
		}
	}

	// 카메라인스턴스 뷰 전환
	if (PC && MainCameraActor)
	{
		PC->SetViewTargetWithBlend(MainCameraActor, 0.0f);
	}

	// HP UI 생성
	if (HPWidgetClass && PC)
	{
		HPWidgetInstance = CreateWidget<UHPWidget>(PC, HPWidgetClass);
		if (HPWidgetInstance)
		{
			HPWidgetInstance->AddToViewport(9999);
		}
	}

	// 라운드 전환 연출 UI 생성
	if (RoundTransitionWidgetClass && PC)
	{
		RoundTransitionWidgetInstance = CreateWidget<URoundTransitionWidget>(PC, RoundTransitionWidgetClass);
		if (RoundTransitionWidgetInstance)
		{
			RoundTransitionWidgetInstance->AddToViewport(10000);
			RoundTransitionWidgetInstance->OnTransitionFinished.AddDynamic(
				this,
				&ABuckshotGameMode::OnReloadTransitionFinished
			);
		}
	}

	// 배틀 메인 UI 생성 및 사격 버튼 이벤트 바인딩
	if (BattleUIClass && PC)
	{
		BattleUIWidgetInstance = CreateWidget<UBattleUIWidget>(PC, BattleUIClass);
		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->AddToViewport();

			if (BattleUIWidgetInstance->ShootDealer)
			{
				BattleUIWidgetInstance->ShootDealer->OnClicked.AddDynamic(this, &ABuckshotGameMode::OnShootDealerClicked);
			}
			if (BattleUIWidgetInstance->ShootME)
			{
				BattleUIWidgetInstance->ShootME->OnClicked.AddDynamic(this, &ABuckshotGameMode::OnShootMeClicked);
			}
		}
	}

	StartNextRound();
}

// 체력 UI 갱신
void ABuckshotGameMode::RefreshHPUI()
{
	if (HPWidgetInstance)
	{
		HPWidgetInstance->UpdateHPUI(CurrentRound, PlayerHP, DealerHP);
	}
}

// 탄창이 비었을 때 재장전 연출 및 전환 처리
void ABuckshotGameMode::HandleMagazineEmpty()
{
	if (bIsReloadTransitionPlaying || PlayerHP <= 0 || DealerHP <= 0)
	{
		return;
	}

	bIsReloadTransitionPlaying = true;

	if (RoundTransitionWidgetInstance)
	{
		GetWorldTimerManager().SetTimer(
			ReloadTransitionFallbackHandle,
			this,
			&ABuckshotGameMode::OnReloadTransitionFinished,
			1.5f,
			false
		);
	}
	else
	{
		OnReloadTransitionFinished();
	}
}

// 인벤토리의 비어있는 첫 번째 슬롯에 아이템 추가
void ABuckshotGameMode::AddItemToInventorySlot(EItemType Item, bool bIsPlayer)
{
	TArray<EItemType>& TargetInventory = bIsPlayer ? PlayerInventory : DealerInventory;

	for (int32 i = 0; i < TargetInventory.Num(); ++i)
	{
		if (TargetInventory[i] == EItemType::None)
		{
			TargetInventory[i] = Item;
			break;
		}
	}
}

// 라운드 전환 연출 UI 재생
void ABuckshotGameMode::PlayRoundTransitionUI(int32 RoundToDisplay)
{
	bIsReloadTransitionPlaying = true;

	if (RoundTransitionWidgetInstance)
	{
		RoundTransitionWidgetInstance->PlayReloadTransition(FMath::Clamp(RoundToDisplay, 1, 3));

		GetWorldTimerManager().SetTimer(
			ReloadTransitionFallbackHandle,
			this,
			&ABuckshotGameMode::OnReloadTransitionFinished,
			2.5f,
			false
		);
	}
	else
	{
		OnReloadTransitionFinished();
	}
}

// 재장전 연출 종료 처리: 탄창을 새로 채우고 턴 진행
void ABuckshotGameMode::OnReloadTransitionFinished()
{
	GetWorldTimerManager().ClearTimer(ReloadTransitionFallbackHandle);
	bIsReloadTransitionPlaying = false;

	LoadMagazine(CurrentRound == 1 ? 4 : 8);

	if (IsPlayerTurn)
	{
		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->SetButtonsEnabled(true);
		}
	}
	else
	{
		FTimerHandle DealerTurnTimer;
		GetWorldTimerManager().SetTimer(DealerTurnTimer, this, &ABuckshotGameMode::TriggerDealerTurn, 1.0f, false);
	}
}

// 무작위로 실탄과 공포탄을 구성하여 탄창에 장전
void ABuckshotGameMode::LoadMagazine(int32 MaxShells)
{
	Magazine.Empty();

	int32 TotalShells = FMath::RandRange(2, MaxShells);
	int32 LiveCount = FMath::RandRange(1, TotalShells - 1);
	int32 BlankCount = TotalShells - LiveCount;

	for (int32 i = 0; i < LiveCount; ++i)
	{
		Magazine.Add(EBulletType::Live);
	}
	for (int32 i = 0; i < BlankCount; ++i)
	{
		Magazine.Add(EBulletType::Blank);
	}

	TArray<EBulletType> DisplayMagazine = Magazine;

	if (OnShellsLoaded.IsBound())
	{
		OnShellsLoaded.Broadcast(DisplayMagazine);
	}

	Algo::RandomShuffle(Magazine);

	if (GEngine)
	{
		FString ReloadMsg = FString::Printf(TEXT("[재장전 완료] 총 %d발 (실탄: %d, 공포탄: %d)"), TotalShells, LiveCount, BlankCount);
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Emerald, ReloadMsg);
	}
}

// 딜러 AI 컨트롤러를 찾아 AI 턴 진행 명령 전달
void ABuckshotGameMode::TriggerDealerTurn()
{
	TArray<AActor*> FoundPawns;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundPawns);

	for (AActor* Actor : FoundPawns)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (Pawn && !Pawn->IsPlayerControlled())
		{
			ADealrAIController* DealerAI = Cast<ADealrAIController>(Pawn->GetController());
			if (DealerAI)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("[딜러 턴 진행 중...]"));
				}
				DealerAI->TakeTurn(this);
				return;
			}
		}
	}
}

// 플레이어 및 딜러에게 랜덤 아이템 지급
void ABuckshotGameMode::DistributeItems(int32 ItemCount)
{
	const TArray<EItemType> AvailableItems = {
		EItemType::Magnifier,
		EItemType::Beer,
		EItemType::Cigarette,
		EItemType::Saw,
		EItemType::Handcuffs,
		EItemType::Phone
	};

	for (int32 i = 0; i < ItemCount; ++i)
	{
		int32 RandomIndex = FMath::RandRange(0, AvailableItems.Num() - 1);
		AddItemToInventorySlot(AvailableItems[RandomIndex], true);
	}

	DistributeItemsToDealer(ItemCount);

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->RefreshItemSlots();
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("[아이템 지급] 플레이어와 딜러에게 각각 %d개 지급 완료"), ItemCount));
	}
}

// 딜러 전용 아이템 지급
void ABuckshotGameMode::DistributeItemsToDealer(int32 ItemCount)
{
	const TArray<EItemType> AvailableItems = {
		EItemType::Magnifier,
		EItemType::Beer,
		EItemType::Cigarette,
		EItemType::Saw,
		EItemType::Handcuffs,
		EItemType::Phone
	};

	for (int32 i = 0; i < ItemCount; ++i)
	{
		int32 RandomIndex = FMath::RandRange(0, AvailableItems.Num() - 1);
		AddItemToInventorySlot(AvailableItems[RandomIndex], false);
	}
}

// 슬롯 번호에 해당하는 아이템 사용 및 효과 적용
void ABuckshotGameMode::UseItemAtSlot(int32 SlotIndex, bool bIsPlayer)
{
	TArray<EItemType>& TargetInventory = bIsPlayer ? PlayerInventory : DealerInventory;

	if (!TargetInventory.IsValidIndex(SlotIndex))
	{
		return;
	}

	EItemType UsedItem = TargetInventory[SlotIndex];
	if (UsedItem == EItemType::None)
	{
		return;
	}

	bool bSuccess = false;

	switch (UsedItem)
	{
	case EItemType::Beer:
		EjectCurrentShell();
		bSuccess = true;
		break;

	case EItemType::Cigarette:
		bSuccess = UseCigarette();
		break;

	case EItemType::Saw:
		UseSaw();
		bSuccess = true;
		break;

	case EItemType::Handcuffs:
		bSuccess = UseHandcuffs();
		break;

	case EItemType::Magnifier:
		PeekNextShell();
		bSuccess = true;
		break;

	case EItemType::Phone:
		int32 DummyIdx;
		EBulletType DummyType;
		bSuccess = UsePhone(DummyIdx, DummyType);
		break;

	default:
		break;
	}

	if (bSuccess)
	{
		TargetInventory[SlotIndex] = EItemType::None;

		if (BattleUIWidgetInstance)
		{
			BattleUIWidgetInstance->RefreshItemSlots();
		}
	}
}

// 아이템 종류에 따른 텍스처 리소스 반환
UTexture2D* ABuckshotGameMode::GetItemTexture(EItemType ItemType) const
{
	switch (ItemType)
	{
	case EItemType::Saw:       return SawTexture;
	case EItemType::Phone:     return PhoneTexture;
	case EItemType::Magnifier: return MagnifierTexture;
	case EItemType::Beer:      return BeerTexture;
	case EItemType::Cigarette: return CigaretteTexture;
	case EItemType::Handcuffs: return HandcuffsTexture;
	default:                   return nullptr;
	}
}

// 인벤토리 내 특정 아이템 개수 반환
int32 ABuckshotGameMode::GetItemCountInInventory(EItemType ItemType, bool bIsPlayer) const
{
	const TArray<EItemType>& TargetInventory = bIsPlayer ? PlayerInventory : DealerInventory;
	int32 Count = 0;

	for (EItemType Item : TargetInventory)
	{
		if (Item == ItemType)
		{
			Count++;
		}
	}
	return Count;
}

// 사격 메인 로직: 데미지 계산, 사망 검사, 턴 전환 처리
bool ABuckshotGameMode::ShootTarget(ETargetType Target)
{
	if (bIsReloadTransitionPlaying || Magazine.Num() == 0)
	{
		return false;
	}

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->SetButtonsEnabled(false);
	}

	EBulletType CurrentShell = Magazine[0];
	Magazine.RemoveAt(0);

	int32 Damage = IsSawOff ? 2 : 1;
	IsSawOff = false;

	FString ShooterStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
	FString TargetStr = (Target == ETargetType::Self) ? TEXT("자신") : TEXT("상대방");
	FString ShellStr = (CurrentShell == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");

	if (GEngine)
	{
		FString ShotLog = FString::Printf(TEXT("[%s]가 [%s]에게 사격! -> 탄 종류: [%s] (데미지: %d)"), *ShooterStr, *TargetStr, *ShellStr, Damage);
		FColor LogColor = (CurrentShell == EBulletType::Live) ? FColor::Red : FColor::Silver;
		GEngine->AddOnScreenDebugMessage(-1, 3.5f, LogColor, ShotLog);
	}

	if (OnShotFired.IsBound())
	{
		OnShotFired.Broadcast(CurrentShell, Target);
	}

	// 실탄 피격 처리
	if (CurrentShell == EBulletType::Live)
	{
		if ((IsPlayerTurn && Target == ETargetType::Opponent) || (!IsPlayerTurn && Target == ETargetType::Self))
		{
			DealerHP = FMath::Max(0, DealerHP - Damage);
		}
		else
		{
			PlayerHP = FMath::Max(0, PlayerHP - Damage);
		}

		RefreshHPUI();

		// 사망 처리 시 타이머 등록 후 즉시 종료하여 추가 턴 전환 방지
		if (PlayerHP <= 0)
		{
			GetWorldTimerManager().SetTimer(RestartTimerHandle, this, &ABuckshotGameMode::ResetCurrentRound, 2.0f, false);
			return true;
		}

		if (DealerHP <= 0)
		{
			GetWorldTimerManager().SetTimer(RoundTimerHandle, this, &ABuckshotGameMode::StartNextRound, 2.0f, false);
			return true;
		}
	}

	// 탄창이 비었을 경우 재장전
	if (Magazine.Num() == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("탄창이 완전히 비었습니다. 재장전을 진행합니다."));
		}

		HandleMagazineEmpty();
		return true;
	}

	// 후속 턴 결정
	if (CurrentShell == EBulletType::Live)
	{
		SwitchTurn();
	}
	else
	{
		if (Target == ETargetType::Opponent)
		{
			SwitchTurn();
		}
		else
		{
			if (IsPlayerTurn)
			{
				if (BattleUIWidgetInstance)
				{
					BattleUIWidgetInstance->SetButtonsEnabled(true);
				}
			}
			else
			{
				FTimerHandle DealerContinueTimer;
				GetWorldTimerManager().SetTimer(DealerContinueTimer, this, &ABuckshotGameMode::TriggerDealerTurn, 1.0f, false);
			}
		}
	}

	return true;
}

// 다음 라운드 시작 설정 및 초기화
void ABuckshotGameMode::StartNextRound()
{
	GetWorldTimerManager().ClearTimer(RoundTimerHandle);
	GetWorldTimerManager().ClearTimer(RestartTimerHandle);
	GetWorldTimerManager().ClearTimer(ReloadTransitionFallbackHandle);

	CurrentRound++;

	if (CurrentRound > 3)
	{
		return;
	}

	MaxHP = CurrentRound * 2;
	PlayerHP = MaxHP;
	DealerHP = MaxHP;
	IsPlayerTurn = true;
	IsSawOff = false;
	IsCuff = false;

	PlayerInventory.Init(EItemType::None, 8);
	DealerInventory.Init(EItemType::None, 8);

	RefreshHPUI();

	int32 ItemsToGive = (CurrentRound == 2) ? 2 : ((CurrentRound == 3) ? 4 : 0);
	if (ItemsToGive > 0)
	{
		DistributeItems(ItemsToGive);
	}

	if (CurrentRound == 1)
	{
		OnReloadTransitionFinished();
	}
	else
	{
		PlayRoundTransitionUI(CurrentRound);
	}
}

// 턴 교대 로직 (수갑 효과 반영)
void ABuckshotGameMode::SwitchTurn()
{
	if (IsCuff)
	{
		IsCuff = false;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple, TEXT("[수갑 효과] 상대방의 턴이 건너뛰어졌습니다!"));
		}
	}
	else
	{
		IsPlayerTurn = !IsPlayerTurn;
	}

	if (GEngine)
	{
		FString TurnStr = IsPlayerTurn ? TEXT(">>> [플레이어 턴] <<<") : TEXT(">>> [딜러 턴] <<<");
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Blue, TurnStr);
	}

	if (BattleUIWidgetInstance)
	{
		BattleUIWidgetInstance->SetButtonsEnabled(IsPlayerTurn);
	}

	if (!IsPlayerTurn)
	{
		FTimerHandle DealerTurnTimer;
		GetWorldTimerManager().SetTimer(DealerTurnTimer, this, &ABuckshotGameMode::TriggerDealerTurn, 1.0f, false);
	}
}

// 플레이어 패배 시 해당 라운드 재시작
void ABuckshotGameMode::ResetCurrentRound()
{
	GetWorldTimerManager().ClearTimer(RestartTimerHandle);

	Magazine.Empty();
	IsPlayerTurn = true;
	IsSawOff = false;
	IsCuff = false;

	MaxHP = CurrentRound * 2;
	PlayerHP = MaxHP;
	DealerHP = MaxHP;

	PlayerInventory.Init(EItemType::None, 8);
	DealerInventory.Init(EItemType::None, 8);

	RefreshHPUI();

	int32 ItemsToGive = (CurrentRound == 2) ? 2 : ((CurrentRound == 3) ? 4 : 0);
	if (ItemsToGive > 0)
	{
		DistributeItems(ItemsToGive);
	}

	PlayRoundTransitionUI(CurrentRound);
}

// 돋보기: 현재 탄창의 첫 번째 총알 확인
EBulletType ABuckshotGameMode::PeekNextShell()
{
	if (Magazine.Num() > 0)
	{
		return Magazine[0];
	}
	return EBulletType::Blank;
}

// 맥주: 현재 총알 배출 및 UI 삭제 델리게이트 호출
EBulletType ABuckshotGameMode::EjectCurrentShell()
{
	if (bIsReloadTransitionPlaying)
	{
		return EBulletType::Blank;
	}

	if (Magazine.Num() > 0)
	{
		EBulletType Ejected = Magazine[0];
		Magazine.RemoveAt(0);

		// 맥주 사용 시 UI 탄약 아이콘 삭제용 브로드캐스트
		if (OnShotFired.IsBound())
		{
			OnShotFired.Broadcast(Ejected, ETargetType::Self);
		}

		if (GEngine)
		{
			FString EjectStr = (Ejected == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[맥주 사용] 배출된 총알: %s"), *EjectStr));
		}

		if (Magazine.Num() == 0 && PlayerHP > 0 && DealerHP > 0)
		{
			HandleMagazineEmpty();
		}
		return Ejected;
	}
	return EBulletType::Blank;
}

// 담배: HP 1 회복
bool ABuckshotGameMode::UseCigarette()
{
	FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");

	int32& CurrentHP = IsPlayerTurn ? PlayerHP : DealerHP;
	if (CurrentHP < MaxHP)
	{
		CurrentHP++;
		RefreshHPUI();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[%s] 담배 사용 -> HP 1 회복!"), *UserStr));
		}
		return true;
	}
	return false;
}

// 톱: 데미지 2배 설정
void ABuckshotGameMode::UseSaw()
{
	IsSawOff = true;
	if (GEngine)
	{
		FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[%s] 톱 사용 -> 다음 총알 데미지 2배!"), *UserStr));
	}
}

// 수갑: 상대방 턴 1회 스킵 설정
bool ABuckshotGameMode::UseHandcuffs()
{
	if (!IsCuff)
	{
		IsCuff = true;
		if (GEngine)
		{
			FString UserStr = IsPlayerTurn ? TEXT("플레이어") : TEXT("딜러");
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[%s] 수갑 사용 -> 상대 턴 1회 건너뜀"), *UserStr));
		}
		return true;
	}
	return false;
}

// 휴대폰: 랜덤한 위치의 총알 정보 확인
bool ABuckshotGameMode::UsePhone(int32& OutIndex, EBulletType& OutType)
{
	if (Magazine.Num() <= 1)
	{
		return false;
	}

	OutIndex = FMath::RandRange(1, Magazine.Num() - 1);
	OutType = Magazine[OutIndex];

	if (GEngine)
	{
		FString ShellStr = (OutType == EBulletType::Live) ? TEXT("실탄") : TEXT("공포탄");
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("[핸드폰 사용] %d번째 위치의 총알은 [%s]입니다."), OutIndex + 1, *ShellStr));
	}

	return true;
}

// UI 클릭 이벤트 핸들러: 딜러 사격
void ABuckshotGameMode::OnShootDealerClicked()
{
	if (IsPlayerTurn)
	{
		ShootTarget(ETargetType::Opponent);
	}
}

// UI 클릭 이벤트 핸들러: 자신 사격
void ABuckshotGameMode::OnShootMeClicked()
{
	if (IsPlayerTurn)
	{
		ShootTarget(ETargetType::Self);
	}
}