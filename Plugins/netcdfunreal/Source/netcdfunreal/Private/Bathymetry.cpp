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
		GridX.Push(Distance(OriginLatitude, OriginLongitude,
			allY[i], OriginLongitude));
	}
	GridY.Empty();
	for (size_t i = indexLonLow; i < indexLonHigh; ++i) {
		GridY.Push(Distance(OriginLatitude, OriginLongitude,
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
			EarthBathymetry->LoadHYCOMDepth(url, HexDepth);
			EarthBathymetry->LoadHYCOMLatitude(url, allLatitude);
			EarthBathymetry->LoadHYCOMLongitude(url, allLongitude);
			EarthBathymetry->LoadHYCOMTime(url, allTime);

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
				HexGridX.Push(
					((allLatitude[i] < OriginLatitude) ? -1.0 : 1.0) *
					Distance(OriginLatitude, OriginLongitude,
						allLatitude[i], OriginLongitude)
				);
			}

			HexGridY.Empty();
			for (int i = westIndex; i <= eastIndex; ++i) {
				const double lon =
					(allLongitude[i] > 180.0) ?
					(allLongitude[i] - 360) : allLongitude[i];
				HexGridY.Push(
					((allLongitude[i] < HOriginLongitude) ? -1.0 : 1.0) *
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

	return FVector(ResX, ResY, 0);
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

