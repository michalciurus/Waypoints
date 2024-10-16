// Copyright 2020 Nicholas Chalkley. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "NavigationSystem.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Waypoint.generated.h"

class AWaypointLoop;
class ANavigationData;

UCLASS(Blueprintable, BlueprintType, meta=(PrioritizeCategories="Waypoint"))
class WAYPOINTS_API AWaypoint : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PostRegisterAllComponents() override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual bool CanDeleteSelectedActor(FText& OutReason) const override { return true; };
#endif // WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Waypoint")
		AWaypoint* GetNextWaypoint() const;
	
	UPROPERTY(EditInstanceOnly, Category="Waypoint")
	TWeakObjectPtr<AWaypointLoop> OwningLoop;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		AWaypoint* GetPreviousWaypoint() const;

	UFUNCTION()
		TArray<TWeakObjectPtr<AWaypoint>> GetLoop() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		float GetWaitTime() const { return WaitTime; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		bool GetOrientGuardToWaypoint() const { return bOrientGuardToWaypoint; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		bool GetStopOnOverlap() const { return bStopOnOverlap; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		float GetAcceptanceRadius() const { return AcceptanceRadius; };

	void CalculateSpline();

	void RecalculateIndex();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Waypoint")
		int32 WaypointIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waypointz")
		class USceneComponent* Scene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waypoint")
		class USphereComponent* OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waypoint")
		class UBillboardComponent* Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waypoint")
		class USplineComponent* PathComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Waypoint")
		class UArrowComponent* GuardFacingArrow;


	const ANavigationData* GetNavData() const;
	const FNavAgentProperties& GetNavAgentProperties() const;

protected:
	UFUNCTION(CallInEditor)
		void OnNavigationGenerationFinished(class ANavigationData* NavData);

	UFUNCTION(CallInEditor, Category = "Waypoint")
		void SelectNextWaypoint() const;

	UFUNCTION(CallInEditor, Category = "Waypoint")
		void CreateWaypointLoop();

	UPROPERTY(EditAnywhere, Category = "Waypoint")
		bool bUseCharacterClassNavProperties;

	UPROPERTY(EditDefaultsOnly, Category = "Waypoint", Meta=(EditCondition="!bUseCharacterClassNavProperties"))
		FNavAgentProperties NavProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (ClampMin = "0.0", UIMin = "0.0"))
		float WaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
		bool bOrientGuardToWaypoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
		bool bStopOnOverlap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (ClampMin = "-1.0", UIMin = "-1.0"))
		float AcceptanceRadius;

	// Character class used for navigation data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
		TSubclassOf<ACharacter> CharacterClass;

	void SetWaypointLoop(AWaypointLoop* Loop);
};
