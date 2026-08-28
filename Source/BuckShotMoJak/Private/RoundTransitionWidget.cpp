#include "RoundTransitionWidget.h"

void URoundTransitionWidget::NotifyTransitionFinished()
{
	OnTransitionFinished.Broadcast();
}