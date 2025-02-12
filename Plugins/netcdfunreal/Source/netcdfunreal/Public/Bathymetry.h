// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logging/StructuredLog.h"
#include "netcdfunreal.h"
#include <algorithm>
#include "Bathymetry.generated.h"

UCLASS(BlueprintType, Category = "Bathymetry")
class NETCDFUNREAL_API ABathymetry : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABathymetry();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool LoadEarthFile(const FString& Filename);

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool Longitude(TArray<double>& Longitude) const;
	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool Latitude(TArray<double>& Latitude) const;

	UFUNCTION(BluePrintCallable, Category = "Bathymetry")
	bool SetOrigin(const double& InOriginLatitude,
		const double& InOriginLongitude);

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool GetEarthBathymetry(const float& North, const float& East,
		const float& South, const float& West,
		TArray<double>& GridX, TArray<double>& GridY,
		TArray<double>& Depth) const;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool GetEarthSoundSpeed(const float& North, const float& East,
		const float& South, const float& West, const FDateTime& Time,
		TArray<double>& GridX, TArray<double>& GridY,
		TArray<double>& Depth, TArray<double>& SoundSpeed) const;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	FVector LatLongToPosition(const float& Latitude, const float& Longitude) const;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	FVector PositionToLatLong(const FVector& Position) const;

private:
	//helpers
	void Init();

	double Distance(double latitud1, double longitud1,
		double latitud2, double longitud2) const;

	void ErrorMessage(const FString& Message) const;

private:
	//members

	FnetcdfunrealModule* EarthBathymetry;

	double OriginLatitude;
	double OriginLongitude;

	const double EarthRadius = 6372797.56085;

};
