#ifndef __GEO_PROJECTION_H__
#define __GEO_PROJECTION_H__

// double lat2y_m      ( double lat);
// double lon2x_m      ( double lon);
// double lat2y_g      ( double y );
// double lon2x_g      ( double x );
// double toRadians    ( double val);

void geo_2_proj ( const double lat, const double lon, double& x, double& y );
void proj_2_geo ( const double x, const double y, double& lat, double& lon );

double gps_distance ( double lon1, double lat1, double lon2, double lat2 );

#endif
