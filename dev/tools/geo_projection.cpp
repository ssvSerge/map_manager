#include <cmath>
#include <geo_projection.h>

#define M_PI                ( 3.14159265358979323846 )
#define EARTH_R             ( 6378137.0 )

// 
//   85,051129∞
//
// - 20037508,34 м
// + 20037508,34 м
// 
//   эллипсоид Ч  расовского
// 	 a = 6378245 м; f = 1:298,3
// 
//   WGS84
//   a = 6378137 м; f = 1:298,257223563
//

void geo_2_proj ( const double lat, const double lon, double& x, double& y ) {

	static const double scale_x = EARTH_R * M_PI / 180;

	x = lon * scale_x;
	y = EARTH_R * log(tan((M_PI * 0.25) + (0.5 * (M_PI * lat / 180))));
}

void proj_2_geo ( const double x, const double y, double& lat, double& lon ) {

	const double R2D = 180 / M_PI;
	const double A   = EARTH_R;

	lon = x * R2D / A;
	lat = ((M_PI * 0.5) - 2.0 * atan(exp(-y / A))) * R2D;
}

static double deg_rad ( double ang ) {
	const double D_R = (M_PI / 180.0);
	return ang * D_R;
}

double gps_distance ( double lon1, double lat1, double lon2, double lat2 ) {

	double R  =  EARTH_R;
	double f1 =  deg_rad(lat1);
	double f2 =  deg_rad(lat2);
	double dF =  deg_rad(lat2 - lat1);
	double dL =  deg_rad(lon2 - lon1);

	double a  =  sin(dF / 2) * sin(dF / 2) + cos(f1) * cos(f2) * sin(dL / 2) * sin(dL / 2);
	double c  =  2 * atan2(sqrt(a), sqrt(1 - a));
	double d  =  R * c;

	return d;
}
