// CXCoinItem.h

#pragma once

#include "CoreMinimal.h"
#include "CXItemBase.h"
#include "CXCoinItem.generated.h"

/** 오버랩 시 원화 획득 */
UCLASS()
class CX_API ACXCoinItem : public ACXItemBase
{
	GENERATED_BODY()

public:
	ACXCoinItem();

protected:
	virtual bool ApplyItemEffect(ACXPawn* Vehicle, ACXPlayerState* VehiclePlayerState) override;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Item")
	int32 CoinAmount = 10;
};
