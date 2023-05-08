#ifndef __OSM_TYPES_H__
#define __OSM_TYPES_H__

#include <stdint.h>
#include <vector>
#include <list>
#include <map>

#define OSM_STR_MAX_LEN             (2048)
#define OSM_MAX_TAGS_CNT            (1024)

typedef uint64_t                    osm_obj_type_t;
typedef uint64_t                    osm_id_t;
typedef uint32_t                    osm_len_t;
typedef double                      osm_lon_t;
typedef double                      osm_lat_t;
typedef const char* const           osm_str_t;

typedef char alloc_str_t[OSM_STR_MAX_LEN];

typedef struct tag_osm_tag {
    alloc_str_t         k;
    alloc_str_t         v;
}   osm_tag_t;

typedef osm_tag_t osm_tags_list_t[OSM_MAX_TAGS_CNT];

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
        DRAW_PATH_TREEROW,
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
        DRAW_AREA_UNKNOWN,
        DRAW_AREA_WATER,
        DRAW_AREA_ASPHALT,
        DRAW_AREA_GRASS,
        DRAW_AREA_FORSET,
        DRAW_AREA_SAND,
        DRAW_AREA_MOUNTAIN,
        DRAW_AREA_STONE,
    DRAW_AREA_END,

    DRAW_REL_BEGIN,
        DRAW_REL_STREET,
        DRAW_REL_WATERWAY,
        DRAW_REL_BRIDGE,
        DRAW_REL_TUNNEL,
    DRAW_REL_END,

    DRAW_WATER,

    DRAW_LAST_ID
}   osm_draw_type_t;

typedef enum tag_ref_type {
    REF_UNKNOWN         = 0,
    REF_NODE            = 1,
    REF_WAY             = 2,
    REF_RELATION        = 3,
    REF_LAST
}   ref_type_t;

typedef enum tag_ref_role {
    ROLE_UNKNOWN        =  0,
    ROLE_INNER          =  1,
    ROLE_OUTER          =  2,
    ROLE_PART           =  3,
    ROLE_FORWARD        =  4,
    ROLE_BACKWARD       =  5,
    ROLE_ATERNATION     =  6,
    ROLE_INFO           =  7,
    ROLE_GUIDEPOST      =  8,
    ROLE_EMPTY          =  9,
    ROLE_MAINSTREAM     = 10,
    ROLE_SIDESTREAM     = 11,
    ROLE_ACROSS         = 12,
    ROLE_LAST
}   ref_role_t;

typedef struct tag_ref_way {
    osm_id_t            id;
    ref_type_t          ref;
    ref_role_t          role;
}   ref_item_t;

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

typedef std::vector<ref_item_t>     vector_rel_refs_t;

typedef std::list<ref_item_t>       list_rel_refs_t;

typedef struct tag_osm_tag_ctx {
    osm_tags_list_t     list;
    int                 cnt;
}   osm_tag_ctx_t;

typedef struct tag_osm_node_info {
    osm_id_t            id;             // Obj id
    osm_len_t           len;            // Obj Len
    osm_lat_t           lat;            // LAT
    osm_lon_t           lon;            // LON
    osm_draw_type_t     type;           // TRANSPORT, METRO
    osm_id_t            name;           // NAME_ID
    int                 level;          // Level <-1>, <0>, <1>, etc.
}   osm_node_info_t;

typedef struct tag_osm_obj_info {
    vector_rel_refs_t   refs;
    osm_tag_ctx_t       xml_tags;
    osm_node_info_t     node_info;
}   osm_obj_info_t;

typedef struct tag_osm_mapper {
    osm_str_t           k;
    osm_draw_type_t     v;
}   osm_mapper_t;

class storeinfo_t {
    public:
        storeinfo_t() {
            id     = static_cast<osm_id_t>(-1);
            type   = DRAW_UNKNOWN;
            name   = static_cast<osm_id_t>(-1);
            in_use = false;
            level  = 0;
        }

    public:
        osm_id_t           id;
        osm_draw_type_t    type;
        osm_id_t           name;
        int                level;
        bool               in_use;
};

class storenode_t : public storeinfo_t {
    public:
        storenode_t () {
            lon    =  0;
            lat    =  0; 
        }

    public:
        osm_lon_t          lon;
        osm_lat_t          lat;
};

typedef std::vector<storenode_t>        vector_storeinfo_t;
typedef std::list<storenode_t>          list_storeinfo_t;
typedef std::list<list_storeinfo_t>     list_list_storeinfo_t;

class storeway_t : public storeinfo_t {

    public:
        storeway_t() {
            refs.clear();
        }

    public:
        list_storeinfo_t   refs;
};

class storerels_t : public storeinfo_t {

    public:
        storerels_t() {
            refs.clear();
        }

    public:
        void populate ( osm_id_t _id );

    public:
        list_rel_refs_t refs;
};

typedef std::map<osm_id_t, storenode_t> map_storenode_t;
typedef std::map<osm_id_t, storeway_t>  map_storeway_t;
typedef std::map<osm_id_t, storerels_t> map_storerel_t;

class obj_t {
    public:
        osm_id_t           id;
        osm_draw_type_t    type;
        int                level;
};

class obj_node_t : public obj_t {
    public:
        osm_lat_t          lat;
        osm_lon_t          lon;
};

typedef std::list<obj_node_t> list_obj_node_t;

class obj_way_t : public obj_t {
    public:
        list_obj_node_t    refs;
};

typedef std::list<obj_t>      list_obj_t;
typedef std::list<list_obj_t> list_list_obj_t;

typedef std::list<obj_way_t>  list_obj_way_t;

class obj_rel_t : public obj_t {
    public:
        list_obj_t         refs;
};

#endif
