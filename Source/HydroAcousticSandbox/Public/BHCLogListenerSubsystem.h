// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BHCLogListenerSubsystem.generated.h"

// Declare a dynamic multicast delegate so Blueprints can bind to it
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNewLogMessage, const FString&, Message, FColor, Color);

/**
 * Custom Output Device to capture logs
 */
class FBHCOutputDevice : public FOutputDevice
{
public:
    FBHCOutputDevice(class UBHCLogListenerSubsystem* InSubsystem)
        : Subsystem(InSubsystem) {
    }

    virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override;

private:
    class UBHCLogListenerSubsystem* Subsystem;
};

/**
 * 
 */
UCLASS()
class HYDROACOUSTICSANDBOX_API UBHCLogListenerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Blueprint assignable event
    UPROPERTY(BlueprintAssignable, Category = "Logging")
    FOnNewLogMessage OnNewLogMessage;

private:
    TSharedPtr<FBHCOutputDevice> LogDevice;
	
};
