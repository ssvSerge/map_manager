#ifndef __GEO_TOOLS_H__
#define __GEO_TOOLS_H__

#include <geo_types.h>

void geo_intersect ( const v_geo_coord_t& polyline, const geo_rect_t& rect, const pos_type_t& type, vv_geo_coord_t& clippedLine );

#endif
