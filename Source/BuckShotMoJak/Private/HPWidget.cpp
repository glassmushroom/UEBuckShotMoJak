// Fill out your copyright notice in the Description page of Project Settings.


#include "HPWidget.h"
#include "Components/Image.h"

void UHPWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerHpArray = { PlayerHp1, PlayerHp2, PlayerHp3, PlayerHp4, PlayerHp5, PlayerHp6 };
	DealerHpArray = { DealerHp1, DealerHp2, DealerHp3, DealerHp4, DealerHp5, DealerHp6 };
}

void UHPWidget::UpdateHPUI(int32 CurrentRound, int32 PlayerHP, int32 DealerHP)
{
	int32 MaxHPForRound = CurrentRound * 2;

	// 1. 플레이어 HP UI 업데이트
	for (int32 i = 0; i < PlayerHpArray.Num(); ++i)
	{
		if (PlayerHpArray[i])
		{
			if (i < MaxHPForRound)
			{
				PlayerHpArray[i]->SetVisibility(ESlateVisibility::Visible);
				if (i < PlayerHP)
				{
					PlayerHpArray[i]->SetRenderOpacity(1.0f);
				}
				else
				{
					PlayerHpArray[i]->SetRenderOpacity(0.2f);
				}
			}
			else
			{
				PlayerHpArray[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// 2. 딜러 HP UI 업데이트
	for (int32 i = 0; i < DealerHpArray.Num(); ++i)
	{
		if (DealerHpArray[i])
		{
			if (i < MaxHPForRound)
			{
				DealerHpArray[i]->SetVisibility(ESlateVisibility::Visible);
				if (i < DealerHP)
				{
					DealerHpArray[i]->SetRenderOpacity(1.0f);
				}
				else
				{
					DealerHpArray[i]->SetRenderOpacity(0.2f);
				}
			}
			else
			{
				DealerHpArray[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}