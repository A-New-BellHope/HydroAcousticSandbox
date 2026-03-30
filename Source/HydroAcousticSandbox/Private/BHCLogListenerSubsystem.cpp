// Fill out your copyright notice in the Description page of Project Settings.


#include "BHCLogListenerSubsystem.h"

void FBHCOutputDevice::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
    if (!Subsystem) return;

    // Filter out very verbose logs if needed
    if (Verbosity > ELogVerbosity::Display) return;

    // Determine color based on verbosity
    FColor LogColor = FColor::White;
    switch (Verbosity)
    {
    case ELogVerbosity::Error:
    case ELogVerbosity::Fatal:
        LogColor = FColor::Red;
        break;
    case ELogVerbosity::Warning:
        LogColor = FColor::Yellow;
        break;
    default:
        LogColor = FColor::Cyan;
        break;
    }

    FString Message = FString::Printf(TEXT("%s: %s"), *Category.ToString(), V);

    // Broadcast to Blueprint (must be on Game Thread)
    if (IsInGameThread())
    {
        Subsystem->OnNewLogMessage.Broadcast(Message, LogColor);
    }
    else
    {
        // If log comes from another thread, force broadcast on GameThread
        FString LocalMessage = Message;
        FColor LocalColor = LogColor;
        AsyncTask(ENamedThreads::GameThread, [this, LocalMessage, LocalColor]()
            {
                if (Subsystem)
                {
                    Subsystem->OnNewLogMessage.Broadcast(LocalMessage, LocalColor);
                }
            });
    }
}

void UBHCLogListenerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Create and register the log device
    LogDevice = MakeShared<FBHCOutputDevice>(this);
    if (GLog)
    {
        GLog->AddOutputDevice(LogDevice.Get());
    }
}

void UBHCLogListenerSubsystem::Deinitialize()
{
    // Clean up to prevent crashes on shutdown
    if (GLog && LogDevice.IsValid())
    {
        GLog->RemoveOutputDevice(LogDevice.Get());
    }

    Super::Deinitialize();
}