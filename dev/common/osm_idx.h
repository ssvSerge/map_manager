#ifndef __OSM_IDX_H__
#define __OSM_IDX_H__

#include <stdint.h>

#pragma pack (1)    

typedef uint64_t        osm_id_t;
typedef uint64_t        osm_off_t;
typedef uint32_t        osm_len_t;
typedef uint32_t        osm_type_t;
typedef uint32_t        osm_lon_t;
typedef uint32_t        osm_lat_t;

typedef struct tag_osm_node_info {
    osm_id_t            id;             // Obj id
    osm_len_t           len;            // Obj Len
    osm_lat_t           lat;            // LAT
    osm_lon_t           lon;            // LON
    uint64_t            type;           // TRANSPORT, METRO
    osm_id_t            name;           // NAME_ID
}   osm_node_info_t;

typedef struct tag_osm_way_info {
    osm_id_t            id;             // 
    osm_type_t          type;           // 
    osm_id_t            name;           // 
    uint32_t            cnt;            // 
    osm_id_t            list[1];        // 
}   osm_way_info_t;

typedef struct tag_osm_rel_item {
    uint32_t            rel_type;       // 
    osm_id_t            rel_id;         // 
    osm_type_t          rel_role;       // 
}   osm_rel_item_t;



typedef struct tag_osm_rel_info {
    osm_id_t            id;             // 
    osm_type_t          type;           // 
    osm_id_t            name;           // 
    osm_rel_item_t      list[1];        //
}   osm_rel_info_t;


typedef struct tag_idx {
    osm_id_t            id;             // 
    osm_off_t           off;            // 
}   idx_t;

typedef enum tag_osm_draw_type {
    DRAW_UNKNOWN,

    DRAW_SKIP,

    DRAW_MOTORWAY,
    DRAW_TRUNK,
    DRAW_PRIMARY,
    DRAW_SECONDARY,
    DRAW_TERTIARY,
    DRAW_ROAD,
    DRAW_FOOTWAY,
    DRAW_PATH,

    DRAW_ASPHALT,

    DRAW_LAST_ID
}   osm_draw_type_t;


#pragma pack ()

#endif
