#include <iostream>
#include <cassert>
#include <string>
#include <sstream>
#include <limits>

#include "..\common\osm_processor.h"

map_storenode_t       g_nodes_list;
map_storeway_t        g_ways_list;
map_storerel_t        g_rels_list;

osm_processor_t       processor;

static void _log_building ( osm_id_t id, const list_obj_way_t& outers, const list_obj_way_t& inners ) {

    char position[80];

    return;

    std::cout << "BUILDING [ID " << id << "] " << std::endl;
        for ( auto it = outers.begin(); it != outers.end(); it++ ) {
            std::cout << "OUTER [ID " << it->id << "] ";
            for ( auto pos = it->refs.begin(); pos != it->refs.end(); pos++ ) {
                sprintf_s ( position, "%.7f %.7f; ", pos->lat, pos->lon );
                std::cout << position;
            }
            std::cout << std::endl;
        }
        for ( auto it = inners.begin(); it != inners.end(); it++ ) {
            std::cout << "INNER [ID " << it->id << "] ";
            for ( auto pos = it->refs.begin(); pos != it->refs.end(); pos++ ) {
                sprintf_s(position, "%.7f %.7f; ", pos->lat, pos->lon);
                std::cout << position;
            }
            std::cout << std::endl;
        }

    std::cout << "" << std::endl;
}

static void scan_child ( const storerels_t& rel ) {

    for (auto it = rel.refs.begin(); it != rel.refs.end(); it++) {

        switch ( it->ref ) {
            case REF_NODE:
                processor.mark_nodes ( it->id );
                break;
            case REF_WAY:
                processor.mark_way ( it->id );
                break;
            case REF_RELATION:
                processor.mark_relation ( it->id );
                break;
        }
    }
}

static void _reconstruct_bridge ( const storerels_t& rel ) {

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {
        
    }
}

static void _reconstruct_waterway ( const storerels_t& rel ) {

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

    }
}

static void _reconstruct_street ( const storerels_t& rel ) {

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

    }
}

static void _reconstruct_tunel ( const storerels_t& rel ) {

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

    }
}

static void _reconstruct_area_undef ( const storerels_t& rel ) {

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

    }
}

static void _reconstruct_building ( const storerels_t& rel ) {

    list_rel_refs_t outer_ways;
    list_rel_refs_t inner_ways;

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

        if ( it->ref == REF_NODE ) {
            continue;
        }

        switch ( it->role ) {
            case ROLE_EMPTY:
            case ROLE_PART:
            case ROLE_OUTER:
                outer_ways.push_back ( *it );
                break;
            case ROLE_INNER:
                inner_ways.push_back ( *it );
                break;
            default:
                break;
        }
    }

    list_obj_way_t  outers;
    list_obj_way_t  inners;

    processor.reconstruct_way ( outer_ways, outers );
    processor.reconstruct_way ( inner_ways, inners );

    _log_building ( rel.id, outers, inners );
}

static void _reconstruct_area ( const storerels_t& rel ) {

    list_rel_refs_t outer_ways;
    list_rel_refs_t inner_ways;

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

        if ( it->ref == REF_NODE ) {
            continue;
        }

        switch ( it->role ) {
            case ROLE_EMPTY:
            case ROLE_PART:
            case ROLE_OUTER:
                outer_ways.push_back ( *it );
                break;
            case ROLE_INNER:
                inner_ways.push_back ( *it );
                break;
            default:
                break;
        }
    }

    list_obj_way_t  outers;
    list_obj_way_t  inners;

    processor.reconstruct_way ( outer_ways, outers );
    processor.reconstruct_way ( inner_ways, inners );

    _log_building ( rel.id, outers, inners );
}

static void process_rel ( const storerels_t& rel ) {

    static int err_cnt = 0;
    list_rel_refs_t     outer_ways;   // +
    list_rel_refs_t     outer_areas;  // 
    list_rel_refs_t     inner_ways;   // 
    list_rel_refs_t     inner_areas;  // 
    list_rel_refs_t     undef_ways;   // +
    list_rel_refs_t     undef_areas;  // 

    if ( rel.in_use ) {
        return;
    }

    switch ( rel.type ) {

        case DRAW_REL_BRIDGE:
            _reconstruct_bridge ( rel );
            break;

        case DRAW_REL_WATERWAY:
            _reconstruct_waterway ( rel );
            break;

        case DRAW_REL_STREET:
            _reconstruct_street ( rel );
            break;

        case DRAW_REL_TUNNEL:
            _reconstruct_tunel ( rel );
            return;

        case DRAW_AREA_WATER:
        case DRAW_AREA_ASPHALT:
        case DRAW_AREA_GRASS:
        case DRAW_AREA_FORSET:
        case DRAW_AREA_SAND:
        case DRAW_AREA_MOUNTAIN:
        case DRAW_AREA_STONE:
            _reconstruct_area ( rel );
            break;

        case DRAW_BUILDING:
            _reconstruct_building ( rel );
            break;

        case DRAW_AREA_UNKNOWN:
            _reconstruct_area_undef ( rel );
            return;

        case DRAW_SKIP:
            return;

        default:
            assert(false);
            return;

    }

    return;

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

        switch ( it->role ) {
            case ROLE_OUTER:
                if ( it->ref == REF_WAY ) {
                    outer_ways.push_back ( *it );
                } else
                if ( it->ref == REF_RELATION ) {
                    outer_areas.push_back ( *it );
                } else {
                    assert ( false );
                }
                break;

            case ROLE_INNER:
                if ( it->ref == REF_WAY ) {
                    inner_ways.push_back ( *it );
                } else
                if ( it->ref == REF_RELATION ) {
                    inner_areas.push_back ( *it );
                } else {
                    assert ( false );
                }
                break;

            case ROLE_MAINSTREAM:
            case ROLE_SIDESTREAM:
                break;

            case ROLE_ACROSS:
                break;


            default:
                assert ( false );
        }
    }

    list_obj_way_t outers;
    list_obj_way_t inners;
    
    processor.reconstruct_way ( outer_ways, outers );
    processor.reconstruct_way ( inner_ways, inners );

    return;
}

int main() {

    processor.process_file ( "D:\\OSM_Extract\\prague.osm" );

    processor.enum_rels  ( scan_child );
    processor.enum_rels  ( process_rel );
    // processor.enum_ways  ( process_way );

    std::cout << "\r\n";

    return 0;
}
