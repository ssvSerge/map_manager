#include <cmath>
#include <geo_projection.h>

#define M_PI                ( 3.14159265358979323846 )
#define EARTH_R             ( 6378137.0 )

//---------------------------------------------------------------------------//

static double deg2rad ( double deg ) {
	return (deg * M_PI / 180.0);
}

static double rad2deg ( double radians ) {
	return radians * 180.0 / M_PI;
}

//---------------------------------------------------------------------------//

void geo_2_proj ( const double lat, const double lon, double& x, double& y ) {

	static const double scale_x = EARTH_R * M_PI / 180;

	x = lon * scale_x;
	y = EARTH_R * log(tan((M_PI * 0.25) + (0.5 * (M_PI * lat / 180))));
}

void proj_2_geo ( const double x, const double y, double& lon, double& lat ) {

	const double R2D = 180 / M_PI;
	const double A   = EARTH_R;

	lon = x * R2D / A;
	lat = ((M_PI * 0.5) - 2.0 * atan(exp(-y / A))) * R2D;
}

//---------------------------------------------------------------------------//

double gps_azimuth ( double lat1, double lon1, double lat2, double lon2 ) {

	lat1 = deg2rad(lat1);
	lon1 = deg2rad(lon1);
	lat2 = deg2rad(lat2);
	lon2 = deg2rad(lon2);

	double deltaLon = lon2 - lon1;

	double y = std::sin(deltaLon) * std::cos(lat2);
	double x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(deltaLon);

	double azimuth = std::atan2(y, x);
	azimuth = rad2deg(azimuth);

	azimuth = fmod(azimuth + 360.0, 360.0);

	return azimuth;
}

//---------------------------------------------------------------------------//

double gps_distance_haversine ( double latitude1, double longitude1, double latitude2, double longitude2 ) {

	double lat1   =  deg2rad(latitude1);
	double lon1   =  deg2rad(longitude1);
	double lat2   =  deg2rad(latitude2);
	double lon2   =  deg2rad(longitude2);
	double d_lat  =  abs(lat1 - lat2);
	double d_lon  =  abs(lon1 - lon2);

	double sin_d_lat = sin(d_lat / 2);
	double sin_d_lon = sin(d_lon / 2);
	double cos_lat1  = cos(lat1);
	double cos_lat2  = cos(lat2);

	double a = sin_d_lat * sin_d_lat  +  cos_lat1 * cos_lat2 * sin_d_lon * sin_d_lon;

	double d_sigma = 2 * asin(sqrt(a));

	return EARTH_R * d_sigma;
}

double gps_distance_vincenty ( double latitude1, double longitude1, double latitude2, double longitude2) {

	double lat1 = deg2rad(latitude1);
	double lon1 = deg2rad(longitude1);
	double lat2 = deg2rad(latitude2);
	double lon2 = deg2rad(longitude2);

	double d_lon = abs(lon1 - lon2);

	double a = pow(cos(lat2) * sin(d_lon), 2);

	double b = cos(lat1) * sin(lat2);
	double c = sin(lat1) * cos(lat2) * cos(d_lon);
	double d = pow(b - c, 2);

	double e = sqrt(a + d);

	double f = sin(lat1) * sin(lat2);
	double g = cos(lat1) * cos(lat2) * cos(d_lon);

	double h = f + g;

	double d_sigma = atan2(e, h);

	return EARTH_R * d_sigma;
}

double gps_distance ( double lon1, double lat1, double lon2, double lat2 ) {
	double dist = gps_distance_vincenty ( lon1, lat1, lon2, lat2 );
	return dist;
}

//---------------------------------------------------------------------------//
