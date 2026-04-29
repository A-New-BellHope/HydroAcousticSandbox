// Fill out your copyright notice in the Description page of Project Settings.


#include "Bathymetry.h"

// Sets default values
ABathymetry::ABathymetry() : OriginLatitude(-1000.0), OriginLongitude(-1000.0)
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABathymetry::BeginPlay()
{
	Super::BeginPlay();

	EarthBathymetry = FModuleManager::GetModulePtr<FnetcdfunrealModule>("netcdfunreal");

	Init();
}

void ABathymetry::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CheckHYCOM()) {
		HYCOMDone = false;
		OnHYCOMDoneEvent.Broadcast();
	}
}

/// <summary>
/// All of the longitude sample points (20k or so).
/// Safe to call with no file, but clears the output array.
/// TODO: should cache these
/// </summary>
/// <param name="Longitude">modified</param>
/// <returns>success</returns>
bool ABathymetry::Longitude(TArray<double>& Longitude) const
{
	Longitude.Empty();

	if (!EarthBathymetry->CheckNcFile()) {
		std::cout << 
			"Error: attempted to access Longitude with bad file ... ignoring.\n" << 
			std::flush;
		return false;
	}

	return EarthBathymetry->GetArray("lon", Longitude);
}

/// <summary>
/// All of the latitude sample points (20k or so)
/// Safe to call with no file, but clears the output array.
/// TODO: should cache these
/// </summary>
/// <param name="Latitude">modified</param>
/// <returns>success</returns>
bool ABathymetry::Latitude(TArray<double>& Latitude) const
{
	Latitude.Empty();

	if (!EarthBathymetry->CheckNcFile()) {
		std::cout <<
			"Error: attempted to access Latitude with bad file ... ignoring.\n" <<
			std::flush;
		return false;
	}

	return EarthBathymetry->GetArray("lat", Latitude);
}

/// <summary>
/// Set the origin for the bathymetry and sound speed to convert
///  from lat/long to km for bellhop.
/// </summary>
/// <param name="InOriginLatitude"></param>
/// <returns>success</returns>
bool ABathymetry::SetOrigin(const double& InOriginLatitude,
	const double& InOriginLongitude)
{
	OriginLatitude = InOriginLatitude;
	OriginLongitude = InOriginLongitude;
	return true;
}

/// <summary>
/// Popup a dialog to open the gebco file.
/// Not very general. Intended to be called from blueprints.
/// </summary>
/// <param name="Filename"></param>
/// <returns>success</returns>
bool ABathymetry::OpenGEBCO(FString& Filename)
{
	TArray<FString> OutFileNames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if (DesktopPlatform)
	{
		const void* ParentWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowHandle,
			TEXT("Open GEBCO Bathymetry File"),
			TEXT(""),
			TEXT(""),
			TEXT("NetCDF Files (*.nc)|*.nc"),
			EFileDialogFlags::None,
			OutFileNames
		);
	}
	if (bOpened && OutFileNames.Num() > 0)
	{
		Filename = OutFileNames[0];
		return LoadEarthFile(Filename);
	}
	return false;
}

/// <summary>
/// Load the nc file with the bathymetry data.
/// Probably downloaded from gebco.
/// </summary>
/// <param name="Filename">e.g. c:\foo\bar\world.nc</param>
/// <returns>success</returns>
bool ABathymetry::LoadEarthFile(const FString& Filename)
{
	return EarthBathymetry->LoadBathymetryFile(Filename);
}

