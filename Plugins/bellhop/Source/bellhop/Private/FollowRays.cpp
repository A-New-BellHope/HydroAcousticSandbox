// Fill out your copyright notice in the Description page of Project Settings.
#include "FollowRays.h"

// Sets default values
AFollowRays::AFollowRays()
{
	Init();
}

// Called when the game starts or when spawned
void AFollowRays::BeginPlay()
{
	Super::BeginPlay();

	Bellhop = FModuleManager::GetModulePtr<FbellhopModule>("bellhop");
}

// Called every frame
void AFollowRays::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Bellhop->IsBellhopReady())
	{
		return;
	}

	float ScaleX = 1.0;
	float ScaleY = 1.0;
	float ScaleZ = 1.0;
	float ArrivalTimeSelect = 0.0f;
	UMaterialParameterCollectionInstance* pci = GetWorld()->GetParameterCollectionInstance(WorldParameters);
	if (pci)
	{
		pci->GetScalarParameterValue(FName("ScaleX"), ScaleX);
		pci->GetScalarParameterValue(FName("ScaleY"), ScaleY);
		pci->GetScalarParameterValue(FName("ScaleZ"), ScaleZ);
		pci->GetScalarParameterValue(FName("ArrivalTimeSelect"), ArrivalTimeSelect);
	}
	
	FVector4 x = Bellhop->GetRayArrival(RayID, ArrivalTimeSelect);
	if (!x.ContainsNaN()) {
		x /= UnitConversion;
		if (x[0] != LastEnd[0] || x[1] != LastEnd[1] || x[2] != LastEnd[2]) {
			LastEnd.Set(x[0], x[1], x[2]);
			//UpdateMeshToPoint(LastEnd);
		}
		x[0] *= ScaleX;
		x[1] *= ScaleY;
		x[2] *= ScaleZ;
		SphereVisual->SetWorldLocation(x);
	}
}

/// <summary>
/// End should be on the list, or it won't do anything.
/// WARNING: Specialized for the Feb 2024 demo, and not very general.
///          Not intended for long term use.
/// </summary>
/// <param name="End"></param>
void AFollowRays::UpdateMeshToPoint(const FVector& End)
{
	TArray<FVector> v = Vertices;
	bool bFound = false;
	for (auto& x : v) {
		if (!bFound && FMath::IsNearlyEqual(x[0], End[0])) {
			bFound = true;
		}
		if (bFound) {
			x = End;
		}
	}
	//0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false
	RayMesh->UpdateMeshSection_LinearColor(0, v, Normals, UVs, VertexColors, Tangents, false);
}

void AFollowRays::SetVector2Ds(const int& InRayID)
{
	RayID = InRayID;
	auto AllRayPoints = Bellhop->GetAllRayPoints(RayID);
	for (size_t i = 0; i < AllRayPoints.Num(); i++)
	{
		Vector2Ds.Add(FVector2D(AllRayPoints[i].X, AllRayPoints[i].Z));
	}
}

void AFollowRays::SetRayID(const int& InRayID)
{
	RayID = InRayID;
	GenerateMesh(Bellhop->GetAllRayPoints(RayID));
}

void AFollowRays::SetAngle(const float& InAngle)
{
	TanAngle = FMath::Tan(InAngle);
	GenerateMesh(Bellhop->GetAllRayPoints(RayID));
}

void AFollowRays::SetRayAndAngle(const int& InRayID, const float& InAngle)
{
	RayID = InRayID;
	TanAngle = FMath::Tan(InAngle);
	GenerateMesh(Bellhop->GetAllRayPoints(RayID));
}

void AFollowRays::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("Initializing a FollowRays ..."));
	PrimaryActorTick.bCanEverTick = true;
	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ParticleSystem"));
	SphereRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	RootComponent = SphereRoot;

	RayMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));

	SphereVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> SphereVisualAsset(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (SphereVisualAsset.Succeeded())
	{
		SphereVisual->SetStaticMesh(SphereVisualAsset.Object);
		SphereVisual->SetMobility(EComponentMobility::Movable);
		SphereVisual->SetupAttachment(RootComponent);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("could not find sphere starter content ... no mesh added"));
	}

	UE_LOG(LogTemp, Warning, TEXT("Done initializing a FollowRays"));
}

void AFollowRays::CreateStructure(const int& InRayID, UMaterial* Mat, const float& StructureRadius)
{
	RayID = InRayID;
	GenerateMesh(Bellhop->GetAllRayPoints(InRayID), StructureRadius);
	RayMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	RayMesh->SetMaterial(0, (UMaterialInterface*)Mat);
}

