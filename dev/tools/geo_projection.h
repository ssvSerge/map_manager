#ifndef __GEO_PROJECTION_H__
#define __GEO_PROJECTION_H__

double lat2y_m      ( double lat);
double lon2x_m      ( double lon);
double toRadians    ( double val);
double gps_distance ( double lon1, double lat1, double lon2, double lat2 );

#endif
