// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "netcdfunreal.h"
#include "SaveHycom.generated.h"

/**
 * 
 */
USTRUCT()
struct FHycomSaveData
{
	GENERATED_BODY()

public:
	
	std::string HycomURL;

	UPROPERTY(VisibleAnywhere, Category = Saving)
	TArray<double> SoundSpeedCache;
};

UCLASS()
class NETCDFUNREAL_API USaveHycom : public USaveGame
{
	GENERATED_BODY()

	USaveHycom();

public:
	UPROPERTY(VisibleAnywhere, Category = Saving)
	FHycomSaveData SaveData;

	UPROPERTY(VisibleAnywhere, Category = Saving)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Saving)
	uint32 UserIndex;
};
