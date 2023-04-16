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

static const char* _type_to_str ( osm_draw_type_t type ) {

    switch (type) {

        // Area
        case DRAW_AREA_UNKNOWN:
            return "GENERAL  ";
            break;
        case DRAW_AREA_WATER:
            return "WATER    ";
            break;
        case DRAW_AREA_ASPHALT:
            return "ASPHALT  ";
            break;
        case DRAW_AREA_GRASS:
            return "GRASS    ";
            break;
        case DRAW_AREA_FORSET:
            return "FOREST   ";
            break;
        case DRAW_AREA_SAND:
            return "SAND     ";
            break;
        case DRAW_AREA_MOUNTAIN:
            return "MOUNTAIN ";
            break;
        case DRAW_AREA_STONE:
            return "STONE    ";
            break;

        case DRAW_BUILDING:
            return "BUILDING ";
            break;
        case DRAW_BUILDING_OUTER:
            return "BUILDING ";
            break;
        case DRAW_BUILDING_INNER:
            return "BUILDING ";
            break;

        case DRAW_PATH_RIVER:
            return "RIVER    ";
            break;
        case DRAW_PATH_MOTORWAY:
            return "MOTORWAY ";
            break;
        case DRAW_PATH_TRUNK:
            return "TRUNK    ";
            break;
        case DRAW_PATH_PRIMARY:
            return "PRIMARY  ";
            break;
        case DRAW_PATH_SECONDARY:
            return "SECONDARY";
            break;
        case DRAW_PATH_TERTIARY:
            return "TERTIARY ";
            break;
        case DRAW_PATH_ROAD:
            return "ROAD     ";
            break;
        case DRAW_PATH_FOOTWAY:
            return "FOOTWAY  ";
            break;
        case DRAW_PATH_RAILWAY:
            return "RAILWAY  ";
            break;
        case DRAW_PATH_BRIDGE:
            return "BRIDGE   ";
            break;
        case DRAW_PATH_TUNNEL:
            return "YUNNEL   ";
            break;

        case DRAW_PENDING:
            return "GENERAL  ";
            break;

        default:
            return "UNDEFINED";
            break;
    }
}

static void _log_position ( osm_lat_t lat, osm_lon_t lon ) {

    char position [80];
    sprintf_s ( position, "%.7f %.7f; ", lat, lon );
    std::cout << position;
}

static void _log_area ( const char* name, const storeway_t& way ) {

    // std::cout << "; " << way.id << std::endl;
    std::cout << name << std::endl;
    std::cout << "OUTER " << _type_to_str(way.type) << " ";
    // std::cout << "COORD ";

    auto it = way.refs.begin();
    while (it != way.refs.end()) {
        _log_position ( it->lat, it->lon );
        it++;
    }
    std::cout << std::endl << std::endl;
}

