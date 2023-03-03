#ifndef __OSM_TYPES_H__
#define __OSM_TYPES_H__

#include "osm_idx.h"

#include <string>
#include <vector>

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
    int                     cnt;
}   osm_tag_ctx_t;

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

typedef enum tag_lex_type {
    LEX_TYPE_UNKOWN         = 0,
    LEX_TYPE_NAME           = 1,
    LEX_TYPE_TRANSPORT      = 2,
    LEX_TYPE_LAST
}   lex_type_t;

typedef enum tag_lex_store {
    LEX_OSM_NODE_UNKNOWN    = 0,
    LEX_OSM_NODE_NODE       = 1,
    LEX_OSM_NODE_WAY        = 2,
    LEX_OSM_NODE_RELATION   = 3,
    LEX_OSM_NODE_LAST
}   lex_store_t;

typedef enum tag_ref_type {
    REF_UNKNOWN   = 0,
    REF_NODE      = 1,
    REF_WAY       = 2,
    REF_RELATION  = 3,
    REF_LAST
}   ref_type_t;

typedef struct tag_link_info {
    ref_type_t      ref;
    osm_id_t        id;
}   link_info_t;

typedef std::vector<std::string>     osm_param_t;
typedef std::vector<std::string>     osm_tokens_t;
typedef std::vector<link_info_t>     ref_list_t;

#endif
