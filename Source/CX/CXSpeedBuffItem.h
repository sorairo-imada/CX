// CXSpeedBuffItem.h

#pragma once

#include "CoreMinimal.h"
#include "CXItemBase.h"
#include "CXSpeedBuffItem.generated.h"

/** 오버랩 시 추가 최대 속도 상승 */
UCLASS()
class CX_API ACXSpeedBuffItem : public ACXItemBase
{
	GENERATED_BODY()

public:
	ACXSpeedBuffItem();

protected:
	virtual bool ApplyItemEffect(ACXPawn* Vehicle, ACXPlayerState* VehiclePlayerState) override;

	/** 추가 최대 속도 증가량 (km/h) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Item")
	float BonusMaxSpeedAmount = 10.0f;
};
