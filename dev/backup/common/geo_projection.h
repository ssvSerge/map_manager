#ifndef __geo_projection_h__
#define __geo_projection_h__

#define M_PI                (3.14159265358979323846)
#define DEG2RAD(a)          ( (a) / (180 / M_PI) )
#define RAD2DEG(a)          ( (a) * (180 / M_PI) )
#define EARTH_RADIUS        (6378137)

static double lat2y_m ( double lat ) {
    return log ( tan ( DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS;
}

static double lon2x_m ( double lon ) {
    return DEG2RAD(lon) * EARTH_RADIUS;
}

static void _pt_to_projection ( map_pos_t& pos ) {

    pos.x = lon2x_m ( pos.x );
    pos.y = lat2y_m ( pos.y );
}

#endif
