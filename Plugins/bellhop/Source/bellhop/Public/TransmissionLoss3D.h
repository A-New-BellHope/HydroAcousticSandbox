// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "bellhop.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformFileManager.h"
#include "tinycolormap.hpp"
#include <map>
#include <fstream>
#include "TransmissionLoss3D.generated.h"

UCLASS(BlueprintType, Category = "Bellhop Acoustic Ray Trace")
class BELLHOP_API ATransmissionLoss3D : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATransmissionLoss3D();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BluePrintCallable)
	void UpdateTransmissionLoss();

	UFUNCTION(BluePrintCallable) int GetNumBearings();
	UFUNCTION(BluePrintCallable) int GetWidth();
	UFUNCTION(BluePrintCallable) int GetHeight();

	UFUNCTION(BluePrintCallable) 
	FVector GetSourcePosition(const int& SourceID) const;

	UFUNCTION(BluePrintCallable)
	TArray<FVector> GetCorners(const int& Bearing);
	UFUNCTION(BlueprintCallable)
	TArray<FVector> GetCylinder(const int& Radial);
	UFUNCTION(BlueprintCallable)
	TArray<FVector> GetHorizontal(const int& Pancake);

	UFUNCTION(BluePrintCallable)
	void UpdateTextureBearing(UMaterialInstanceDynamic* Material, const int& Bearing);
	UFUNCTION(BluePrintCallable)
	void UpdateTextureCylinder(UMaterialInstanceDynamic* Material, const int& Radial);
	UFUNCTION(BluePrintCallable)
	void UpdateTextureHorizontal(UMaterialInstanceDynamic* Material, const int& Horizontal);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
	TArray<float> AllWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
	TArray<float> AllHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
	TArray<float> AllBearings;

	unsigned int ColorMap(const float& Value, const float& Min, const float& Max) const;

	float ToReal(const bhc::cpxf& x) const;

private:
	//helpers
	UTexture2D* CreateTexture(const TArray<unsigned int>& Data,
		const int& TexWidth, const int& TexHeight) const;
	void UpdateLimits();

	bool CheckFileExists(const FString& FileName);

private:
	FbellhopModule* Bellhop;

	TArray<bhc::cpxf> TransmissionLoss;
	int Width = 0;
	int Height = 0;
	int Bearings = 0;

	float MinValue = 0;
	float MaxValue = 0;

	std::map<FString, tinycolormap::ColormapType> ColormapNames;
	FString Colormap = "Jet";

};
