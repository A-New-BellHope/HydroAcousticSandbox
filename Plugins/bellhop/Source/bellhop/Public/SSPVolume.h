// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "TextureResource.h"
#include "Engine/Texture2d.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "bellhop.h"
#include "SSPVolume.generated.h"

UCLASS()
class BELLHOP_API ASSPVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASSPVolume();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
	void MunkProfile();

	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
	void SoundSpeedAtPoint(const FVector& x, float& SoundSpeed) const;

	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
	void UpdateSSPTexture(UMaterialInstanceDynamic* Material,
		const int& NumX, const int& NumY, const int& NumZx, const int& NumZy);
	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
	void UpdateSSPTextureBounded(UMaterialInstanceDynamic* Material,
		const int& NumX, const int& NumY, const int& NumZx, const int& NumZy,
		const FVector& LowerBound, const FVector& UpperBound);

private:
	//helpers
	UTexture2D* CreateTexture(const TArray<unsigned int>& Data,
		const int& TexWidth, const int& TexHeight) const;

	void UpdateBoundaryBox();
	void ConvertToColor(const TArray<float>& SoundSpeeds, TArray<unsigned int>& Pixels) const;

private:
	FbellhopModule* Bellhop;

	FVector MaxBound;
	FVector MinBound;
	bool bBoundaryDirty;
};
