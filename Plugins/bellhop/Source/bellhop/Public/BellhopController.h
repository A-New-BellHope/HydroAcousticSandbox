// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformFileCommon.h"
#include "bellhop.h"
#include "BellhopController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnvfileDoneEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBellhopDoneEvent);

UENUM(BlueprintType, Category = "Bellhop Acoustic Ray Trace")
enum class ETransmissionLossMode : uint8
{
	Coherent UMETA(DisplayName = "C"),
	Incoherent UMETA(DisplayName = "I"),
	SemiCoherent UMETA(DisplayName = "S")
};

UCLASS(BlueprintType, Category = "Bellhop Acoustic Ray Trace")
class BELLHOP_API ABellhopController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABellhopController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Interface
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	void SetupDefaults(const bool& O3D, const bool& R3D);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	void RunBellhop();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	void BackgroundRunBellhop();
	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
		void ReadFile(const FString& BellhopRoot, const bool& O3D, const bool& R3D);
	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
		FString GetPluginDefaultDirectory();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		int GetNPoints(const int& InRayID) const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		int GetNRays() const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void AdjustSoundSpeedProfile(const int& Depth, const float& SoundSpeed);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void TestingFlattenSoundSpeedProfile();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void MunkProfile();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void MoveSource(const int& Source, const float& X);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void MoveSource3D(const int& Source, const FVector& position);

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void GetRayAzimuths(float& low, float& high, int& n) const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void GetRayAltitudes(float& low, float& high, int& n) const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetRayAzimuths(const float& low, const float& high, const int& n);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetRayAltitudes(const float& low, const float& high, const int& n);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetAzimuthCount(const int& n);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetAltitudeCount(const int& n);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetStepSize(const double& StepSize);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		double GetStepSize() const;


	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void GetReceiverBearings(TArray<float>& ReceiverBearings) const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void GetReceiverDepths(TArray<float>& ReceiverDepths) const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void GetReceiverRanges(TArray<float>& ReceiverRanges) const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetReceiverBearings(const float& low, const float& high, const int& n);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetReceiverDepths(const float& low, const float& high, const int& n);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetReceiverRanges(const float& low, const float& high, const int& n);

	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
		float GetMaxArrivalTime();
	UFUNCTION(BluePrintCallable, Category = "Bellhop Acoustic Library")
		FVector GetSourceLocation(const int& SourceID);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool GetSoundSpeedProfile(TArray<double>& Depth,
		TArray<double>& SoundSpeed,
		int& NumberX, int& NumberY, int& NumberZ) const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool SetSingleSoundSpeed(const TArray<double>& Depth,
		const TArray<double>& SoundSpeed);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool SetHexahedralSoundSpeed(
		const TArray<double>& GridX, const TArray<double>& GridY,
		const TArray<double>& Depth, const TArray<double>& SoundSpeed);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool GetSingleSoundSpeed(const FVector &Depth, float& SoundSpeed);

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool IsRayMode() const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool IsTransmissionLossMode() const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	void SetRayMode();
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	void SetTransmissionLossMode(ETransmissionLossMode tl);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	int GetPercentDone();

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool CheckSource(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	void WriteBellhopEnvironment(FString BaseName, FString Directory);

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
	bool IsEnvfileRunning() const;
	UPROPERTY(BlueprintAssignable, Category = "Bellhop Acoustic Library")
	FEnvfileDoneEvent OnEnvfileDoneEvent;
	UPROPERTY(BlueprintAssignable, Category = "Bellhop Acoustic Library")
	FBellhopDoneEvent OnBellhopDoneEvent;

	void BackgroundRunComplete();

private:
	//helpers
	void Init();
	bool CheckFileExists(const FString& FileName);

private:
	//members

	FbellhopModule* Bellhop;

	FThreadSafeBool EnvfileRunning;
	FThreadSafeBool BellhopRunning;
	FThreadSafeBool BellhopDone;
	FString EnvFileName;
};
