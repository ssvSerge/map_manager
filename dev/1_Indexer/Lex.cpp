#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>

#include "Lex.h"

#define  TOKEN_STR          " \t"


tokens_t split ( const std::string src ) {

    tokens_t res;

    std::string token;
    std::istringstream iss(src);

    while ( iss ) {
        iss >> token;
        if (token.length() > 0) {
            res.push_back(token);
            token = "";
        }
    }

    return res;
}

Lex::Lex() {

}


void Lex::Load ( const char* const file_name ) {

    std::ifstream cfg_flie (file_name);

    if ( ! cfg_flie ) {
        return;
    }

    while ( !cfg_flie.eof() ) {

        std::string line;
        tokens_t    tokens;

        if ( ! std::getline ( cfg_flie, line) ) {
            break;
        }

        if ( line.size() == 0 ) {
            continue;
        }

        if (line[0] == '#') {
            continue;
        }

        tokens = split( line );

        if (tokens.size() < 4) {
            continue;
        }

        osm_node_t          osm_node;
        store_type_t        store_type;
        lex_list_item_t     lex_item;

        if ( ! map_type( tokens[0].c_str(), store_type ) ) {
            assert(false);
        }

        switch ( store_type ) {

            case LEX_TYPE_NAME:
                if ( ! _load_name(tokens, lex_item) ) {
                    assert(false);
                }
                break;

            case LEX_TYPE_TRANSPORT:
                if ( ! _load_transport(tokens, lex_item) ) {
                    assert(false);
                }
                break;

            case LEX_TYPE_UNKOWN:
            case LEX_TYPE_LAST:
                assert(false);
                break;
        }

        if ( ! map_node ( tokens[1].c_str(), osm_node ) ) {
            assert(false);
        }

        switch ( osm_node ) {

            case LEX_OSM_NODE_NODE:
                m_lex_nodes.push_back (lex_item);
                break;

            case LEX_OSM_NODE_WAY:
                m_lex_ways.push_back ( lex_item );
                break;

            case LEX_OSM_NODE_RELATION:
                m_lex_rels.push_back ( lex_item );
                break;

            case LEX_OSM_NODE_UNKNOWN:
            case LEX_OSM_NODE_LAST:
                assert(false);
                break;
        }






        
        
        

    }

}
