// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "CXPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
class UParticleSystem;
class USoundBase;
class ACXMine;
class ACXPlayerState;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateVehicle, Log, All);

/**
 *  Vehicle Pawn class
 *  Handles common functionality for all vehicle types,
 *  including input handling and camera management.
 *
 *  Specific vehicle configurations are handled in subclasses.
 *
 *  [CX 추가]
 *  - 좌클릭 공격 (ServerRPC -> 라인트레이스 -> ApplyPointDamage -> 원화 강탈 / 바퀴 패널티)
 *  - F키 지뢰 설치 (ServerRPC -> 차량 뒤쪽에 스폰, Owner = 설치 차량)
 *  - 가속 페달 고장 (지뢰 피격 시 2초간 스로틀 0)
 *  - 게임 상태별 입력 제약 (Playing 에서만 운전/공격/지뢰/리셋)
 *  - 추가 최대 속도 적용 (BaseMaxSpeed + Bonus + Penalty)
 */
UCLASS(abstract)
class ACXPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

	/** Spring Arm for the front camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	/** Front Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	/** Spring Arm for the back camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	/** Back Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	/** 공격 라인트레이스 시작점 (앞 범퍼 중앙). BP 뷰포트에서 위치 조정 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Combat", meta = (AllowPrivateAccess = "true"))
	USceneComponent* FrontBumperPoint;

	/** 지뢰 스폰 위치 (차량 뒤쪽). BP 뷰포트에서 위치 조정 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Combat", meta = (AllowPrivateAccess = "true"))
	USceneComponent* MineSpawnPoint;

	/** Cast pointer to the Chaos Vehicle movement component */
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