/// <summary>
/// Get the bathymetry within lat/lon for bellhop.
/// Formated as 
/// </summary>
/// <param name="North">Degrees latitude of north edge</param>
/// <param name="East">Degrees longitude of east edge</param>
/// <param name="South">latitude</param>
/// <param name="West">longitude</param>
/// <param name="Depth">modified. depth in meters</param>
/// <returns>Bounds in compass order, North, East, South, West</returns>
bool ABathymetry::GetEarthBathymetry (
	const double& North, const double& East,
	const double& South, const double& West,
	double& ActualNorth, double& ActualEast,
	double& ActualSouth, double& ActualWest,
	TArray<double>& GridX, TArray<double>& GridY,
	TArray<double>& Depth) const
{
	TArray<double> allX;
	TArray<double> allY;

	if (bTiffLoaded)
	{
		// Log what we actually received
		UE_LOG(LogTemp, Warning, TEXT("TIFF bounds in: N=%.4f S=%.4f E=%.4f W=%.4f"),
			North, South, East, West);
		UE_LOG(LogTemp, Warning, TEXT("TIFF coverage:  N=%.4f S=%.4f E=%.4f W=%.4f"),
			TiffNorth, TiffSouth, TiffEast, TiffWest);

		double ClampedNorth = FMath::Min(North, TiffNorth);
		double ClampedSouth = FMath::Max(South, TiffSouth);
		double ClampedEast = FMath::Min(East, TiffEast);
		double ClampedWest = FMath::Max(West, TiffWest);

		if (ClampedSouth >= ClampedNorth || ClampedWest >= ClampedEast)
		{
			ErrorMessage(TEXT("TIFF: Requested bounds don't overlap TIFF coverage."));
			return false;
		}

		int32 ColLow = (int32)((ClampedWest - TiffWest) / (TiffEast - TiffWest) * TiffWidth);
		int32 ColHigh = (int32)((ClampedEast - TiffWest) / (TiffEast - TiffWest) * TiffWidth);
		int32 RowLow = (int32)((TiffNorth - ClampedNorth) / (TiffNorth - TiffSouth) * TiffHeight);
		int32 RowHigh = (int32)((TiffNorth - ClampedSouth) / (TiffNorth - TiffSouth) * TiffHeight);

		ColLow = FMath::Clamp(ColLow, 0, TiffWidth - 1);
		ColHigh = FMath::Clamp(ColHigh, 0, TiffWidth - 1);
		RowLow = FMath::Clamp(RowLow, 0, TiffHeight - 1);
		RowHigh = FMath::Clamp(RowHigh, 0, TiffHeight - 1);

		// Subsample to keep output manageable for Bellhop
		// Match whatever resolution the NetCDF path was giving you
		int32 Step = FMath::Max(1, (ColHigh - ColLow) / 200);

		UE_LOG(LogTemp, Warning, TEXT("TIFF pixel bounds: Col=%d-%d Row=%d-%d Step=%d"),
			ColLow, ColHigh, RowLow, RowHigh, Step);

		ActualWest = TiffWest + ColLow * (TiffEast - TiffWest) / TiffWidth;
		ActualEast = TiffWest + ColHigh * (TiffEast - TiffWest) / TiffWidth;
		ActualNorth = TiffNorth - RowLow * (TiffNorth - TiffSouth) / TiffHeight;
		ActualSouth = TiffNorth - RowHigh * (TiffNorth - TiffSouth) / TiffHeight;

		GridX.Empty();
		for (int32 Row = RowLow; Row < RowHigh; Row += Step)
		{
			double Lat = TiffNorth - Row * (TiffNorth - TiffSouth) / TiffHeight;
			GridX.Add(Distance(OriginLatitude, OriginLongitude, Lat, OriginLongitude)
				* (Lat >= OriginLatitude ? 1.0 : -1.0));
		}

		GridY.Empty();
		for (int32 Col = ColLow; Col < ColHigh; Col += Step)
		{
			double Lon = TiffWest + Col * (TiffEast - TiffWest) / TiffWidth;
			GridY.Add(Distance(OriginLatitude, OriginLongitude, OriginLatitude, Lon)
				* (Lon >= OriginLongitude ? 1.0 : -1.0));
		}

		Depth.Empty();
		for (int32 Row = RowLow; Row < RowHigh; Row += Step)
			for (int32 Col = ColLow; Col < ColHigh; Col += Step)
				Depth.Add(-TiffDepthGrid[Row * TiffWidth + Col]);

		UE_LOG(LogTemp, Warning, TEXT("TIFF -> Bellhop: GridX=%d GridY=%d Depth=%d"),
			GridX.Num(), GridY.Num(), Depth.Num());

		return true;
	}

	if (!EarthBathymetry->GetArray("lon", allX) ||
		!EarthBathymetry->GetArray("lat", allY)) {
		ErrorMessage("Error: could not read lat/long data. "
			"Check that the file loaded ... returning.");
		return false;
	}

	const size_t indexLonLow = Algo::UpperBound(allX, West) - 1;
	const size_t indexLonHigh = Algo::UpperBound(allX, East);
	const size_t indexLatLow = Algo::UpperBound(allY, South) - 1;
	const size_t indexLatHigh = Algo::UpperBound(allY, North);
	ActualWest = allX[indexLonLow];
	ActualEast = allX[indexLonHigh];
	ActualSouth = allY[indexLatLow];
	ActualNorth = allY[indexLatHigh];

	if (indexLatLow >= indexLatHigh || indexLonLow >= indexLonHigh) {
		ErrorMessage("Error: ranges are too small or in the wrong order."
			"Check values of North, South, East, and West edges ... returning.");
		return false;
	}

	const std::vector<size_t> start = { indexLatLow, indexLonLow };
	const std::vector<size_t> count = { indexLatHigh - indexLatLow, indexLonHigh - indexLonLow };

	if (!EarthBathymetry->GetArraySubset("elevation", start, count, Depth)) {
		ErrorMessage("Error: unknown error getting array data ... returning.");
		return false;
	}

	//Bellhop wants depth positive going down.
	for (auto& x : Depth) { x *= -1; }

	if (OriginLatitude < -500 || OriginLongitude < -500) {
		ErrorMessage("Warning (GetEarthSoundSpeed): Origin not set, "
			"cannot calculate grid ... returning.");
		return false;
	}

	//TODO: extra copying and allocation (probably not very large)
	GridX.Empty();
	for (size_t i = indexLatLow; i < indexLatHigh; ++i) {
		double sign = (allY[i] < OriginLatitude) ? -1.0 : 1.0;
		GridX.Push(sign * Distance(OriginLatitude, OriginLongitude,
			allY[i], OriginLongitude));
	}
	GridY.Empty();
	for (size_t i = indexLonLow; i < indexLonHigh; ++i) {
		double sign = (allX[i] < OriginLongitude) ? -1.0 : 1.0;
		GridY.Push(sign * Distance(OriginLatitude, OriginLongitude,
			OriginLatitude, allX[i]));
	}

	return true;
}

