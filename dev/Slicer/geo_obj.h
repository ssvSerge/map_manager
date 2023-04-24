#ifndef __GEO_OBJ_H__
#define __GEO_OBJ_H__

#include <vector>
#include <list>

typedef enum tag_geo_entry {
    GEO_ENTRY_AREA,
    GEO_ENTRY_BUILDING,
    GEO_ENTRY_HIGHWAY,
}   geo_entry_t;

typedef enum tag_geo_area_type {
    GEO_AREA_ASPHALT,
    GEO_AREA_WATER,
    GEO_AREA_GRASS,
}   geo_area_type_t;

typedef enum tag_geo_role {
    ROLE_OUTER,
    ROLE_INNER
}   geo_role_t;

typedef struct tag_geo_coord {
    double                      lon;
    double                      lat;
}   geo_coord_t;

typedef std::vector<geo_coord_t> vector_geo_coords_t;

typedef struct tag_geo_object {
    geo_role_t                  role;
    geo_entry_t                 type;
    size_t                      area;
    vector_geo_coords_t         coords;
}   geo_object_t;

typedef enum tag_lex_param {
    PARAM_RECORD,
    PARAM_TYPE,
    PARAM_ROLE,
    PARAM_SIZE,
    PARAM_POINTS,
    PARAM_COORD,
    PARAM_LAST_ID
}   lex_param_t;


#endif

