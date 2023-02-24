#ifndef __LEX_H__
#define __LEX_H__

#include <string>

#include "..\common\osm_idx.h"

#define KEY_NODE__NODE         "NODE"
#define KEY_NODE__WAY          "WAY"
#define KEY_NODE__REL          "REL"

#define KEY_MAP__NAME          "NAME"
#define KEY_MAP__TRANSPORT     "TRANSPORT"
#define KEY_MAP__RELATION      "REL"


typedef enum tag_store_type {
    LEX_TYPE_UNKOWN         = 0,
    LEX_TYPE_NAME           = 1,
    LEX_TYPE_TRANSPORT      = 2,
    LEX_TYPE_LAST
}   store_type_t;

typedef enum tag_osm_node {
    LEX_OSM_NODE_UNKNOWN    = 0,
    LEX_OSM_NODE_NODE       = 1,
    LEX_OSM_NODE_WAY        = 2,
    LEX_OSM_NODE_RELATION   = 3,
    LEX_OSM_NODE_LAST
}   osm_node_t;


typedef struct tag_lex_list_item {
    store_type_t        type;
    std::string         k;
    std::string         v;
    std::string         o;
}   lex_list_item_t;

typedef std::vector<lex_list_item_t>   lex_list_t;

typedef struct tag_lex_pair {
    std::string         k;
    std::string         v;
}   lex_pair_t;

typedef std::vector<std::string> tokens_t;


class Lex {

    public:
        Lex();

    public:
        void    Load ( const char* const file_name );

    public:
        void    ResolveNodeType ();
        void    ResolveWayType  ();
        void    ResolveRelType  ();

    private:
        bool    map_node ( const char* const text, osm_node_t& osm_node );
        bool    map_type ( const char* const text, store_type_t& lex_type );

        bool    _load_name ( const tokens_t& tokens, lex_list_item_t& info );
        bool    _load_transport ( const tokens_t& tokens, lex_list_item_t& info );

    private:
        lex_list_t      m_lex_nodes;
        lex_list_t      m_lex_ways;
        lex_list_t      m_lex_rels;
};

#endif

