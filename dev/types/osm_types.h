#ifndef __OSM_TYPES_H__
#define __OSM_TYPES_H__

#include <stdint.h>
#include <vector>
#include <list>

#define OSM_STR_MAX_LEN             (2048)
#define OSM_MAX_TAGS_CNT            (1024)

typedef uint64_t                    osm_obj_type_t;
typedef uint64_t                    osm_id_t;
typedef uint32_t                    osm_len_t;
typedef double                      osm_lon_t;
typedef double                      osm_lat_t;
typedef const char* const           osm_str_t;

typedef enum tag_osm_draw_type {

    DRAW_UNKNOWN,

    DRAW_STUFF_BEGIN,
        DRAW_SKIP,
    DRAW_STUFF_END,

    DRAW_START,

    DRAW_PENDING,

    DRAW_PATH_BEGIN,
        DRAW_PATH_RIVER,
        DRAW_PATH_MOTORWAY,
        DRAW_PATH_TRUNK,
        DRAW_PATH_PRIMARY,
        DRAW_PATH_SECONDARY,
        DRAW_PATH_TERTIARY,
        DRAW_PATH_ROAD,
        DRAW_PATH_FOOTWAY,
        DRAW_PATH_RAILWAY,
        DRAW_PATH_BRIDGE,
        DRAW_PATH_TUNNEL,
    DRAW_PATH_END,

    DRAW_BUILDING_BEGIN,
        DRAW_BUILDING,
        DRAW_BUILDING_OUTER,
        DRAW_BUILDING_INNER,
    DRAW_BUILDING_END,

    DRAW_AREA_BEGIN,
        DRAW_AREA_WATER,
        DRAW_AREA_ASPHALT,
        DRAW_AREA_GRASS,
        DRAW_AREA_FORSET,
        DRAW_AREA_SAND,
        DRAW_AREA_MOUNTAIN,
        DRAW_AREA_UNKNOWN,
        DRAW_AREA_STONE,
    DRAW_AREA_END,

    DRAW_WATER,

    DRAW_LAST_ID
}   osm_draw_type_t;

typedef enum tag_osm_node {
    XML_NODE_UNDEF,
    XML_NODE_ROOT,
    XML_NODE_NODE,
    XML_NODE_WAY,
    XML_NODE_REL,
    XML_NODE_TAG,
    XML_NODE_ND,
    XML_NODE_MEMBER,
    XML_NODE_SKIP,
}   osm_node_t;

typedef char alloc_str_t[OSM_STR_MAX_LEN];

typedef struct tag_osm_tag {
    alloc_str_t             k;
    alloc_str_t             v;
}   osm_tag_t;

typedef osm_tag_t osm_tags_list_t[OSM_MAX_TAGS_CNT];

typedef struct tag_osm_tag_ctx {
    osm_tags_list_t         list;
    int                     cnt;
}   osm_tag_ctx_t;

typedef enum tag_ref_type {
    REF_UNKNOWN         = 0,
    REF_NODE            = 1,
    REF_WAY             = 2,
    REF_RELATION        = 3,
    REF_LAST
}   ref_type_t;

typedef enum tag_ref_role {
    ROLE_UNKNOWN        = 0,
    ROLE_INNER          = 1,
    ROLE_OUTER          = 2,
    ROLE_PART           = 3,
    ROLE_FORWARD        = 4,
    ROLE_BACKWARD       = 5,
    ROLE_ATERNATION     = 6,
    ROLE_INFO           = 7,
    ROLE_GUIDEPOST      = 8,
    ROLE_LAST
}   ref_role_t;

typedef struct tag_ref_way {
    osm_id_t        id;
    ref_type_t      ref;
    ref_role_t      role;
}   ref_way_t;

typedef std::vector<ref_way_t>     vector_ways_t;

typedef struct tag_osm_node_info {
    osm_id_t            id;             // Obj id
    osm_len_t           len;            // Obj Len
    osm_lat_t           lat;            // LAT
    osm_lon_t           lon;            // LON
    osm_draw_type_t     type;           // TRANSPORT, METRO
    osm_id_t            name;           // NAME_ID
}   osm_node_info_t;

typedef struct tag_osm_mapper {
    osm_str_t               k;
    osm_draw_type_t         v;
}   osm_mapper_t;

typedef std::vector<osm_id_t>           vector_nodes_t;
typedef std::vector<vector_nodes_t>     vector_list_nodes_t;

typedef std::list<osm_id_t>             list_nodes_t;
typedef std::list<list_nodes_t>         list_list_nodes_t;

typedef std::list<ref_way_t>            list_ways_t;
typedef list_ways_t::iterator           ways_list_pos_t;

typedef std::list<list_ways_t>          list_list_ways_t;
typedef list_list_ways_t::iterator      list_list_ways_pos_t;

typedef std::list<vector_ways_t>        list_vector_ways_t;
typedef list_vector_ways_t::iterator    list_vector_ways_pos_t;

typedef struct tag_osm_obj_info {
    vector_ways_t       refs;
    osm_tag_ctx_t       xml_tags;
    osm_node_info_t     node_info;
}   osm_obj_info_t;

#endif