/// <summary>
/// Get the sound speed within lat/lon for bellhop.
/// Formated for bellhop plugin
/// This just starts the calculation, CheckHYCOM() to check the status. 
/// </summary>
/// <param name="North">Degrees latitude of north edge</param>
/// <param name="East">Degrees longitude (-180,180) of east edge</param>
/// <param name="South">latitude</param>
/// <param name="West">longitude</param>
/// <param name="Time">time</param>
/// <param name="Depth">modified. depth in meters</param>
/// <param name="Soundspeed">modified. soundspeed in meters per second</param>
void ABathymetry::GetEarthSoundSpeed(const double& North, const double& East,
	const double& South, const double& West, const FDateTime& Time)
{
	if (HYCOMDone) {
		ErrorMessage("Warning (GetEarthSoundSpeed): already downloading hycom."
			"This should not happen ... ignoring.");
		return;
	}

	if (OriginLatitude < -500 || OriginLongitude < -500) {
		ErrorMessage("Warning (GetEarthSoundSpeed): Origin not set, "
			"cannot calculate grid ... returning.");
		return;
	}

	// Switch to HYCOM's 'degrees east of 0' convention
	const double HOriginLongitude = 
		(OriginLongitude < 0) ? OriginLongitude + 360.0 : OriginLongitude;

	HYCOMDone = false;
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]
		{
			FString url = "https://tds.hycom.org/thredds/dodsC/GLBy0.08/expt_93.0/ts3z";
			TArray<double> allLatitude;
			TArray<double> allLongitude;
			TArray<double> allTime;
			bool hycomSuccess = true;
			hycomSuccess &= EarthBathymetry->LoadHYCOMDepth(url, HexDepth);
			if (hycomSuccess) { hycomSuccess &= EarthBathymetry->LoadHYCOMLatitude(url, allLatitude); }
			if (hycomSuccess) {
				hycomSuccess &= EarthBathymetry->LoadHYCOMLongitude(url, allLongitude);
			}
			if (hycomSuccess) {
				hycomSuccess &= EarthBathymetry->LoadHYCOMTime(url, allTime);
			}

			if (!hycomSuccess) {
				ErrorMessage("Error: could not read HYCOM data. "
					"Defaulting to a Munk profile ... returning.");
				HYCOMDone = true;
				return;
			}

			int southIndex = Algo::UpperBound(allLatitude, South) - 1;
			int northIndex = Algo::UpperBound(allLatitude, North);
			int westIndex =
				Algo::UpperBound(allLongitude, (West < 0) ? West + 360.0 : West) - 1;
			int eastIndex =
				Algo::UpperBound(allLongitude, (East < 0) ? East + 360.0 : East);
			//HACK - The reference time is 2000-01-01 at 00:00:00, or 946713600 unix timestamp
			double HoursElapsed = (Time - FDateTime::FromUnixTimestamp(946713600)).GetTotalHours();
			UE_LOGFMT(LogTemp, Warning, "Hours elapsed {0}", HoursElapsed);
			int timeIndex = Algo::UpperBound(allTime, HoursElapsed);

			HexGridX.Empty();
			for (int i = southIndex; i <= northIndex; ++i) {
				double sign = (allLatitude[i] < OriginLatitude) ? -1.0 : 1.0;
				HexGridX.Push(sign * Distance(OriginLatitude, OriginLongitude,
					allLatitude[i], OriginLongitude)
				);
			}

			HexGridY.Empty();
			for (int i = westIndex; i <= eastIndex; ++i) {
				double lon =
					(allLongitude[i] > 180.0) ?
					(allLongitude[i] - 360) : allLongitude[i];
				double sign = (lon < OriginLongitude) ? -1.0 : 1.0;
				HexGridY.Push(sign *
					Distance(OriginLatitude, OriginLongitude,
						OriginLatitude, lon)
				);
			}

			HexSoundSpeed.Empty();
			EarthBathymetry->LoadHYCOMSoundSpeed(url, timeIndex, timeIndex, 0, 39,
				southIndex, northIndex, westIndex, eastIndex, HexSoundSpeed);

			HYCOMDone = true;
		}
	);
}

