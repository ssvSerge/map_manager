#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <sstream>
#include <iomanip>

#include "..\common\osm_processor.h"

map_storenode_t       g_nodes_list;
map_storeway_t        g_ways_list;
map_storerel_t        g_rels_list;

osm_processor_t       processor;

#if 0

static void process_rels() {

        // DRAW_AREA_UNKNOWN,
        // DRAW_AREA_WATER,
        // DRAW_AREA_ASPHALT,
        // DRAW_AREA_GRASS,
        // DRAW_AREA_FORSET,
        // DRAW_AREA_SAND,
        // DRAW_AREA_MOUNTAIN,
        // DRAW_AREA_STONE,

        // DRAW_REL_STREET,
        // DRAW_REL_WATERWAY,
        // DRAW_REL_BRIDGE,
        // DRAW_REL_TUNNEL,
}

#endif

static void _get_role ( ref_role_t in_role, ref_role_t& out_role ) {

    out_role = ROLE_UNKNOWN;

    switch ( in_role ) {
        case ROLE_PART:
        case ROLE_OUTER:
            out_role = ROLE_OUTER;
            return;
        case ROLE_INNER:
            out_role = ROLE_INNER;
            return;
        case ROLE_UNKNOWN:
            break;
        default:
            assert(false);
            return;
    }
}

static void _process_area_building ( const storerels_t& area ) {

    ref_role_t      role;
    list_rel_refs_t outer;
    list_rel_refs_t inner;
    list_rel_refs_t undef;

    if ( area.level != 0 ) {
        return;
    }

    {   for (auto it = area.refs.begin(); it != area.refs.end(); it++) {

            if ( it->ref == REF_NODE ) {
                processor.mark_nodes ( it->id );
                continue;
            }

            if ( it->ref == REF_RELATION ) {
                processor.mark_relation ( it->id );
                continue;
            }

            _get_role ( it->role, role );

            if ( role == ROLE_OUTER ) {
                outer.push_back ( *it );
            } else
            if ( role == ROLE_INNER ) {
                inner.push_back ( *it );
            } else {
                undef.push_back ( *it );
            }

            processor.mark_way ( it->id );
        }
    }
        
    {   auto it = undef.begin();
        obj_way_t way;

        for ( auto it = undef.begin(); it != undef.end(); it++ ) {

            processor.mark_way(it->id);

            if ( ! processor.populate_way(it->id, false, way) ) {
                continue;
            }

            if ( way.level != 0 ) {
                continue;
            }

            if ( (way.type < DRAW_BUILDING_BEGIN) || (way.type > DRAW_BUILDING_END) ) {
                continue;
            }

            outer.push_back ( *it );
        }

    }

    list_obj_way_t outers;
    list_obj_way_t inners;

    processor.reconstruct_way ( inner, DRAW_BUILDING, true,  inners );
    processor.reconstruct_way ( outer, DRAW_BUILDING, false, outers );
}

void info_rel ( const storerels_t& rel ) {

    if (rel.in_use) {
        return;
    }

    switch (rel.type) {

        case DRAW_BUILDING:
            _process_area_building ( rel );
            break;

        case DRAW_AREA_UNKNOWN:
        case DRAW_AREA_WATER:
        case DRAW_AREA_ASPHALT:
        case DRAW_AREA_GRASS:
        case DRAW_AREA_FORSET:
        case DRAW_AREA_SAND:
        case DRAW_AREA_MOUNTAIN:
        case DRAW_AREA_STONE:
            assert ( false );
            break;

        default:
            break;
    }

}

void info_way ( const storeway_t& way ) {
}

void info_node ( const storenode_t& rel ) {
}

int main() {

    processor.process_file ( "D:\\OSM_Extract\\prague.osm" );

    processor.enum_rels  ( info_rel );
    processor.enum_ways  ( info_way );
    processor.enum_nodes ( info_node );
    std::cout << "\r\n";

    return 0;
}
