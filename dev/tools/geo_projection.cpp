#include <cmath>
#include <geo_projection.h>

#define M_PI                (3.14159265358979323846)
#define DEG2RAD(a)          ( (a) / (180 / M_PI) )
#define RAD2DEG(a)          ( (a) * (180 / M_PI) )
#define EARTH_RADIUS        (6378137)


double lat2y_m ( double lat ) {
    return log ( tan ( DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS;
}

double lon2x_m ( double lon ) {
    return DEG2RAD(lon) * EARTH_RADIUS;
}

double toRadians ( double val ) {
    return val / 57.295779513082325;
}

double gps_distance ( double lon1, double lat1, double lon2, double lat2 ) {

    double R  = 6371e3;
    double f1 = toRadians(lat1);
    double f2 = toRadians(lat2);
    double dF = toRadians(lat2 - lat1);
    double dL = toRadians(lon2 - lon1);

    double a = sin(dF / 2) * sin(dF / 2) + cos(f1) * cos(f2) * sin(dL / 2) * sin(dL / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double d = R * c;

    return d;
}