/// <summary>
/// Check if the HYCOM calculation is going.
/// Threadsafe
/// </summary>
/// <returns>Is hycom done?</returns>
bool ABathymetry::CheckHYCOM() const
{
	return HYCOMDone;
}

/// <summary>
/// Convert lat long to Unreal coordinates.
/// Only makes sense if the origin is set (no checking).
/// </summary>
/// <param name="Latitude"></param>
/// <param name="Longitude"></param>
/// <returns></returns>
FVector ABathymetry::LatLongToPosition(
	const double& Latitude, const double& Longitude) const
{
	auto ResX = Distance(OriginLatitude, OriginLongitude,
		OriginLatitude, Longitude);
	if (Longitude < OriginLongitude) {
		ResX *= -1.0;
	}

	auto ResY = Distance(OriginLatitude, OriginLongitude,
		Latitude, OriginLongitude);
	if (Latitude < OriginLatitude) {
		ResY *= -1.0;
	}

	return FVector(ResY, ResX, 0);
}

/// <summary>
/// Convert Unreal coordinates to lat long.
/// Only makes sense if the origin is set.
/// Assumes a depth of 0.
/// </summary>
/// <param name="Position">unreal coords (meters)</param>
/// <returns>latitude, longitude, 0</returns>
FVector ABathymetry::PositionToLatLong(const FVector& Position) const
{
	const double latRad = FMath::DegreesToRadians(OriginLatitude);
	const double lonRad = FMath::DegreesToRadians(OriginLongitude);
	const double bearingRad = FMath::Atan2(Position.Y, Position.X);
	double angularDistance =
		FMath::Sqrt(Position.X * Position.X + Position.Y * Position.Y) /
		EarthRadius;

	double newLatRad = asin(sin(latRad) * cos(angularDistance) + cos(latRad) * sin(angularDistance) * cos(bearingRad));
	double newLonRad = lonRad + atan2(sin(bearingRad) * sin(angularDistance) * cos(latRad), cos(angularDistance) - sin(latRad) * sin(newLatRad));

	return FVector(FMath::RadiansToDegrees(newLatRad),
		FMath::RadiansToDegrees(newLonRad), 0);
}

bool ABathymetry::OpenTIFF(FString& Filename)
{
	TArray<FString> OutFileNames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if (DesktopPlatform)
	{
		const void* ParentWindowHandle = FSlateApplication::Get()
			.GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowHandle,
			TEXT("Open Bathymetry TIFF"),
			TEXT(""),
			TEXT(""),
			TEXT("TIFF Files (*.tif;*.tiff)|*.tif;*.tiff"),
			EFileDialogFlags::None,
			OutFileNames
		);
	}
	if (bOpened && OutFileNames.Num() > 0)
	{
		Filename = OutFileNames[0];
		return LoadTiffBathymetry(Filename);
	}
	return false;
}

