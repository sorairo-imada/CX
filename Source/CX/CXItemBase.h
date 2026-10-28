// CXItemBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class URotatingMovementComponent;
class UParticleSystem;
class USoundBase;
class ACXPawn;
class ACXPlayerState;

/**
 *  맵 아이템 공통 베이스 (코인 / 스피드 버프)
 *  - 서버에서만 오버랩 처리 → 효과 적용 → Destroy
 *  - 파괴 시 Destroyed() 에서 모든 머신이 파티클/사운드 재생
 *    (서버 Destroy 가 복제되면 클라에서도 Destroyed() 가 호출된다)
 */
UCLASS(Abstract)
class CX_API ACXItemBase : public AActor
{
	GENERATED_BODY()

public:
	ACXItemBase();

	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

protected:
	UFUNCTION()
	void OnItemBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** [Server] 아이템 효과 적용. 성공 시 true → 아이템 파괴 */
	virtual bool ApplyItemEffect(ACXPawn* Vehicle, ACXPlayerState* VehiclePlayerState) PURE_VIRTUAL(ACXItemBase::ApplyItemEffect, return false;);

	void PlayDestroyEffects();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	/** 시각 효과용 회전 (각 머신 로컬 처리, 복제 X) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Item|FX")
	TObjectPtr<UParticleSystem> DestroyParticle;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Item|FX")
	TObjectPtr<USoundBase> DestroySound;

	/** 중복 획득 방지 (스켈레탈 메시는 바디별로 오버랩이 여러 번 들어올 수 있음) */
	bool bConsumed = false;
};
