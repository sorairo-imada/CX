// CXItemSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"

class ACXItemBase;
class UBillboardComponent;

/**
 *  아이템 스포너 (서버 전용, 복제 X)
 *  - 생성한 아이템을 속성으로 저장하고 OnDestroyed 델리게이트를 구독
 *  - 아이템이 파괴되면 RespawnDelay 뒤 같은 종류의 아이템을 리스폰
 */
UCLASS()
class CX_API ACXItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACXItemSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** [Server] 아이템 스폰 */
	void SpawnItem();

	UFUNCTION()
	void OnSpawnedItemDestroyed(AActor* DestroyedActor);

protected:
	UPROPERTY(VisibleAnywhere, Category = "CX|Spawner")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "CX|Spawner")
	TObjectPtr<UBillboardComponent> Billboard;
#endif

	/** 스포너 별로 코인 / 버프 BP 지정 (레벨 인스턴스에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Spawner")
	TSubclassOf<ACXItemBase> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Spawner", meta = (ClampMin = "0.0"))
	float RespawnDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Spawner")
	FVector SpawnOffset = FVector(0.0f, 0.0f, 80.0f);

	/** 현재 스폰되어 있는 아이템 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "CX|Spawner")
	TObjectPtr<ACXItemBase> SpawnedItem;

	FTimerHandle RespawnTimerHandle;
};
