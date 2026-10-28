// CXItemBase.cpp

#include "CXItemBase.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

ACXItemBase::ACXItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(120.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(CollisionSphere);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetGenerateOverlapEvents(false);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);

	// 네트워크 설정
	bReplicates = true;
	SetReplicateMovement(false);
	bAlwaysRelevant = true;			// 모든 플레이어가 동일하게 보도록 (거리 컬링으로 인한 가짜 Destroy 방지)
	SetNetUpdateFrequency(1.0f);	// 과제 요구사항
}

void ACXItemBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsValid(CollisionSphere))
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnItemBeginOverlap);
	}
}

void ACXItemBase::OnItemBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 판정은 서버에서만
	if (HasAuthority() == false || bConsumed)
	{
		return;
	}

	ACXPawn* Vehicle = Cast<ACXPawn>(OtherActor);
	if (IsValid(Vehicle) == false)
	{
		return;
	}

	// Playing 중에만 획득 가능
	if (ACXGameState::IsMatchPlaying(GetWorld()) == false)
	{
		return;
	}

	ACXPlayerState* VehiclePlayerState = Vehicle->GetPlayerState<ACXPlayerState>();
	if (IsValid(VehiclePlayerState) == false)
	{
		return;
	}

	if (ApplyItemEffect(Vehicle, VehiclePlayerState))
	{
		bConsumed = true;
		Destroy();	// 복제되어 클라이언트에서도 파괴됨
	}
}

void ACXItemBase::Destroyed()
{
	PlayDestroyEffects();

	Super::Destroyed();
}

void ACXItemBase::PlayDestroyEffects()
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

	if (DestroyParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, DestroyParticle, EffectLocation);
	}

	if (DestroySound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, DestroySound, EffectLocation);
	}
}
