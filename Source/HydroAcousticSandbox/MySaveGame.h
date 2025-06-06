// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MySaveGame.generated.h"

/**
 * 
 */
UCLASS()
class HYDROACOUSTICSANDBOX_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = Saving)
	FString HycomURL;

	UPROPERTY(VisibleAnywhere, Category = Saving)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Saving)
	uint32 UserIndex;
};
