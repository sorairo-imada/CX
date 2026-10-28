// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "CXMine.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// [CX] 앞 범퍼 중앙 / 뒤쪽 지뢰 스폰 위치 (BP 뷰포트에서 미세 조정)
	FrontBumperPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Front Bumper Point"));
	FrontBumperPoint->SetupAttachment(GetMesh());
	FrontBumperPoint->SetRelativeLocation(FVector(230.0f, 0.0f, 50.0f));

	MineSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Mine Spawn Point"));
	MineSpawnPoint->SetupAttachment(GetMesh());
	MineSpawnPoint->SetRelativeLocation(FVector(-350.0f, 0.0f, 50.0f));

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));
	// [CX] 코인/버프/지뢰 오버랩 감지를 위해 필요
	GetMesh()->SetGenerateOverlapEvents(true);

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, bAcceleratorBroken, COND_OwnerOnly);
}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);

		// [CX] attack (좌클릭)
		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACXPawn::Attack);
		}

		// [CX] place mine (F)
		if (PlaceMineAction)
		{
			EnhancedInputComponent->BindAction(PlaceMineAction, ETriggerEvent::Started, this, &ACXPawn::PlaceMine);
		}
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));

	// [CX] 입력 제약 적용
	if (IsLocallyControlled())
	{
		ApplyDriveInputs();
	}
	else if (HasAuthority())
	{
		EnforceServerInputLock();
	}
}

// ============================================================
// Input handlers : 원본값만 저장하고 실제 적용은 ApplyDriveInputs() 에서
// ============================================================

void ACXPawn::Steering(const FInputActionValue& Value)
{
	RawSteeringInput = Value.Get<float>();
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	RawThrottleInput = Value.Get<float>();
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	RawBrakeInput = Value.Get<float>();
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	RawBrakeInput = 0.0f;
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	bHandbrakeInputPressed = true;

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	bHandbrakeInputPressed = false;

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

// ============================================================
// 게임 상태별 입력 제약 / 최대 속도
// ============================================================

bool ACXPawn::CanDrive() const
{
	return ACXGameState::IsMatchPlaying(GetWorld());
}

ACXPlayerState* ACXPawn::GetCXPlayerState() const
{
	return GetPlayerState<ACXPlayerState>();
}

float ACXPawn::GetCurrentMaxSpeed() const
{
	const ACXPlayerState* CXPlayerState = GetCXPlayerState();
	const float Bonus = IsValid(CXPlayerState) ? CXPlayerState->GetEffectiveBonusMaxSpeed() : 0.0f;
	return FMath::Max(BaseMaxSpeed + Bonus, MinMaxSpeed);
}

void ACXPawn::ApplyDriveInputs()
{
	const bool bCanDrive = CanDrive();

	// Waiting / Finished : 스로틀, 조향(+브레이크) 0, 핸드브레이크 true 유지
	// Playing 으로 바뀌면 bCanDrive 가 true 가 되어 게임 상태용 핸드브레이크 잠금이 자동 해제됨
	float FinalThrottle = bCanDrive ? RawThrottleInput : 0.0f;
	const float FinalSteering = bCanDrive ? RawSteeringInput : 0.0f;
	const float FinalBrake = bCanDrive ? RawBrakeInput : 0.0f;
	const bool bFinalHandbrake = (bCanDrive == false) || bHandbrakeInputPressed;

	// 지뢰: 가속 페달 고장
	if (bAcceleratorBroken)
	{
		FinalThrottle = 0.0f;
	}

	// 추가 최대 속도 반영: 최대 속도 이상이면 전진 가속 차단
	const float ForwardSpeedKmh = ChaosVehicleMovement->GetForwardSpeed() * 0.036f;
	if (FinalThrottle > 0.0f && ForwardSpeedKmh >= GetCurrentMaxSpeed())
	{
		FinalThrottle = 0.0f;
	}

	ChaosVehicleMovement->SetThrottleInput(FinalThrottle);
	ChaosVehicleMovement->SetSteeringInput(FinalSteering);
	ChaosVehicleMovement->SetBrakeInput(FinalBrake);
	ChaosVehicleMovement->SetHandbrakeInput(bFinalHandbrake);
}

void ACXPawn::EnforceServerInputLock()
{
	// 클라이언트 입력은 ServerUpdateState 로 들어오므로 정상 클라이언트라면 이미 0 이다.
	// 변조된 클라이언트를 대비한 보조 잠금.
	if (CanDrive() == false)
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		ChaosVehicleMovement->SetSteeringInput(0.0f);
		ChaosVehicleMovement->SetHandbrakeInput(true);
	}
	else if (bAcceleratorBroken)
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
	}
}

// ============================================================
// Reset
// ============================================================

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// Waiting / Finished 에서는 리셋 불가
	if (CanDrive() == false)
	{
		return;
	}

	ServerResetVehicle();
}

