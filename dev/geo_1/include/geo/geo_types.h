#ifndef __GEO_TYPES_H__
#define __GEO_TYPES_H__

#include <stdint.h>
#include <list>
#include <vector>
#include <set>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include <clipper2/clipper.h>

typedef Clipper2Lib::PointD             geo_coords_t;
typedef Clipper2Lib::PathD              geo_path_t;
typedef Clipper2Lib::PathsD             geo_set_t;

typedef std::vector<geo_set_t>          geo_map_t;

typedef uint32_t                        geo_offset_t;

typedef std::vector<geo_offset_t>       vector_geo_off_t;
typedef std::set<geo_offset_t>          set_geo_off_t;

typedef std::list<geo_offset_t>         list_offset_t;
typedef std::vector<list_offset_t>      vector_list_offset_t;

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

typedef enum tag_lex_param {
    PARAM_RECORD,
    PARAM_TYPE,
    PARAM_ROLE,
    PARAM_SIZE,
    PARAM_POINTS,
    PARAM_COORD,
    PARAM_LAST_ID
}   lex_param_t;

typedef enum tag_obj_id {

    OBJID_UNDEF,

    OBJID_RECORD_AREA,
    OBJID_RECORD_BUILDING,
    OBJID_RECORD_HIGHWAY,
    OBJID_RECORD_END,
    OBJID_ROLE_OUTER,
    OBJID_ROLE_INNER,
    OBJID_ROLE_COORDS,
    OBJID_ROLE_END,
    OBJID_TYPE_ASPHALT,
    OBJID_TYPE_WATER,
    OBJID_TYPE_FOREST,
    OBJID_TYPE_GRASS,
    OBJID_TYPE_GENERAL,
    OBJID_TYPE_MOUNTAIN,
    OBJID_TYPE_STONE,
    OBJID_TYPE_SAND,
    OBJID_TYPE_UNDEFINED,
    OBJID_TYPE_BUILDING,
    OBJID_TYPE_FOOTWAY,
    OBJID_TYPE_ROAD,
    OBJID_TYPE_SECONDARY,
    OBJID_TYPE_TRUNK,
    OBJID_TYPE_MOTORWAY,
    OBJID_TYPE_PRIMARY,
    OBJID_TYPE_TERTIARY,
    OBJID_TYPE_RAILWAY,
    OBJID_TYPE_RIVER,
    OBJID_TYPE_BRIDGE,
    OBJID_TYPE_TUNNEL,
    OBJID_AREA_SIZE,
    OBJID_COORD,

    OBJID_IDX_BEGIN,
    OBJID_IDX_RECT,
    OBJID_IDX_MEMBERS_BEGIN,
    OBJID_IDX_MEMBER,
    OBJID_IDX_MEMBERS_END,
    OBJID_IDX_END,

    OBJID_LAST_ID

}   obj_id_t;


#endif
