#ifndef __GEO_PROJECTION_H__
#define __GEO_PROJECTION_H__

void geo_2_proj ( const double lon, const double lat, double& x, double& y );
void proj_2_geo ( const double x, const double y, double& lon, double& lat );

double gps_distance ( double lon1, double lat1, double lon2, double lat2 );
double gps_azimuth  ( double lat1, double lon1, double lat2, double lon2 );

#endif