/// <summary>
/// Set a conversion on the bellhop units for position.
/// Scales everything, mostly so rendering doesn't get confused with large numbers.
/// </summary>
/// <param name="InUnitConversion">default 1, meters</param>
void AFollowRays::SetUnitConversion(const double& InUnitConversion)
{
	UnitConversion = InUnitConversion;
}
double AFollowRays::GetUnitConversion() const
{
	return UnitConversion;
}

void AFollowRays::AttachParticles()
{
	NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(Particle, SphereVisual, NAME_None, SphereVisual->GetComponentLocation(), FRotator(0.f), EAttachLocation::Type::KeepRelativeOffset, false, true);
}

void AFollowRays::GenerateMesh(const TArray<FVector4>& Nodes, const float& Radius)
{
	int TopVertex = 0;
	int BottomVertex = 1;
	int RightVertex = 2;
	int LeftVertex = 3;

	Vertices.Empty();
	Triangles.Empty();
	UVs.Empty();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		const int nStep = 4;
		const float vectorOffset = Radius / UnitConversion;
		const FVector vector = Nodes[i] / UnitConversion;

		UVs.Add(FVector2D(0, 0));
		UVs.Add(FVector2D(0, 1));
		UVs.Add(FVector2D(1, 1));
		UVs.Add(FVector2D(1, 0));

		if (i == 0)
		{
			Vertices.Add(FVector(vector.X, vector.Y, vector.Z + vectorOffset));
			Vertices.Add(FVector(vector.X, vector.Y, vector.Z - vectorOffset));
			Vertices.Add(FVector(vector.X, vector.Y + vectorOffset, vector.Z));
			Vertices.Add(FVector(vector.X, vector.Y - vectorOffset, vector.Z));
			// Create the first set of mesh vertices and connect them
			// makes a square at a 45 degree angle
			Triangles.Add(TopVertex);
			Triangles.Add(BottomVertex);
			Triangles.Add(RightVertex);

			Triangles.Add(TopVertex);
			Triangles.Add(LeftVertex);
			Triangles.Add(BottomVertex);

			TopVertex += nStep;
			BottomVertex += nStep;
			RightVertex += nStep;
			LeftVertex += nStep;
		}
		else
		{
			Vertices.Add(FVector(vector.X, vector.Y, vector.Z + vectorOffset));
			Vertices.Add(FVector(vector.X, vector.Y, vector.Z - vectorOffset));
			Vertices.Add(FVector(vector.X, vector.Y + vectorOffset, vector.Z));
			Vertices.Add(FVector(vector.X, vector.Y - vectorOffset, vector.Z));
			// Create the next set of vertices at the next given vector
			Triangles.Add(TopVertex);
			Triangles.Add(BottomVertex);
			Triangles.Add(LeftVertex);

			Triangles.Add(TopVertex);
			Triangles.Add(RightVertex);
			Triangles.Add(BottomVertex);

			// Connect the the new vertices to the last set of vertices
			Triangles.Add(TopVertex);
			Triangles.Add(LeftVertex - nStep);
			Triangles.Add(TopVertex - nStep);

			Triangles.Add(LeftVertex - nStep);
			Triangles.Add(TopVertex);
			Triangles.Add(LeftVertex);

			Triangles.Add(LeftVertex);
			Triangles.Add(BottomVertex);
			Triangles.Add(BottomVertex - nStep);

			Triangles.Add(LeftVertex);
			Triangles.Add(BottomVertex - nStep);
			Triangles.Add(LeftVertex - nStep);

			Triangles.Add(TopVertex);
			Triangles.Add(RightVertex - nStep);
			Triangles.Add(RightVertex);

			Triangles.Add(TopVertex);
			Triangles.Add(TopVertex - nStep);
			Triangles.Add(RightVertex - nStep);

			Triangles.Add(RightVertex);
			Triangles.Add(RightVertex - nStep);
			Triangles.Add(BottomVertex - nStep);

			Triangles.Add(RightVertex);
			Triangles.Add(BottomVertex - nStep);
			Triangles.Add(BottomVertex);

			TopVertex += nStep;
			BottomVertex += nStep;
			RightVertex += nStep;
			LeftVertex += nStep;
		}
	}

	Normals.Init(FVector(0.0f, 0.0f, 1.0f), Nodes.Num() * 4);
	VertexColors.Init(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f), Nodes.Num() * 4);
	Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Nodes.Num() * 4);
	if (RayMesh)
	{
		RayMesh->ClearAllMeshSections();
	}
	RayMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	RayMesh->SetCastShadow(false);
}