void ACXPawn::ServerResetVehicle_Implementation()
{
	if (CanDrive() == false)
	{
		return;
	}

	DoResetVehicle();
}

void ACXPawn::DoResetVehicle()
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Log, TEXT("Reset Vehicle"));
}

// ============================================================
// Attack
// ============================================================

bool ACXPawn::CanAttack(double LastActionTime) const
{
	const UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return false;
	}

	// Playing 상태에서만 공격 허용
	if (ACXGameState::IsMatchPlaying(World) == false)
	{
		return false;
	}

	return (World->GetTimeSeconds() - LastActionTime) >= AttackCooldown;
}

void ACXPawn::Attack(const FInputActionValue& Value)
{
	if (CanAttack(LastAttackRequestTime) == false)
	{
		return;
	}

	LastAttackRequestTime = GetWorld()->GetTimeSeconds();
	ServerAttack();
}

void ACXPawn::ServerAttack_Implementation()
{
	// 서버에서 다시 검사 (클라 검사는 신뢰하지 않음)
	if (CanAttack(LastAttackServerTime) == false)
	{
		return;
	}

	LastAttackServerTime = GetWorld()->GetTimeSeconds();
	Fire();
}

void ACXPawn::Fire()
{
	if (HasAuthority() == false || IsValid(FrontBumperPoint) == false)
	{
		return;
	}

	UWorld* World = GetWorld();

	// 앞 범퍼 중앙에서 전방으로 라인트레이스
	const FVector TraceStart = FrontBumperPoint->GetComponentLocation();
	const FVector TraceEnd = TraceStart + GetActorForwardVector() * AttackRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CXVehicleAttack), false, this);
	QueryParams.AddIgnoredActor(this);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	ACXPawn* HitVehicle = bHit ? Cast<ACXPawn>(HitResult.GetActor()) : nullptr;
	const bool bHitVehicle = IsValid(HitVehicle) && HitVehicle != this;
	const FVector EffectEnd = bHit ? FVector(HitResult.ImpactPoint) : TraceEnd;

	// 모든 클라에 디버그 라인 + 파티클
	MulticastPlayAttackEffect(TraceStart, EffectEnd, bHitVehicle);

	if (bHitVehicle)
	{
		// [도전] ApplyDamage 대신 ApplyPointDamage → 맞은 Bone 정보 전달
		UGameplayStatics::ApplyPointDamage(
			HitVehicle,
			AttackDamage,
			GetActorForwardVector(),
			HitResult,
			GetController(),
			this,
			UDamageType::StaticClass());
	}
}

void ACXPawn::MulticastPlayAttackEffect_Implementation(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& TraceEnd, bool bHitVehicle)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UWorld* World = GetWorld();

	const FColor LineColor = bHitVehicle ? FColor::Red : FColor::Green;
	DrawDebugLine(World, TraceStart, TraceEnd, LineColor, false, 1.0f, 0, 3.0f);

	if (bHitVehicle)
	{
		DrawDebugPoint(World, TraceEnd, 15.0f, FColor::Red, false, 1.0f);

		// 닿은 차량에 파티클 재생
		if (HitParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, HitParticle, TraceEnd);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, TraceEnd);
		}
	}
}

float ACXPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HasAuthority() == false)
	{
		return ActualDamage;
	}

	ACXPlayerState* VictimPlayerState = GetCXPlayerState();
	ACXPlayerState* AttackerPlayerState = IsValid(EventInstigator) ? EventInstigator->GetPlayerState<ACXPlayerState>() : nullptr;

	// 원화 강탈 : 피해 차량이 5원 이상이면 피해 -5, 가해 +5. 5원 미만이면 아무 동작 안 함.
	if (IsValid(VictimPlayerState) && IsValid(AttackerPlayerState) && VictimPlayerState != AttackerPlayerState)
	{
		if (VictimPlayerState->RemoveCoins(StealCoinAmount))
		{
			AttackerPlayerState->AddCoins(StealCoinAmount);
		}
	}

	// [도전] 부위 공격 : 바퀴 피격 시 추가 최대 속도 음수 패널티
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID) && IsValid(VictimPlayerState))
	{
		const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (IsWheelHit(PointDamageEvent.HitInfo))
		{
			UE_LOG(LogTemplateVehicle, Log, TEXT("[%s] Wheel hit! Bone: %s"), *GetName(), *PointDamageEvent.HitInfo.BoneName.ToString());
			VictimPlayerState->ApplySpeedPenalty(WheelHitSpeedPenalty);
		}
	}

	return ActualDamage;
}

