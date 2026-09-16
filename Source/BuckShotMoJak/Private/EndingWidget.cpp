#include "EndingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

namespace
{
	// Verified map packages: Content/Level/*.umap. Content/MainLevel is a redirector.
	const FName GameplayLevel(TEXT("/Game/Level/MainLevel"));
	const FName MenuLevel(TEXT("/Game/Level/TitleLevel"));
	constexpr float EndingDuration = 2.0f;
}

TSharedRef<SWidget> UEndingWidget::RebuildWidget()
{
	// Keep designer content intact if an existing Blueprint subclass provides it.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EndingRoot"));
		WidgetTree->RootWidget = Root;

		FadeImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Fade"));
		FadeImage->SetColorAndOpacity(FLinearColor::Black);
		FadeImage->SetRenderOpacity(0.0f);
		UCanvasPanelSlot* FadeSlot = Root->AddChildToCanvas(FadeImage);
		FadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FadeSlot->SetOffsets(FMargin(0.0f));

		ResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Result"));
		ResultText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 52));
		ResultText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		ResultText->SetShadowOffset(FVector2D::ZeroVector);
		ResultText->SetJustification(ETextJustify::Center);
		UBorder* MessageBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MessageBox"));
		MessageBox->SetBrushColor(FLinearColor(0.12f, 0.12f, 0.12f));
		MessageBox->SetPadding(FMargin(24.0f, 16.0f));
		MessageBox->SetHorizontalAlignment(HAlign_Center);
		MessageBox->SetVerticalAlignment(VAlign_Center);
		MessageBox->SetContent(ResultText);
		UCanvasPanelSlot* ResultSlot = Root->AddChildToCanvas(MessageBox);
		ResultSlot->SetAnchors(FAnchors(0.5f));
		ResultSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ResultSlot->SetSize(FVector2D(520.0f, 120.0f));

		ButtonBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EndingButtons"));
		UCanvasPanelSlot* ButtonsSlot = Root->AddChildToCanvas(ButtonBox);
		ButtonsSlot->SetAnchors(FAnchors(0.5f));
		ButtonsSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		ButtonsSlot->SetPosition(FVector2D(0.0f, 82.0f));
		ButtonsSlot->SetAutoSize(true);
		ReplayButton = CreateEndingButton(TEXT("REPLAY"));
		ReplayButton->OnClicked.AddDynamic(this, &UEndingWidget::Replay);
		CreateEndingButton(TEXT("MENU"))->OnClicked.AddDynamic(this, &UEndingWidget::ReturnToMenu);
		ButtonBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

UButton* UEndingWidget::CreateEndingButton(const TCHAR* Label)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(260.0f);
	Size->SetHeightOverride(60.0f);
	ButtonBox->AddChildToVerticalBox(Size)->SetPadding(FMargin(0.0f, 6.0f));
	UBorder* Outline = WidgetTree->ConstructWidget<UBorder>();
	Outline->SetBrushColor(FLinearColor::White);
	Outline->SetPadding(FMargin(2.0f));
	Size->AddChild(Outline);

	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(FLinearColor::White);
	Style.SetNormal(Brush);
	Brush.TintColor = FSlateColor(FLinearColor::Black);
	Style.SetHovered(Brush);
	Brush.TintColor = FSlateColor(FLinearColor(0.15f, 0.15f, 0.15f));
	Style.SetPressed(Brush);
	Style.SetNormalForeground(FSlateColor(FLinearColor::Black));
	Style.SetHoveredForeground(FSlateColor(FLinearColor::White));
	Style.SetPressedForeground(FSlateColor(FLinearColor::White));
	Style.SetNormalPadding(FMargin(0.0f));
	Style.SetPressedPadding(FMargin(0.0f));
	Button->SetStyle(Style);
	Outline->SetContent(Button);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 24));
	Text->SetColorAndOpacity(FSlateColor::UseForeground());
	Text->SetShadowOffset(FVector2D::ZeroVector);
	Button->AddChild(Text);
	return Button;
}

void UEndingWidget::PlayVictoryEnding_Implementation()
{
	if (bEndingStarted) return;
	bEndingStarted = true;
	if (ResultText) ResultText->SetText(FText::FromString(TEXT("PLAYER WIN")));
	GetWorld()->GetTimerManager().SetTimer(EndingTimerHandle, this, &UEndingWidget::ReturnToMenu, EndingDuration, false);
}

void UEndingWidget::PlayDefeatEnding_Implementation()
{
	if (bEndingStarted) return;
	bEndingStarted = true;
	if (ResultText) ResultText->SetText(FText::FromString(TEXT("DEALER WIN")));
	FadeStartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(EndingTimerHandle, this, &UEndingWidget::UpdateDefeatFade, 1.0f / 60.0f, true);
}

void UEndingWidget::UpdateDefeatFade()
{
	const float Alpha = FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - FadeStartTime) / EndingDuration), 0.0f, 1.0f);
	if (FadeImage) FadeImage->SetRenderOpacity(Alpha);
	if (Alpha >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(EndingTimerHandle);
		bDefeatButtonsReady = true;
		if (ButtonBox) ButtonBox->SetVisibility(ESlateVisibility::Visible);
		if (APlayerController* PC = GetOwningPlayer()) PC->bShowMouseCursor = true;
		if (ReplayButton) ReplayButton->SetKeyboardFocus();
	}
}

void UEndingWidget::Replay()
{
	if (bDefeatButtonsReady) OpenEndingLevel(GameplayLevel);
}

void UEndingWidget::ReturnToMenu()
{
	OpenEndingLevel(MenuLevel);
}

void UEndingWidget::OpenEndingLevel(FName LevelName)
{
	if (bTravelRequested) return;
	bTravelRequested = true;
	SetIsEnabled(false);
	GetWorld()->GetTimerManager().ClearTimer(EndingTimerHandle);
	UGameplayStatics::OpenLevel(this, LevelName, true);
}

void UEndingWidget::NativeDestruct()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(EndingTimerHandle);
	Super::NativeDestruct();
}
