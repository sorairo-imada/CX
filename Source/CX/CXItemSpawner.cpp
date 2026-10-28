// CXItemSpawner.cpp

#include "CXItemSpawner.h"
#include "CX.h"
#include "CXItemBase.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACXItemSpawner::ACXItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 스포너 자체는 서버 로직 전용 → 복제하지 않음
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (Billboard)
	{
		Billboard->SetupAttachment(SceneRoot);
	}
#endif
}

void ACXItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 비복제 레벨 액터는 클라이언트에서도 HasAuthority()==true 이므로 NetMode 로 검사해야 함
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	SpawnItem();
}

void ACXItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);

	if (IsValid(SpawnedItem))
	{
		SpawnedItem->OnDestroyed.RemoveDynamic(this, &ThisClass::OnSpawnedItemDestroyed);
	}

	Super::EndPlay(EndPlayReason);
}

void ACXItemSpawner::SpawnItem()
{
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	if (ItemClass == nullptr)
	{
		UE_LOG(LogCX, Warning, TEXT("[%s] ItemClass is not set."), *GetName());
		return;
	}

	if (IsValid(SpawnedItem))
	{
		return; // 이미 존재
	}

	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = GetActorLocation() + SpawnOffset;
	SpawnedItem = World->SpawnActor<ACXItemBase>(ItemClass, SpawnLocation, GetActorRotation(), SpawnParams);

	if (IsValid(SpawnedItem))
	{
		// 아이템 파괴 감지
		SpawnedItem->OnDestroyed.AddDynamic(this, &ThisClass::OnSpawnedItemDestroyed);
	}
}

void ACXItemSpawner::OnSpawnedItemDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != SpawnedItem)
	{
		return;
	}

	SpawnedItem = nullptr;

	UWorld* World = GetWorld();
	if (IsValid(World) == false || World->bIsTearingDown)
	{
		return;
	}

	// 2초 뒤 같은 종류(ItemClass) 리스폰
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ThisClass::SpawnItem, FMath::Max(RespawnDelay, 0.01f), false);
}
