// CXCoinItem.cpp

#include "CXCoinItem.h"
#include "CXPlayerState.h"

ACXCoinItem::ACXCoinItem()
{
}

bool ACXCoinItem::ApplyItemEffect(ACXPawn* Vehicle, ACXPlayerState* VehiclePlayerState)
{
	if (IsValid(VehiclePlayerState) == false)
	{
		return false;
	}

	VehiclePlayerState->AddCoins(CoinAmount);
	return true;
}
