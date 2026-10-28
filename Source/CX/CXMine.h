// CXMine.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXMine.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;

/**
 *  지뢰 (도전)
 *  - 서버가 스폰, Owner = 설치한 차량 (설치자는 밟아도 발동 안 함)
 *  - 다른 차량이 오버랩하면 가속 페달 고장(2초) + 폭발 FX + 파괴
 */
UCLASS()
class CX_API ACXMine : public AActor
{
	GENERATED_BODY()

public:
	ACXMine();

	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

protected:
	UFUNCTION()
	void OnMineBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void PlayExplosionEffects();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Mine")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Mine")
	TObjectPtr<UStaticMeshComponent> MineMesh;

	/** 가속 페달 고장 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Mine")
	float AcceleratorBreakDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Mine|FX")
	TObjectPtr<UParticleSystem> ExplosionParticle;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Mine|FX")
	TObjectPtr<USoundBase> ExplosionSound;

	bool bTriggered = false;
};
