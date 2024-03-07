// Copyright 2020 EpicGameGuy. All Rights Reserved.

#include "Waypoint.h"
#include "Components/SceneComponent.h"
#include "WaypointLoop.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/BillboardComponent.h"
#include "Components/SplineComponent.h"

#include "Editor/UnrealEdEngine.h"
#include "Engine/Selection.h"
#endif // WITH_EDITOR

#include "GameFramework/Character.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

static bool IsCorrectWorldType(const UWorld* World)
{
	return World->WorldType == EWorldType::Editor;
}

AWaypoint::AWaypoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(Scene);

	AcceptanceRadius = 128.f;
	WaitTime = 0.f;

	bStopOnOverlap = true;
	bOrientGuardToWaypoint = false;

	WaypointIndex = INDEX_NONE;

#if WITH_EDITOR
	bRunConstructionScriptOnDrag = false;

	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> WaypointIcon;

		FName ID_WaypointIcon;

		FText NAME_WaypointIcon;

		FConstructorStatics()
			: WaypointIcon(TEXT("/Waypoints/EditorResources/S_Waypoint"))
			, ID_WaypointIcon(TEXT("WaypointIcon"))
			, NAME_WaypointIcon(NSLOCTEXT("SpriteCategory", "Waypoints", "WaypointIcon"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	if (Sprite)
	{

		Sprite->Sprite = ConstructorStatics.WaypointIcon.Get();
		Sprite->SpriteInfo.Category = ConstructorStatics.ID_WaypointIcon;
		Sprite->SpriteInfo.DisplayName = ConstructorStatics.NAME_WaypointIcon;
		Sprite->SetupAttachment(Scene);
	}

	PathComponent = CreateDefaultSubobject<USplineComponent>(TEXT("PathRenderComponent"));
	if (PathComponent)
	{
		PathComponent->SetupAttachment(Scene);
		PathComponent->bSelectable = false;
		PathComponent->bEditableWhenInherited = true;
		PathComponent->SetUsingAbsoluteLocation(true);
		PathComponent->SetUsingAbsoluteRotation(true);
		PathComponent->SetUsingAbsoluteScale(true);
	}
#endif // WITH_EDITOR

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Overlap Sphere Visualization Component"));
	if (OverlapSphere)
	{
		OverlapSphere->SetupAttachment(Scene);
		OverlapSphere->bSelectable = true;
		OverlapSphere->bEditableWhenInherited = true;
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap);
		OverlapSphere->SetSphereRadius(AcceptanceRadius);
	}

	GuardFacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Guard Facing Arrow Component"));
	if (GuardFacingArrow)
	{
		GuardFacingArrow->SetupAttachment(Scene);
		GuardFacingArrow->bSelectable = true;
		GuardFacingArrow->bEditableWhenInherited = false;
		GuardFacingArrow->SetVisibility(false);
		GuardFacingArrow->ArrowColor = FColor(255, 255, 255, 255);
	}

	bUseCharacterClassNavProperties = true;
	CharacterClass = ACharacter::StaticClass();
}

TArray<TWeakObjectPtr<AWaypoint>> AWaypoint::GetLoop() const
{
	if (OwningLoop.IsValid())
	{
		return OwningLoop->Waypoints;
	}

	return {};
}

AWaypoint* AWaypoint::GetNextWaypoint() const
{
	if (OwningLoop.IsValid())
	{
		const TArray<TWeakObjectPtr<AWaypoint>>& WaypointLoop = OwningLoop->Waypoints;
		auto Index = OwningLoop->FindWaypoint(this);
		if (Index != INDEX_NONE)
		{
			return WaypointLoop[(Index + 1) % WaypointLoop.Num()].Get();
		}
	}

	return nullptr;
}

