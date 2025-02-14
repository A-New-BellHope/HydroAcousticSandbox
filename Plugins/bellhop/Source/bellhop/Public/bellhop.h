// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <iostream>
#include <fstream>
#include <variant>
#include <map>
#include <vector>

#include "Modules/ModuleManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#define BHC_DLL_IMPORT 1
#include "bhc/bhc.hpp"

class BELLHOP_API FbellhopModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	enum class TransmissionLossMode: char
	{
		Coherent = 'C',
		SemiCoherent = 'S',
		Incoherent = 'I'
	};

	enum class BeamMode: char
	{
		Hat = 'G',  //default
		RayCenteredHat = 'g',
		Gaussian = 'B'
	};

	typedef std::pair<bhc::bhcParams<false>, bhc::bhcOutputs<false, false> >
		RunType2D;
	typedef std::pair<bhc::bhcParams<true>, bhc::bhcOutputs<true, false> >
		RunTypeNx2D;
	typedef std::pair<bhc::bhcParams<true>, bhc::bhcOutputs<true, true> >
		RunType3D;

	FbellhopModule();
	~FbellhopModule() {}

public:
	//interface
	int RunBellhopFile(FString fname, bool O3D = false, bool R3D = false);
	void RunBellhop();
	void SetupDefaults(const bool& O3D = false, const bool& R3D = false);
	FString GetBellhopDirectory();
	FString GetBellhopEnvironmentDirectory();
	bool IsBellhopReady();
	bool IsBellhopSetup();
	bool IsBellhopRun() const { return rayReady; }
	void MarkBellhopRun(const bool& State);

	int GetNRays() const;
	int GetMaxPoints();

	void SetTitle(const FString& Title);
	FString GetTitle() const;

	void SetBottom(const TArray<double>& Depths,
		const TArray<double>& XGrid, const TArray<double>& YGrid = {},
		bool O3D = false);

	//TODO: deprecate these
	float GetSingleRayAltitude(const int& rayID) const;
	void SetSingleRayAltitude(const float& altitude, const int& rayID);
	float GetSingleRayAzimuth(const int& rayID) const;
	void SetSingleRayAzimuth(const float& azimuth, const int& rayID);
	// end deprecate

	void GetRayAltitudes(float& low, float& high, int& n) const;
	void SetRayAltitudes(const float& low, const float& high, const int& n);
	void GetRayAzimuths(float& low, float& high, int& n) const;
	void SetRayAzimuths(const float& low, const float& high, const int& n);

	TArray<float> GetReceiverRanges() const;
	TArray<float> GetReceiverDepths() const;
	TArray<float> GetReceiverBearings() const;
	TArray<FVector> GetReceivers() const;
	void SetReceiver(const int& rangeID, const float& range,
		const int& depthID, const float& depth,
		const int& bearingID = 0, const float& bearing = 0.0f);
	void SetReceiverRanges(const float& low, const float& high, const int& n);
	void SetReceiverDepths(const float& low, const float& high, const int& n);
	void SetReceiverBearings(const float& low, const float& high, const int& n);

	//Cycle through the points (mostly for testing)
	FVector GetNextPoint(const int& ray, const int& sample);

	float GetSourceDepth(const int& sourceID);
	void SetSourceDepth(const int& sourceID, const float& z);

	FVector GetSourcePosition(const int& sourceID);
	void SetSourcePosition(const int& sourceID, const FVector& position);

	TArray<FVector2D> GetSoundSpeedProfile();
	void GetSoundSpeedProfile(TArray<double>& Depth,
		TArray<double>& SoundSpeed,
		int& NumberX, int& NumberY, int& NumberZ);
	void SetSoundSpeedProfile(const TArray<FVector2D>& InSoundSpeedProfile);
	bool GetSoundSpeed(const FVector& Position, float& SoundSpeed);
	void Set1DSoundSpeedProfile(const TArray<FVector2D>& InSoundSpeedProfile);
	void SetHexahedralSpeedProfile(const TArray<double>& GridX,
		const TArray<double>& GridY, const TArray<double>& Depth,
		const TArray<double>& SoundSpeeds);

	TArray<FVector> GetBoundaryPoints();
	int GetBoundarySizeX();
	int GetBoundarySizeY();

	void
		GetTransmissionLoss(TArray<bhc::cpxf>& TransmissionLoss,
			int32_t& Width, int32_t& Height, int32_t& Bearings);
	void
		GetTransmissionLoss(TArray<bhc::cpxf>& TransmissionLoss,
			int32_t& Width, int32_t& Height,
			double& DepthFactor, double& RangeFactor);
	void GetCorners(const int& bearing, TArray<FVector>& corners);
	void GetCylinder(const int& radial, TArray<FVector>& vertices);
	void GetHorizontal(const int& pancake, TArray<FVector>& corners);

	void SetRayMode();
	void SetTransmissionLossMode(const TransmissionLossMode& tl);
	void SetBeamMode(const BeamMode& b);
	bool IsTransmissionLossMode();
	bool IsRayMode();

	FString GetCurrentMode();

	int GetNPoints(const int& ray);
	TArray<FVector4> GetAllRayPoints(const int& ray);

	std::map<double, FVector> GetAllRayPointArrival(const int& ray);
	FVector4 GetRayArrival(const int& ray, const int& sample);
	FVector4 GetRayArrival(const int& ray, const float& time);
	double GetMaxArrivalTime();

private:
	//helper functions
	bool CheckSourceDepthExists(const int& sourceID);
	void UpdateAllRays();
	TArray<double> CalculateArrival(const TArray<FVector4>& ray);

	std::size_t GetFieldAddr(
		int32_t isx, int32_t isy, int32_t isz, int32_t itheta, int32_t id, int32_t ir,
		const bhc::Position* Pos);

	void UpdateBoundary3D(bhc::BdryInfoTopBot<true>& Boundary,
		const TArray<double>& Depth,
		const TArray<double>& GridX, const TArray<double>& GridY) const;

	void ResetBellhopInitialization();

private:
	void* BellhopLibraryHandle;
	FString lastFileName;

	bhc::bhcInit BellhopInitializaiton;

	std::variant<RunType2D, RunTypeNx2D, RunType3D> params;

	TArray< TArray<FVector4> > AllRays;

	TArray< std::map<double, FVector> > AllRayArrivals;

	double MaxArrivalTime;

	
public:

	//flags
	bool recalculateRays;
	bool working;
	bool rayReady;
	bool IsSetup = false;
};
