// CXMine.cpp

#include "CXMine.h"
#include "CXPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

ACXMine::ACXMine()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(100.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	MineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MineMesh"));
	MineMesh->SetupAttachment(CollisionSphere);
	MineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MineMesh->SetGenerateOverlapEvents(false);

	bReplicates = true;
	SetReplicateMovement(false);
	bAlwaysRelevant = true;
}

void ACXMine::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(CollisionSphere))
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnMineBeginOverlap);
	}
}

void ACXMine::OnMineBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority() == false || bTriggered)
	{
		return;
	}

	// 설치한 차량(Owner)은 무시
	if (OtherActor == GetOwner())
	{
		return;
	}

	ACXPawn* Victim = Cast<ACXPawn>(OtherActor);
	if (IsValid(Victim) == false)
	{
		return;
	}

	bTriggered = true;

	// 가속 페달 고장 (2초 뒤 자동 회복)
	Victim->BreakAccelerator(AcceleratorBreakDuration);

	// 파괴 → Destroyed() 에서 폭발 FX
	Destroy();
}

void ACXMine::Destroyed()
{
	PlayExplosionEffects();

	Super::Destroyed();
}

void ACXMine::PlayExplosionEffects()
{
	UWorld* World = GetWorld();
	if (IsValid(World) == false || World->IsGameWorld() == false || World->bIsTearingDown)
	{
		return;
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const FVector EffectLocation = GetActorLocation();

	if (ExplosionParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, ExplosionParticle, EffectLocation);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, ExplosionSound, EffectLocation);
	}
}