AWaypoint* AWaypoint::GetPreviousWaypoint() const
{
	if (OwningLoop.IsValid())
	{
		const TArray<TWeakObjectPtr<AWaypoint>>& WaypointLoop = OwningLoop->Waypoints;
		auto Index = OwningLoop->FindWaypoint(this);
		if (Index != INDEX_NONE)
		{
			int32 WrappedIndex = (Index - 1) % WaypointLoop.Num();
			if (WrappedIndex < 0)
			{
				WrappedIndex += WaypointLoop.Num();
			}

			return WaypointLoop[WrappedIndex].Get();
		}
	}

	return nullptr;
}


void AWaypoint::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (World && World->WorldType == EWorldType::Editor)
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			NavSys->OnNavigationGenerationFinishedDelegate.RemoveAll(this);
			NavSys->OnNavigationGenerationFinishedDelegate.AddUniqueDynamic(this, &AWaypoint::OnNavigationGenerationFinished);
		}
	}
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void AWaypoint::PreEditChange(FProperty* PropertyThatWillChange)
{
	static const FName NAME_OwningLoop = GET_MEMBER_NAME_CHECKED(AWaypoint, OwningLoop);

	if (PropertyThatWillChange)
	{
		const FName ChangedPropName = PropertyThatWillChange->GetFName();
		if (ChangedPropName == NAME_OwningLoop)
		{
			SetWaypointLoop(nullptr);
		}
	}

	Super::PreEditChange(PropertyThatWillChange);
}

void AWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_bOrientGuardToWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, bOrientGuardToWaypoint);
	static const FName NAME_AcceptanceRadius = GET_MEMBER_NAME_CHECKED(AWaypoint, AcceptanceRadius);
	static const FName NAME_OwningLoop = GET_MEMBER_NAME_CHECKED(AWaypoint, OwningLoop);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		const FName ChangedPropName = PropertyChangedEvent.Property->GetFName();

		if (ChangedPropName == NAME_bOrientGuardToWaypoint && GuardFacingArrow)
		{
			GuardFacingArrow->SetVisibility(bOrientGuardToWaypoint);
		}

		if (ChangedPropName == NAME_AcceptanceRadius)
		{
			OverlapSphere->SetSphereRadius(AcceptanceRadius);
		}

		if (ChangedPropName == NAME_OwningLoop)
		{
			SetWaypointLoop(OwningLoop.Get());
		}
	}
}

void AWaypoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	CalculateSpline();

	AWaypoint* PreviousWaypoint = GetPreviousWaypoint();
	if (PreviousWaypoint && PreviousWaypoint != this)
	{
		PreviousWaypoint->CalculateSpline();
	}
}
#endif // WITH_EDITOR

void AWaypoint::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

#if WITH_EDITOR
	if (DuplicateMode != EDuplicateMode::Normal)
		return;

	if (OwningLoop.IsValid() && OwningLoop->Waypoints.IsValidIndex(WaypointIndex))
	{
		OwningLoop->InsertWaypoint(this, WaypointIndex + 1);
		RecalculateIndex();
	}
#endif
}

void AWaypoint::CalculateSpline()
{
#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::Editor)
		return;

	AWaypoint* NextWaypoint = GetNextWaypoint();
	if (NextWaypoint && NextWaypoint != this)
	{
		PathComponent->SetVisibility(true);
		PathComponent->EditorUnselectedSplineSegmentColor = OwningLoop->SplineColor;

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FPathFindingQuery NavParams;
			NavParams.StartLocation = GetActorLocation();
			NavParams.EndLocation = NextWaypoint->GetActorLocation();
			NavParams.NavData = GetNavData();
			if (NavParams.NavData != nullptr)
			{
				NavParams.QueryFilter = NavParams.NavData->GetDefaultQueryFilter();
			}
			NavParams.SetNavAgentProperties(GetNavAgentProperties());

			FNavPathQueryDelegate Delegate;
			Delegate.BindLambda([WeakThis = TWeakObjectPtr<ThisClass>(this)](uint32 aPathId, ENavigationQueryResult::Type, FNavPathSharedPtr NavPointer)
				{
					// Since this lambda is async it can be called after the object was deleted
					if (!NavPointer.IsValid() || !WeakThis.IsValid() || !WeakThis->PathComponent->IsValidLowLevel())
						return;

					TArray<FVector> SplinePoints;
					for (const FNavPathPoint& NavPoint : NavPointer->GetPathPoints())
					{
						SplinePoints.Push(NavPoint.Location + FVector(0.f, 0.f, 128.f));
					}

					WeakThis->PathComponent->SetSplineWorldPoints(SplinePoints);
					if (SplinePoints.Num() > 1)
					{
						for (int32 i = 0; i < SplinePoints.Num(); ++i)
						{
							WeakThis->PathComponent->SetTangentsAtSplinePoint(i, FVector(0.f, 0.f, 0.f), FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::World);
						}
					}
				});
			NavSys->FindPathAsync(GetNavAgentProperties(), NavParams, Delegate);
		}
	}
	else
	{
		PathComponent->SetSplineWorldPoints(TArray<FVector>());
	}