bool ABathymetry::LoadTiffBathymetry(const FString& Filename)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Filename))
	{
		ErrorMessage(TEXT("TIFF: Failed to read file."));
		return false;
	}

	const uint8* D = Bytes.GetData();
	if (Bytes.Num() < 8 || D[0] != 'I' || D[1] != 'I')
	{
		ErrorMessage(TEXT("TIFF: Not a valid little-endian TIFF."));
		return false;
	}

	// Helper lambdas
	auto U16 = [&](int32 o) { uint16 v; FMemory::Memcpy(&v, D + o, 2); return v; };
	auto U32 = [&](int32 o) { uint32 v; FMemory::Memcpy(&v, D + o, 4); return v; };

	uint32 IFDOff = U32(4);
	uint16 NumEntries = U16(IFDOff);

	uint32 Width = 0, Height = 0;
	TArray<uint32> StripOffsets, StripByteCounts;

	for (int32 i = 0; i < NumEntries; ++i)
	{
		int32  E = IFDOff + 2 + i * 12;
		uint16 Tag = U16(E);
		uint16 Typ = U16(E + 2);
		uint32 Cnt = U32(E + 4);

		auto Val = [&]() -> uint32 {
			uint32 tb = (Typ == 3 ? 2u : 4u) * Cnt;
			if (tb <= 4) return Typ == 3 ? U16(E + 8) : U32(E + 8);
			uint32 off = U32(E + 8); return Typ == 3 ? U16(off) : U32(off);
		};
		auto Arr = [&](TArray<uint32>& Out) {
			uint32 tb = (Typ == 3 ? 2u : 4u) * Cnt;
			uint32 off = (tb <= 4) ? (uint32)(E + 8) : U32(E + 8);
			for (uint32 s = 0; s < Cnt; ++s)
				Out.Add(Typ == 3 ? U16(off + s * 2) : U32(off + s * 4));
		};

		switch (Tag)
		{
		case 256: Width = Val(); break;
		case 257: Height = Val(); break;
		case 273: Arr(StripOffsets);    break;
		case 279: Arr(StripByteCounts); break;
		}
	}

	TiffWidth = (int32)Width;
	TiffHeight = (int32)Height;
	int32 Total = TiffWidth * TiffHeight;
	TiffDepthGrid.SetNumUninitialized(Total);

	// Uncompressed float32 read — one strip per row (RowsPerStrip=1)
	int32 Cursor = 0;
	for (int32 s = 0; s < StripOffsets.Num() && Cursor < Total; ++s)
	{
		const uint8* Row = D + StripOffsets[s];
		int32        NPix = StripByteCounts[s] / 4;
		for (int32 p = 0; p < NPix && Cursor < Total; ++p, ++Cursor)
		{
			float Val; FMemory::Memcpy(&Val, Row + p * 4, 4);
			TiffDepthGrid[Cursor] = (Val >= TIFF_NODATA * 0.5f) ? 0.f : Val;
		}
	}

	float FallbackDepth = 0.f;
	for (float V : TiffDepthGrid)
		if (V < 0.f && V > -1e30f) // valid negative depth
			FallbackDepth = FMath::Min(FallbackDepth, V);

	UE_LOG(LogTemp, Warning, TEXT("TIFF: Deepest valid depth = %.2f m, using as NoData fill."),
		FallbackDepth);

	// Replace NoData with the deepest depth so they sit flat at the bottom
	for (float& V : TiffDepthGrid)
		if (V >= TIFF_NODATA * 0.5f || V == 0.f)
			V = FallbackDepth;

	bTiffLoaded = true;
	FScalarParameterValue();
	UE_LOG(LogTemp, Log, TEXT("TIFF loaded: %dx%d, origin (%.1f, %.1f) UTM Zone 20N"),
		TiffWidth, TiffHeight, TiffOriginEasting, TiffOriginNorthing);
	return true;
}

void ABathymetry::SetTiffBounds(const double& North, const double& East, const double& South, const double& West)
{
	TiffNorth = North;
	TiffEast = East;
	TiffSouth = South;
	TiffWest = West;
}

/// <summary>
/// Extra initialization.
/// </summary>
void ABathymetry::Init()
{}

/// <summary>
/// Estimate the distance between these two points on Earth.
/// TODO: should use a better estimate (e.g., from boost::geometry)
///    taken from https://stackoverflow.com/questions/27126714/c-latitude-and-longitude-distance-calculator
/// </summary>
double ABathymetry::Distance(double latitud1, double longitud1,
	double latitud2, double longitud2) const
{
	double haversine;
	double temp;
	double distancia_puntos;

	latitud1 = FMath::DegreesToRadians(latitud1);
	longitud1 = FMath::DegreesToRadians(longitud1);
	latitud2 = FMath::DegreesToRadians(latitud2);
	longitud2 = FMath::DegreesToRadians(longitud2);

	haversine = (pow(sin((1.0 / 2) * (latitud2 - latitud1)), 2)) + 
		((cos(latitud1)) * (cos(latitud2)) * (pow(sin((1.0 / 2) * (longitud2 - longitud1)), 2)));
	temp = 2 * asin(FMath::Min(1.0, FMath::Sqrt(haversine)));
	distancia_puntos = EarthRadius * temp;

	return distancia_puntos;
}

void ABathymetry::ErrorMessage(const FString& Message) const
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
}

