#include "ShellIcon.h"
#include "Components/Image.h"

void UShellIcon::NativeConstruct()
{
	Super::NativeConstruct();

	if (Anim_PopUp)
	{
		PlayAnimation(Anim_PopUp);
	}
}

void UShellIcon::SetShellType(bool bIsLive)
{
	if (!ShellImage) return;

	UTexture2D* TargetTexture = bIsLive ? LiveShellTexture : BlankShellTexture;
	if (TargetTexture)
	{
		ShellImage->SetBrushFromTexture(TargetTexture);
	}
}