#endif // WITH_EDITOR
}

void AWaypoint::RecalculateIndex()
{
	if (OwningLoop.IsValid())
	{
		WaypointIndex = OwningLoop->FindWaypoint(this);
	}
}

const ANavigationData* AWaypoint::GetNavData() const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys == nullptr)
	{
		return nullptr;
	}

	if (CharacterClass)
	{
		const ACharacter* CharacterCDO = CharacterClass->GetDefaultObject<ACharacter>();
		return NavSys->GetNavDataForProps(CharacterCDO->GetNavAgentPropertiesRef());
	}
	else
	{
		return NavSys->GetAbstractNavData();
	}
}

const FNavAgentProperties& AWaypoint::GetNavAgentProperties() const
{
	if (bUseCharacterClassNavProperties && CharacterClass)
	{
		const ACharacter* CharacterCDO = CharacterClass->GetDefaultObject<ACharacter>();
		static FNavAgentProperties NavAgentProps;
		NavAgentProps.NavWalkingSearchHeightScale = FNavigationSystem::GetDefaultSupportedAgent().NavWalkingSearchHeightScale;

		NavAgentProps.AgentRadius = CharacterCDO->GetCapsuleComponent()->GetScaledCapsuleRadius();
		NavAgentProps.AgentHeight = CharacterCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f;
		return NavAgentProps;
	}
	return NavProperties;
}

void AWaypoint::OnNavigationGenerationFinished(class ANavigationData* NavData)
{
	CalculateSpline();
}

void AWaypoint::Destroyed()
{
	if (OwningLoop.IsValid())
	{
		OwningLoop->RemoveWaypoint(this);
		OwningLoop = nullptr;
	}

	Super::Destroyed();
}

void AWaypoint::SelectNextWaypoint() const
{
#if WITH_EDITOR
	USelection* Selection = GEditor->GetSelectedActors();

	if (AWaypoint* NextWaypoint = GetNextWaypoint())
	{
		Selection->DeselectAll();
		Selection->Select(NextWaypoint);
	}
#endif // WITH_EDITOR
}

void AWaypoint::CreateWaypointLoop()
{
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		//UE_LOG(LogWaypoints, Warning, TEXT("Spawning new waypoint loop!!!!!"));
		FActorSpawnParameters Params;
		Params.bAllowDuringConstructionScript = true;
		AWaypointLoop* NewOwningLoop = World->SpawnActor<AWaypointLoop>(AWaypointLoop::StaticClass(), Params);

		SetWaypointLoop(NewOwningLoop);
	}
#endif // WITH_EDITOR
}

void AWaypoint::SetWaypointLoop(AWaypointLoop* Loop)
{
	if (OwningLoop.IsValid())
	{
		OwningLoop->RemoveWaypoint(this);

		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		WaypointIndex = INDEX_NONE;
	}

	OwningLoop = Loop;

	if (OwningLoop.IsValid())
	{
		AttachToActor(OwningLoop.Get(), FAttachmentTransformRules::KeepWorldTransform);

		OwningLoop->AddWaypoint(this);
		WaypointIndex = OwningLoop->FindWaypoint(this);
	}
}
