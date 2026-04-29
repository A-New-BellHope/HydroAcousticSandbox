// Fill out your copyright notice in the Description page of Project Settings.


#include "SSPVolume.h"

// Sets default values
ASSPVolume::ASSPVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MaxBound.Set(1, 1, 1);
	MinBound.Set(0, 0, 0);
	bBoundaryDirty = true;
}

// Called when the game starts or when spawned
void ASSPVolume::BeginPlay()
{
	Super::BeginPlay();
	
	Bellhop = FModuleManager::GetModulePtr<FbellhopModule>("bellhop");
}

// Called every frame
void ASSPVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/// <summary>
/// Set the profile to a simple 1D Munk estimate at 100m intervals.
/// Taken from oalib writeup - https://ocw.mit.edu/courses/2-s998-marine-autonomy-sensing-and-communications-spring-2012/ecfe1dec3e4f6ff085d055faa2860b39_MIT2_S998S12_Lab05.pdf.
/// Mostly for testing.
/// </summary>
void ASSPVolume::MunkProfile()
{
	UpdateBoundaryBox();

	TArray<FVector2D> Profile;

	double eps = 0.00737;  //magic number
	double zc = 1300; //the depth of the SOFAR channel
	double d = 0.0;
	double StepSize = 100.0;

	double MaxDepth = FMath::Max(MinBound[2], -20000.0);

	while (d > MaxDepth + 1.0) {
		double zt = 2 * (-d - zc) / zc;
		FVector2D x(d, 1500.0 * (1.0 + eps * (zt - 1.0 + FMath::Exp(-1.0 * zt))));
		Profile.Add(x);
		d -= StepSize;
	}
	d = MaxDepth;
	double zt = 2 * (-d - zc) / zc;
	FVector2D x(d, 1500.0 * (1.0 + eps * (zt - 1.0 + FMath::Exp(-1.0 * zt))));
	Profile.Add(x);

	Bellhop->Set1DSoundSpeedProfile(Profile);
}

void ASSPVolume::SoundSpeedAtPoint(const FVector& x, float& SoundSpeed) const
{
	Bellhop->GetSoundSpeed(x, SoundSpeed);
}

float ASSPVolume::GetDepthBelow(const FVector& x) const
{
	float ret;
	Bellhop->GetBottomDepth(x[0], x[1], ret);
	return ret;
}


void ASSPVolume::UpdateSSPTexture(UMaterialInstanceDynamic* Material,
	const int& NumX, const int& NumY, const int& NumZx, const int& NumZy)
{
	UpdateBoundaryBox();

	TArray<float> SoundSpeeds;

	int NumZ = NumZx * NumZy;
	int CountZ = 0;
	for (int n = 0; n < NumZy; ++n) {
		for (int i = 0; i < NumY; ++i) {
			for (int j = 0; j < NumZx; ++j) {
				for (int k = 0; k < NumX; ++k) {
					float ssp;
					FVector SamplePoint(
						FMath::Lerp(MinBound[0], MaxBound[0], float(k) / float(NumX)),
						FMath::Lerp(MinBound[1], MaxBound[1], float(i) / float(NumY)),
						FMath::Lerp(MinBound[2], MaxBound[2], float(CountZ) / float(NumZ))
					);
					Bellhop->GetSoundSpeed(SamplePoint, ssp);
					SoundSpeeds.Add(ssp);
				}
				++CountZ;
			}
		}
	}

	TArray<unsigned int> Pixels;
	ConvertToColor(SoundSpeeds, Pixels);

	UTexture2D* tex = CreateTexture(Pixels, NumZx*NumX, NumZy*NumY);
	if (Material)
	{
		Material->SetTextureParameterValue(FName("SoundSpeed"), (UTexture*)tex);
		UE_LOG(LogTemp, Warning, TEXT("updated ssp texture"));
	}
}

void ASSPVolume::UpdateSSPTextureBounded(UMaterialInstanceDynamic* Material,
	const int& NumX, const int& NumY, const int& NumZx, const int& NumZy,
	const FVector& LowerBound, const FVector& UpperBound)
{
	TArray<float> SoundSpeeds;

	int NumZ = NumZx * NumZy;
	int CountZ = 0;
	for (int n = 0; n < NumZy; ++n) {
		for (int i = 0; i < NumY; ++i) {
			for (int j = 0; j < NumZx; ++j) {
				for (int k = 0; k < NumX; ++k) {
					float ssp;
					FVector SamplePoint(
						FMath::Lerp(LowerBound[0], UpperBound[0], float(k) / float(NumX)),
						FMath::Lerp(UpperBound[1], LowerBound[1], float(i) / float(NumY)),
						-FMath::Lerp(LowerBound[2], UpperBound[2], float(n*NumZx+j) / float(NumZ))
					);
					Bellhop->GetSoundSpeed(SamplePoint, ssp);
					SoundSpeeds.Add(ssp);
				}
			}
		}
	}

	TArray<unsigned int> Pixels;
	ConvertToColor(SoundSpeeds, Pixels);

	UTexture2D* tex = CreateTexture(Pixels, NumZx * NumX, NumZy * NumY);
	if (Material)
	{
		Material->SetTextureParameterValue(FName("SoundSpeed"), (UTexture*)tex);
		UE_LOG(LogTemp, Warning, TEXT("updated ssp texture"));
	}
}

