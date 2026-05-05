// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logging/StructuredLog.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "netcdfunreal.h"
#include <algorithm>
#include "Bathymetry.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHYCOMDoneEvent);

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
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool OpenGEBCO(FString& Filename);
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
	bool GetEarthBathymetry(const double& North, const double& East,
		const double& South, const double& West,
		double& ActualNorth, double& ActualEast,
		double& ActualSouth, double& ActualWest,
		TArray<double>& GridX, TArray<double>& GridY,
		TArray<double>& Depth) const;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	void GetEarthSoundSpeed(const double& North, const double& East,
		const double& South, const double& West, const FDateTime& Time);

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	FVector LatLongToPosition(const double& Latitude, const double& Longitude) const;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	FVector PositionToLatLong(const FVector& Position) const;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool CheckHYCOM() const;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	void GetHexGridX(TArray<double>& GridX) const { GridX.Insert(HexGridX, 0); }
	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	void GetHexGridY(TArray<double>& GridY) const { GridY.Insert(HexGridY, 0); }
	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	void GetHexDepth(TArray<double>& Depth) const { Depth.Insert(HexDepth, 0); }
	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	void GetHexSoundSpeed(TArray<double>& SoundSpeed) const {
		SoundSpeed.Insert(HexSoundSpeed, 0);
	}

	UPROPERTY(BlueprintAssignable, Category = "Bathymetry")
	FHYCOMDoneEvent OnHYCOMDoneEvent;

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool OpenTIFF(FString& Filename);

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	bool LoadTiffBathymetry(const FString& Filename);

	UFUNCTION(BlueprintCallable, Category = "Bathymetry")
	void SetTiffBounds(const double& North, const double& East,
		const double& South, const double& West);



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

	FThreadSafeBool HYCOMDone;

	//Geotiff members
	TArray<float>  TiffDepthGrid;
	int32          TiffWidth = 0;
	int32          TiffHeight = 0;

	// GeoTIFF origin in UTM meters (from ModelTiepointTag)
	double TiffOriginEasting = 371587.5;
	double TiffOriginNorthing = 4385687.5;
	double TiffPixelSize = 25.0;       // meters per pixel

	double TiffNorth = 39.6154;
	double TiffSouth = 37.7762;
	double TiffEast = -61.8695;
	double TiffWest = -64.4581;

	bool bTiffLoaded = false;

	static constexpr float TIFF_NODATA = 3.4028234663852886e+38f;

	// room for the hexahedral stuff
	TArray<double> HexGridX;
	TArray<double> HexGridY;
	TArray<double> HexDepth;
	TArray<double> HexSoundSpeed;

};
