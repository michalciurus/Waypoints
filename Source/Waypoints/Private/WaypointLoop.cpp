// Fill out your copyright notice in the Description page of Project Settings.


#include "WaypointLoop.h"
#include "Waypoint.h"
#include "Components/SceneComponent.h"
#include "Internationalization/TextLocalizationResource.h"

// Sets default values
AWaypointLoop::AWaypointLoop(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(Scene);
	bSplineColorSetup = false;
}

#if WITH_EDITOR
void AWaypointLoop::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);


	// pupsko
	// duże pupsko

	static const FName NAME_Waypoints = GET_MEMBER_NAME_CHECKED(AWaypointLoop, Waypoints);
	static const FName NAME_SplineColor = GET_MEMBER_NAME_CHECKED(AWaypointLoop, SplineColor);

	if (Event.Property)
	{
		const FName ChangedPropName = Event.Property->GetFName();

		if (ChangedPropName == NAME_Waypoints)
		{
			RecalculateAllWaypoints();
		}

		if (ChangedPropName == NAME_SplineColor)
		{
			bSplineColorSetup = true;
			RecalculateAllWaypoints();
		}
	}
}

void AWaypointLoop::PostLoad()
{
	Super::PostLoad();

	if (!bSplineColorSetup)
	{
		FRandomStream Stream(FTextLocalizationResource::HashString(GetName(), 0));
		SplineColor = FLinearColor::MakeFromHSV8((uint8)(Stream.GetUnsignedInt() % 256), 255, 255);
		bSplineColorSetup = true;
	}

	RecalculateAllWaypoints();
}
#endif // WITH_EDITOR

void AWaypointLoop::AddWaypoint(AWaypoint* NewWaypoint)
{
	check(!Waypoints.Contains(TWeakObjectPtr<AWaypoint>(NewWaypoint)));

	Waypoints.Push(TWeakObjectPtr<AWaypoint>(NewWaypoint));

	RecalculateAllWaypoints();
}

void AWaypointLoop::InsertWaypoint(AWaypoint* NewWaypoint, int32 Index)
{
	check(!Waypoints.Contains(TWeakObjectPtr<AWaypoint>(NewWaypoint)));

	Waypoints.Insert(NewWaypoint, Index);

	RecalculateAllWaypoints();
}

void AWaypointLoop::RemoveWaypoint(const AWaypoint* Waypoint)
{
	// Remove the last matching element
	for (int32 i = Waypoints.Num() - 1; i >= 0; --i)
	{
		if (Waypoints[i].Get() == Waypoint)
		{
			Waypoints.RemoveAt(i);

			// Tell the previous waypoint to recalculate its spline
			if (Waypoints.Num() != 0)
			{
				int32 WrappedIndex = (i - 1) % Waypoints.Num();
				if (WrappedIndex < 0)
				{
					WrappedIndex += Waypoints.Num();
				}

				Waypoints[WrappedIndex]->CalculateSpline();
			}

			break;
		}
	}

	// Destroy this waypoint loop if there's no waypoints
	if (Waypoints.Num() == 0)
	{
		Destroy();
	}
	else
	{
		RecalculateAllWaypoints();
	}
}

int32 AWaypointLoop::FindWaypoint(const AWaypoint* Elem) const
{
	for (int32 i = 0; i < Waypoints.Num(); ++i)
	{
		if (Waypoints[i].Get() == Elem)
		{
			return i;
		}
	}
	
	return INDEX_NONE;
}

AWaypoint* AWaypointLoop::GetClosestWaypoint(const FVector& Location)
{
	float MinDistance = TNumericLimits<float>::Max();
	AWaypoint* ClosestWaypoint = nullptr;

	for (int32 i = 0; i < Waypoints.Num(); ++i)
	{
		if (Waypoints[i].IsValid())
		{
			float Distance = FVector::DistSquared(Waypoints[i]->GetActorLocation(), Location);
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestWaypoint = Waypoints[i].Get();
			}
		}
	}

	return ClosestWaypoint;
}

void AWaypointLoop::RecalculateAllWaypoints()
{
	// Recalculate all indicies
	for (int32 i = Waypoints.Num() - 1; i >= 0; --i)
	{
		if (Waypoints[i].IsValid())
		{
			Waypoints[i]->RecalculateIndex();
		}
	}

	// Recalculate splines
	for (int32 i = Waypoints.Num() - 1; i >= 0; --i)
	{
		if (Waypoints[i].IsValid())
		{
			Waypoints[i]->CalculateSpline();
		}
	}
}
