// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "ProceduralMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "bellhop.h"
#include "FollowRays.generated.h"

UCLASS()
class BELLHOP_API AFollowRays : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFollowRays();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Bellhop Acoustic Library")
		USceneComponent* SphereRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
		UStaticMeshComponent* SphereVisual;

	UPROPERTY(EditAnywhere, Category = "Bellhop Acoustic Library")
		UParticleSystemComponent* SphereParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
		UProceduralMeshComponent* RayMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
		UMaterialParameterCollection* WorldParameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
		UNiagaraSystem* Particle;

	UPROPERTY(EditAnywhere, Category = "Bellhop Acoustic Library")
		int RayID;	
	UPROPERTY(BlueprintReadWrite, Category = "Bellhop Acoustic Library")
		int NRays;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bellhop Acoustic Library")
		TArray<FVector2D> Vector2Ds;	
	UPROPERTY()
		TArray<FVector> Vertices;
	UPROPERTY()
		TArray<FVector> MeshVertices;	
	UPROPERTY()
		TArray<FVector> Normals;
	UPROPERTY()
		TArray<int32> Triangles;	
	UPROPERTY()
		TArray<FVector2D> UVs;	
	UPROPERTY()
		TArray<FLinearColor> VertexColors;
	UPROPERTY()
		TArray<FProcMeshTangent> Tangents;
	UPROPERTY()
		TArray<FProcMeshSection> ProcMeshSections;	
	UPROPERTY()
		UNiagaraComponent* NiagaraComp;

	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetVector2Ds(const int& InRayID);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void CreateStructure(const int& InRayID, UMaterial* Mat, const float& StructureRadius);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void SetUnitConversion(const double& InUnitConversion);
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		double GetUnitConversion() const;
	UFUNCTION(BlueprintCallable, Category = "Bellhop Acoustic Library")
		void AttachParticles();

	void SetRayID(const int& InRayID);
	void SetAngle(const float& InAngle);
	void SetRayAndAngle(const int& InRayID, const float& InAngle);

private:
	//helpers
	void Init();
	void GenerateMesh(const TArray<FVector4>& Nodes, const float& Radius = 10.0f);

	void UpdateMeshToPoint(const FVector& End);

private:
	//members
	int PointID = 0;
	float TanAngle = 0.0f;
	double UnitConversion = 1.0;
	bool RandomizeRay = true;
	bool Initialized = false;
	double RayMaxArrivalTime;
	FbellhopModule* Bellhop;

	FVector LastEnd{ 0.0, 0.0, 0.0 };
};