/// <summary>
/// Sample a line of sound speeds from Top to Bottom.
/// </summary>
/// <param name="Mat">Colored material</param>
/// <param name="NumSamples">Vertical resolution</param>
/// <param name="Top">top of the profile line</param>
/// <param name="Bottome">bottom of the profile line</param>
void ASSPVolume::
UpdateSSPLineTexture(UMaterialInstanceDynamic* Mat,
	const int& NumSamples, const FVector& Top, const FVector& Bottom)
{
	TArray<float> SoundSpeeds;
	for (int i = 0; i < NumSamples; ++i) {
		float ssp;
		auto SamplePoint = FMath::Lerp(Bottom, Top, float(i) / float(NumSamples));
		Bellhop->GetSoundSpeed(SamplePoint, ssp);
		SoundSpeeds.Add(ssp);
	}
	TArray<unsigned int> Pixels;
	ConvertToColor(SoundSpeeds, Pixels);
	UTexture2D* tex = CreateTexture(Pixels, NumSamples, 1);
	if (Mat)
	{
		Mat->SetTextureParameterValue(FName("SoundSpeedLine"), (UTexture*)tex);
		UE_LOG(LogTemp, Warning, TEXT("updated ssp line texture"));
	}
}


UTexture2D*
ASSPVolume::CreateTexture(const TArray<unsigned int>& Data,
	const int& TexWidth, const int& TexHeight) const
{
	UTexture2D* Texture;
	Texture = UTexture2D::CreateTransient(TexHeight, TexWidth, PF_R8G8B8A8);
	if (!Texture) return nullptr;
#if WITH_EDITORONLY_DATA
	Texture->MipGenSettings = TMGS_NoMipmaps;
#endif
	Texture->NeverStream = true;
	Texture->SRGB = 0;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(RawData, Data.GetData(), TexWidth * TexHeight * 4);
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

/// <summary>
/// Update the boundary to a cube around the whole bathymetry, up to the surface.
/// Unreal coords
/// Note that z is up, so MinBound is at the bottom, MaxBound at the surface.
/// </summary>
void ASSPVolume::UpdateBoundaryBox()
{
	if (bBoundaryDirty) {
		TArray<FVector> bounds = Bellhop->GetBoundaryPoints();
		if (bounds.Num() < 1) {
			UE_LOG(LogTemp, Warning, TEXT("Warning: not enough boundary points ... continuing"));
			return;
		}
		MinBound = bounds[0];
		MaxBound = bounds[0];
		for (auto const& p : bounds)
		{
			for (int i = 0; i < 2; ++i) {
				MinBound[i] = FMath::Min(MinBound[i], p[i]);
				MaxBound[i] = FMath::Max(MaxBound[i], p[i]);
			}
		}
		//no sound speed above the surface (z is down)
		MaxBound[2] = 0;
	}
	bBoundaryDirty = false;
}

/// <summary>
/// Convert the sound speeds to gray scale colors.
/// Values < 1 are mapped to 0, rest are lerped.
/// </summary>
/// <param name="SoundSpeeds"></param>
/// <param name="Pixels"></param>
void ASSPVolume::ConvertToColor(const TArray<float>& SoundSpeeds,
	TArray<unsigned int>& Pixels) const
{
	if (SoundSpeeds.Num() < 1) {
		UE_LOG(LogTemp, Warning, TEXT("Warning: not enough sound speeds ... something is very bad ... continuing"));
		return;
	}

	//TODO: could pass in these values from previous read and save a loop
	float MinSSP = -1;
	float MaxSSP = -1;
	for (auto const& ssp : SoundSpeeds) {
		if (ssp > 1) {
			if (MinSSP < 0) {
				MinSSP = ssp;
				MaxSSP = ssp;
			}
			MinSSP = FMath::Min(ssp, MinSSP);
			MaxSSP = FMath::Max(ssp, MaxSSP);
		}
	}

	if (MinSSP < 0) {
		UE_LOG(LogTemp, Warning, TEXT("Warning: all sound speeds are zero ... continuing"));
	}

	int LowGray = 0x00;
	int HighGray = 0xff;
	for (auto const& ssp : SoundSpeeds) {
		if (ssp > 1) {
			char l = FMath::Lerp(LowGray, HighGray, (ssp - MinSSP) / (MaxSSP - MinSSP));
			unsigned int val = 0 | (l << 16) | (l << 8) | l;
			Pixels.Add(val);
		}
		else {
			Pixels.Add(0);
		}
	}
}