bool ACXPawn::IsWheelHit(const FHitResult& HitResult) const
{
	// 1) 물리 에셋에 바퀴 바디가 있다면 BoneName 으로 바로 판별
	const FName HitBoneName = HitResult.BoneName;
	if (HitBoneName != NAME_None && HitBoneName.ToString().Contains(TEXT("Wheel")))
	{
		return true;
	}

	// 2) 바퀴 바디가 없는 경우(차량 템플릿 기본) : 바퀴 Bone 위치와 피격 지점 거리로 판별
	const USkeletalMeshComponent* VehicleMesh = GetMesh();
	if (IsValid(VehicleMesh) == false || ChaosVehicleMovement == nullptr)
	{
		return false;
	}

	for (const FChaosWheelSetup& WheelSetup : ChaosVehicleMovement->WheelSetups)
	{
		if (WheelSetup.BoneName == NAME_None || VehicleMesh->GetBoneIndex(WheelSetup.BoneName) == INDEX_NONE)
		{
			continue;
		}

		if (HitBoneName == WheelSetup.BoneName)
		{
			return true;
		}

		const FVector WheelLocation = VehicleMesh->GetBoneLocation(WheelSetup.BoneName);
		if (FVector::DistSquared(WheelLocation, HitResult.ImpactPoint) <= FMath::Square(WheelHitRadius))
		{
			return true;
		}
	}

	return false;
}

// ============================================================
// Mine
// ============================================================

bool ACXPawn::CanPlaceMine(double LastActionTime) const
{
	const UWorld* World = GetWorld();
	if (IsValid(World) == false || MineClass == nullptr)
	{
		return false;
	}

	// Playing 상태에서만 지뢰 설치 허용
	if (ACXGameState::IsMatchPlaying(World) == false)
	{
		return false;
	}

	return (World->GetTimeSeconds() - LastActionTime) >= MineCooldown;
}

void ACXPawn::PlaceMine(const FInputActionValue& Value)
{
	if (CanPlaceMine(LastMineRequestTime) == false)
	{
		return;
	}

	LastMineRequestTime = GetWorld()->GetTimeSeconds();
	ServerPlaceMine();
}

void ACXPawn::ServerPlaceMine_Implementation()
{
	if (CanPlaceMine(LastMineServerTime) == false)
	{
		return;
	}

	// 동시 유지 지뢰 수 제한
	ActiveMines.RemoveAll([](const TWeakObjectPtr<ACXMine>& Mine) { return Mine.IsValid() == false; });
	if (ActiveMines.Num() >= MaxActiveMines)
	{
		return;
	}

	LastMineServerTime = GetWorld()->GetTimeSeconds();
	SpawnMine();
}

void ACXPawn::SpawnMine()
{
	if (HasAuthority() == false || MineClass == nullptr || IsValid(MineSpawnPoint) == false)
	{
		return;
	}

	UWorld* World = GetWorld();

	// 차량 뒤쪽 지점에서 바닥으로 트레이스 → 지면 위에 설치
	FVector SpawnLocation = MineSpawnPoint->GetComponentLocation();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CXMineGround), false, this);
	FHitResult GroundHit;
	if (World->LineTraceSingleByChannel(GroundHit, SpawnLocation + FVector(0.0f, 0.0f, 100.0f), SpawnLocation - FVector(0.0f, 0.0f, 500.0f), ECC_WorldStatic, QueryParams))
	{
		SpawnLocation = GroundHit.ImpactPoint;
	}

	const FRotator SpawnRotation(0.0f, GetActorRotation().Yaw, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;		// 지뢰의 Owner = 설치한 차량
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACXMine* NewMine = World->SpawnActor<ACXMine>(MineClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (IsValid(NewMine))
	{
		NewMine->SetOwner(this);
		ActiveMines.Add(NewMine);
	}
}

// ============================================================
// Accelerator break (지뢰 피격)
// ============================================================

void ACXPawn::BreakAccelerator(float Duration)
{
	if (HasAuthority() == false)
	{
		return;
	}

	bAcceleratorBroken = true;
	ForceNetUpdate();
	OnRep_AcceleratorBroken(); // 리슨 서버 호스트 차량용

	// 이미 고장 상태에서 또 밟으면 타이머 갱신
	GetWorldTimerManager().SetTimer(AcceleratorBreakTimerHandle, this, &ThisClass::RepairAccelerator, FMath::Max(Duration, 0.01f), false);
}

void ACXPawn::RepairAccelerator()
{
	bAcceleratorBroken = false;
	ForceNetUpdate();
	OnRep_AcceleratorBroken();
}

void ACXPawn::OnRep_AcceleratorBroken()
{
	// 고장 즉시 전진 가속 입력 0
	if (bAcceleratorBroken && ChaosVehicleMovement)
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
	}

	UE_LOG(LogTemplateVehicle, Log, TEXT("[%s] Accelerator %s"), *GetName(), bAcceleratorBroken ? TEXT("BROKEN") : TEXT("REPAIRED"));
}

#undef LOCTEXT_NAMESPACE
