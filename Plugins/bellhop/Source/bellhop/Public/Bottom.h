// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Atomic.h"
#include "GameFramework/Actor.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/Platform.h"
#include "HAL/PlatformFileManager.h"
#include "bellhop.h"
#include "Bottom.generated.h"

UCLASS()
class BELLHOP_API ABottom : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABottom();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
		void ReadFile(const FString& BellhopRoot, const bool& O3D, const bool& R3D);

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		TArray<FVector> GetBottomMeshVertices();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		TArray<FVector2D> GetHeightMapUV();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void GetBottomBoundary(FVector& Low, FVector& High);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		int GetNPointsX();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		int GetNPointsY();
	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
		float GetZMin() const;
	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
		float GetZMax() const;

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool UpdateBottom(const TArray<double>& Depths,
		const TArray<double>& GridX, const TArray<double>& GridY,
		const bool& O3D = false);

private:

	FbellhopModule* Bellhop;

	bool CheckFileExists(const FString& FileName);

	void UpdateBottomVertices();
	void UpdateHeightMapUV();
	bool bBottomDirty = true;
	TArray<FVector> BottomVertices;
	TArray<FVector2D> HeightMapUV;

	double ZMin = -5389.06;
	double ZMax = 3969.89;
};
