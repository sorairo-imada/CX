// CXSpeedBuffItem.cpp

#include "CXSpeedBuffItem.h"
#include "CXPlayerState.h"

ACXSpeedBuffItem::ACXSpeedBuffItem()
{
}

bool ACXSpeedBuffItem::ApplyItemEffect(ACXPawn* Vehicle, ACXPlayerState* VehiclePlayerState)
{
	if (IsValid(VehiclePlayerState) == false)
	{
		return false;
	}

	VehiclePlayerState->AddBonusMaxSpeed(BonusMaxSpeedAmount);
	return true;
}