static void _log_relation ( const char* name, const storerels_t& rel, const list_obj_way_t& out, const list_obj_way_t& inl ) {

    std::cout << name << std::endl;

    for (auto it = out.begin(); it != out.end(); it++ ) {
        std::cout << "OUTER ";
        std::cout << _type_to_str(rel.type) << " ";
        for ( auto coord = it->refs.begin(); coord != it->refs.end(); coord++ ) {
            _log_position(coord->lat, coord->lon);
        }
        std::cout << std::endl;
    }

    for (auto it = inl.begin(); it != inl.end(); it++) {
        std::cout << "INNER ";
        std::cout << _type_to_str(it->type) << " ";
        for (auto coord = it->refs.begin(); coord != it->refs.end(); coord++) {
            _log_position(coord->lat, coord->lon);
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

static void _log_road ( const storeway_t& way ) {

    std::cout << "HIGHWAY" << std::endl;
    std::cout << _type_to_str(way.type) << "       ";

    auto it = way.refs.begin();
    while (it != way.refs.end()) {
        _log_position(it->lat, it->lon);
        it++;
    }
    std::cout << std::endl << std::endl;
}

static void store_area ( const storeway_t& way ) {

    if ( way.in_use ) {
        return;
    }

    if ( way.level != 0 ) {
        return;
    }

    if ( (way.type <= DRAW_AREA_BEGIN) || (way.type >= DRAW_AREA_END) ) {
        return;
    }

    _log_area ( "AREA", way );
}

static void scan_child ( const storerels_t& rel ) {

    for (auto it = rel.refs.begin(); it != rel.refs.end(); it++) {

        switch (it->ref) {
            case REF_NODE:
                processor.mark_nodes(it->id);
                break;
            case REF_WAY:
                processor.mark_way(it->id);
                break;
            case REF_RELATION:
                processor.mark_relation(it->id);
                break;
            }
    }
}

static void store_rel_area ( const storerels_t& rel ) {

    if ( rel.level != 0 ) {
        return;
    }

    if ( (rel.type <= DRAW_AREA_BEGIN) || (rel.type >= DRAW_AREA_END)) {
        return;
    }

    list_rel_refs_t outer_ways;
    list_rel_refs_t inner_ways;

    for (auto it = rel.refs.begin(); it != rel.refs.end(); it++) {

        if (it->ref == REF_NODE) {
            continue;
        }

        switch (it->role) {
            case ROLE_EMPTY:
            case ROLE_PART:
            case ROLE_OUTER:
                outer_ways.push_back(*it);
                break;
            case ROLE_INNER:
                inner_ways.push_back(*it);
                break;
            default:
                break;
        }
    }

    list_obj_way_t  outers;
    list_obj_way_t  inners;

    processor.reconstruct_way ( outer_ways, outers );
    processor.reconstruct_way ( inner_ways, inners );

    _log_relation ( "AREA", rel, outers, inners );
}

static void store_building ( const storeway_t& way ) {

    if (way.in_use) {
        return;
    }

    if (way.level != 0) {
        return;
    }

    if ( (way.type <= DRAW_BUILDING_BEGIN) || (way.type >= DRAW_BUILDING_END) ) {
        return;
    }

    _log_area ( "BUILDING", way );
}

static void store_rel_building ( const storerels_t& rel ) {

    if (rel.level != 0) {
        return;
    }

    if ( (rel.type <= DRAW_BUILDING_BEGIN) || (rel.type >= DRAW_BUILDING_END) ) {
        return;
    }

    list_rel_refs_t outer_ways;
    list_rel_refs_t inner_ways;

    for (auto it = rel.refs.begin(); it != rel.refs.end(); it++) {

        if (it->ref == REF_NODE) {
            continue;
        }

        switch (it->role) {
        case ROLE_EMPTY:
        case ROLE_PART:
        case ROLE_OUTER:
            outer_ways.push_back(*it);
            break;
        case ROLE_INNER:
            inner_ways.push_back(*it);
            break;
        default:
            break;
        }
    }

    list_obj_way_t  outers;
    list_obj_way_t  inners;

    processor.reconstruct_way(outer_ways, outers);
    processor.reconstruct_way(inner_ways, inners);

    _log_relation ( "BUIDING", rel, outers, inners );
}

static void store_roads ( const storeway_t& way ) {

    if ( way.in_use ) {
        return;
    }

    if ( way.level != 0 ) {
        return;
    }

    if ( (way.type <= DRAW_PATH_BEGIN) || (way.type >= DRAW_PATH_END) ) {
        return;
    }

    _log_road ( way );
}

int main() {

    processor.process_file ( "D:\\OSM_Extract\\prague.osm" );
    // processor.process_file("C:\\Users\\serg\\Downloads\\map.osm");

    processor.enum_rels  ( scan_child );
    processor.enum_ways  ( store_area );
    processor.enum_rels  ( store_rel_area );

    processor.enum_ways  ( store_building );
    processor.enum_rels  ( store_rel_building );

    processor.enum_ways  ( store_roads );

    return 0;
}
