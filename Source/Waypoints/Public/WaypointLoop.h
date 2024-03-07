// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "WaypointLoop.generated.h"

class AWaypoint;
class USceneComponent;

UCLASS()
class WAYPOINTS_API AWaypointLoop : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
		USceneComponent* Scene;

	UPROPERTY(EditInstanceOnly, Category="Waypoint Loop")
		TArray<TWeakObjectPtr<AWaypoint>> Waypoints;

	UPROPERTY()
		bool bSplineColorSetup;

	UPROPERTY(EditInstanceOnly, Category = "Waypoint Loop")
		FLinearColor SplineColor;

	void AddWaypoint(AWaypoint* NewWaypoint);
	void InsertWaypoint(AWaypoint* NewWaypoint, int32 Index);
	void RemoveWaypoint(const AWaypoint* Waypoint);
	int32 FindWaypoint(const AWaypoint* Elem) const;

	void RecalculateAllWaypoints();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	virtual void PostLoad() override;
#endif // WITH_EDITOR
};
