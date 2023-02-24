#ifndef __OSM_TYPES_H__
#define __OSM_TYPES_H__

#define OSM_STR_MAX_LEN             (2048)
#define OSM_MAX_TAGS_CNT            (1024)

typedef char alloc_str_t [ OSM_STR_MAX_LEN ];

typedef struct tag_osm_tag {
    alloc_str_t             k;
    alloc_str_t             v;
}   osm_tag_t;

typedef osm_tag_t osm_tags_list_t[OSM_MAX_TAGS_CNT];

typedef struct tag_osm_tag_ctx {
    osm_tags_list_t         list;
    int                     pos_1;
}   osm_tag_ctx_t;


void _clean_ctx ( osm_tag_ctx_t& ctx );


#endif