protected:

	/** Steering Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* SteeringAction;

	/** Throttle Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ThrottleAction;

	/** Brake Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* BrakeAction;

	/** Handbrake Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* HandbrakeAction;

	/** Look Around Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAroundAction;

	/** Toggle Camera Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ToggleCameraAction;

	/** Reset Vehicle Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ResetVehicleAction;

	/** [CX] 좌클릭 공격 Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* AttackAction;

	/** [CX] F키 지뢰 설치 Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* PlaceMineAction;

	/** Keeps track of which camera is active */
	bool bFrontCameraActive = false;

	// ---------------- Combat 설정 ----------------

	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat")
	float AttackRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat")
	float AttackDamage = 10.0f;

	/** 서버 측 공격 쿨다운 (연타/패킷 스팸 방어) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat")
	float AttackCooldown = 0.5f;

	/** 피격 시 빼앗기는 원화 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat")
	int32 StealCoinAmount = 5;

	/** 바퀴 피격 시 추가 최대 속도 패널티 (양수로 입력, km/h) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat")
	float WheelHitSpeedPenalty = 30.0f;

	/** 물리 에셋에 바퀴 바디가 없을 때 사용하는 바퀴 판정 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat")
	float WheelHitRadius = 60.0f;

	/** 공격에 맞은 차량 위치에 재생할 파티클 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat|FX")
	TObjectPtr<UParticleSystem> HitParticle;

	/** 공격 적중 사운드 (선택) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Combat|FX")
	TObjectPtr<USoundBase> HitSound;

	/** 설치할 지뢰 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Mine")
	TSubclassOf<ACXMine> MineClass;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Mine")
	float MineCooldown = 2.0f;

	/** 한 차량이 동시에 유지할 수 있는 최대 지뢰 수 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Mine")
	int32 MaxActiveMines = 3;

	// ---------------- Speed 설정 ----------------

	/** 기본 최대 속도 (km/h). 실제 최대 속도 = Base + Bonus + Penalty */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Speed")
	float BaseMaxSpeed = 120.0f;

	/** 패널티로 내려갈 수 있는 최저 최대 속도 (km/h) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Speed")
	float MinMaxSpeed = 30.0f;

	// ---------------- Runtime 상태 ----------------

	/** 지뢰 피격으로 인한 가속 페달 고장 상태 (소유 클라이언트에 복제) */
	UPROPERTY(ReplicatedUsing = OnRep_AcceleratorBroken, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|State")
	bool bAcceleratorBroken = false;

	/** 입력 원본값 (게임 상태/고장/속도제한을 적용하기 전) */
	float RawThrottleInput = 0.0f;
	float RawSteeringInput = 0.0f;
	float RawBrakeInput = 0.0f;
	bool bHandbrakeInputPressed = false;

	/** 쿨다운 체크용 (클라 요청 시간 / 서버 처리 시간을 분리 → 리슨 서버에서도 정상 동작) */
	double LastAttackRequestTime = -1000.0;
	double LastAttackServerTime = -1000.0;
	double LastMineRequestTime = -1000.0;
	double LastMineServerTime = -1000.0;

	TArray<TWeakObjectPtr<ACXMine>> ActiveMines;

	FTimerHandle AcceleratorBreakTimerHandle;

public:
	ACXPawn();

	// Begin Pawn interface

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	// End Pawn interface

	// Begin Actor interface

	virtual void Tick(float Delta) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// End Actor interface

	/** [Server] 지뢰 피격 → 가속 페달 고장 */
	void BreakAccelerator(float Duration);

	/** 현재 적용 중인 최대 속도 (km/h) */
	float GetCurrentMaxSpeed() const;

	bool IsAcceleratorBroken() const { return bAcceleratorBroken; }

protected:

	/** Handles steering input */
	void Steering(const FInputActionValue& Value);

	/** Handles throttle input */
	void Throttle(const FInputActionValue& Value);

	/** Handles brake input */
	void Brake(const FInputActionValue& Value);

	/** Handles brake start/stop inputs */
	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);

	/** Handles handbrake start/stop inputs */
	void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);

	/** Handles look around input */
	void LookAround(const FInputActionValue& Value);

	/** Handles toggle camera input */
	void ToggleCamera(const FInputActionValue& Value);

	/** Handles reset vehicle input */
	void ResetVehicle(const FInputActionValue& Value);

	/** Called when the brake lights are turned on or off */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking);

	// ---------------- 입력 제약 ----------------

	/** Playing 상태에서만 운전 가능 */
	bool CanDrive() const;

	/** [Local] 원본 입력에 제약을 적용해 무브먼트 컴포넌트에 전달 */
	void ApplyDriveInputs();

	/** [Server] 원격 차량에 대한 서버측 입력 잠금 (클라 변조 방어용 보조 장치) */
	void EnforceServerInputLock();

	// ---------------- 공격 ----------------

	/** 좌클릭 입력 */
	void Attack(const FInputActionValue& Value);

	/** 공격 가능 여부 (Playing + 쿨다운) */
	bool CanAttack(double LastActionTime) const;

	UFUNCTION(Server, Reliable)
	void ServerAttack();

	/** [Server] 실제 공격: 앞 범퍼 중앙에서 라인트레이스 */
	void Fire();

	/** [Server -> All] 디버그 라인 + 적중 파티클 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAttackEffect(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& TraceEnd, bool bHitVehicle);

	/** 맞은 위치가 바퀴인지 판별 */
	bool IsWheelHit(const FHitResult& HitResult) const;

	// ---------------- 지뢰 ----------------

	/** F키 입력 */
	void PlaceMine(const FInputActionValue& Value);

	bool CanPlaceMine(double LastActionTime) const;

	UFUNCTION(Server, Reliable)
	void ServerPlaceMine();

	/** [Server] 지뢰 스폰 */
	void SpawnMine();

	// ---------------- 리셋 ----------------

	UFUNCTION(Server, Reliable)
	void ServerResetVehicle();

	/** [Server] 실제 리셋 */
	void DoResetVehicle();

	// ---------------- 가속 페달 고장 ----------------

	/** [Server] 2초 뒤 고장 해제 */
	void RepairAccelerator();

	UFUNCTION()
	void OnRep_AcceleratorBroken();

	ACXPlayerState* GetCXPlayerState() const;

public:
	/** Returns the front spring arm subobject */
	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }
	/** Returns the front camera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }
	/** Returns the back spring arm subobject */
	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }
	/** Returns the back camera subobject */
	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }
	/** Returns the cast Chaos Vehicle Movement subobject */
	FORCEINLINE const TObjectPtr<UChaosWheeledVehicleMovementComponent>& GetChaosVehicleMovement() const { return ChaosVehicleMovement; }
};
