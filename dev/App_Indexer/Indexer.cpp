﻿#include <iostream>
#include <cassert>
#include <string>
#include <sstream>
#include <limits>
#include <vector>

#include <osm_processor.h>
#include <geo_projection.h>
#include <lex_keys.h>

osm_processor_t             processor;

//-----------------------------------------------------------------//

static const char* _type_to_str ( draw_type_t type ) {

    switch (type) {

        // Area
        case DRAW_AREA_UNKNOWN:
            return "GENERAL";
            break;
        case DRAW_AREA_WATER:
            return "WATER";
            break;
        case DRAW_AREA_ASPHALT:
            return "ASPHALT";
            break;
        case DRAW_AREA_GRASS:
            return "GRASS";
            break;
        case DRAW_AREA_FORSET:
            return "FOREST";
            break;
        case DRAW_AREA_SAND:
            return "SAND";
            break;
        case DRAW_AREA_MOUNTAIN:
            return "MOUNTAIN";
            break;
        case DRAW_AREA_STONE:
            return "STONE";
            break;

        case DRAW_BUILDING:
            return "BUILDING";
            break;
        case DRAW_BUILDING_OUTER:
            return "BUILDING";
            break;
        case DRAW_BUILDING_INNER:
            return "BUILDING";
            break;

        case DRAW_PATH_RIVER:
            return "RIVER";
            break;
        case DRAW_PATH_MOTORWAY:
            return "MOTORWAY";
            break;
        case DRAW_PATH_TRUNK:
            return "TRUNK";
            break;
        case DRAW_PATH_PRIMARY:
            return "PRIMARY";
            break;
        case DRAW_PATH_SECONDARY:
            return "SECONDARY";
            break;
        case DRAW_PATH_TERTIARY:
            return "TERTIARY";
            break;
        case DRAW_PATH_ROAD:
            return "ROAD";
            break;
        case DRAW_PATH_FOOTWAY:
            return "FOOTWAY";
            break;
        case DRAW_PATH_SERVICE:
            return "SERVICE";
            break;
        case DRAW_PATH_UNCLIASSIFIED:
            return "UNCLIASSIFIED";
            break;
        case DRAW_PATH_RESIDENTIAL:
            return "RESIDENTIAL";
            break;
        case DRAW_PATH_STREET:
            return "STREET";
            break;
        case DRAW_PATH_PEDISTRAN:
            return "PEDISTRAN";
            break;
        case DRAW_PATH_TRACK:
            return "TRACK";
            break;
        case DRAW_PATH_STEPS:
            return "STEPS";
            break;
        case DRAW_PATH_PATH:
            return "PATH";
            break;
        case DRAW_PATH_TREEROW:
            return "TREEROW";
            break;
        case DRAW_PATH_RAILWAY:
            return "RAILWAY";
            break;
        case DRAW_PATH_BRIDGE:
            return "BRIDGE";
            break;
        case DRAW_PATH_TUNNEL:
            return "TUNNEL";
            break;

        case DRAW_PENDING:
            return "GENERAL";
            break;

        case DRAW_SKIP:
            return "UNDEFINED";
            break;

        default:
            return "UNDEFINED";
            break;
    }
}

//-----------------------------------------------------------------//

static void _log_key ( const char* const name, const char* const value, bool cr = false ) {

    std::cout << name;
    std::cout << ":";
    std::cout << value;
    std::cout << ";";

    if (cr) {
        std::cout << std::endl;
    }
}

static void _log_key ( const char* const name, size_t value, bool cr = false ) {

    char tmp[32];

    sprintf_s ( tmp, sizeof(tmp), "%zd", value );

    _log_key ( name, tmp, cr );
}

static void _log_position ( osm_lat_t lat, osm_lon_t lon ) {

    char position[ 160 ];

    double projection_y;
    double projection_x;

    geo_2_proj ( lon, lat, projection_x, projection_y );

    sprintf_s ( position, "%.7f %.7f %.1f %.1f", lat, lon, projection_y, projection_x );
    // sprintf_s(position, "%.7f %.7f", lat, lon );

    _log_key ( KEYNAME_COORDINATES, position );
}

static void _log_header ( const char* const name, draw_type_t draw_type, size_t cnt, uint64_t osm_ref1 ) {

    static uint64_t map_record = 0;
    map_record++;

    _log_key(KEYNAME_RECORD, name);
    _log_key(KEYNAME_XTYPE, _type_to_str(draw_type));
    _log_key(KEYNAME_CONTER, cnt);
    _log_key(KEYNAME_OSM_REF, osm_ref1, true );
}

static void _log_footer() {
    _log_key(KEYNAME_RECORD, KEYPARAM_END, true);
}

template<typename Type>
static void _log_role(const char* const out_type, draw_type_t draw_type, double area, const Type& refs) {

    _log_key(KEYNAME_ROLE, out_type);
    _log_key(KEYNAME_OTYPE, _type_to_str(draw_type));
    _log_key(KEYNAME_SIZE, (int)(area));

    auto it = refs.begin();
    while (it != refs.end()) {
        _log_position(it->lat, it->lon);
        it++;
    }

    _log_key(KEYNAME_ROLE, KEYPARAM_END, true);
}

//-----------------------------------------------------------------//

template<typename Type>
double _calc_area ( const Type& refs ) {

    double  res      = 0;
    double  s        = 0;
    double  minLat   = 0;
    double  maxLat   = 0;
    double  minLon   = 0;
    double  maxLon   = 0;

    std::vector<double>  x;
    std::vector<double>  y;
    size_t n = refs.size();

    x.reserve ( refs.size() + 8 );
    y.reserve ( refs.size() + 8 );

    auto in_ptr = refs.begin();

    minLat = in_ptr->lat;
    maxLat = in_ptr->lat;
    minLon = in_ptr->lon;
    maxLon = in_ptr->lon;

    while ( in_ptr != refs.end() ) {
        minLat = std::min ( minLat, in_ptr->lat );
        maxLat = std::max ( maxLat, in_ptr->lat );
        minLon = std::min ( minLon, in_ptr->lon );
        maxLon = std::max ( maxLon, in_ptr->lon );
        in_ptr++;
    }

    double lon_m     = gps_distance ( minLon, minLat, maxLon, minLat );
    double lat_m     = gps_distance ( minLon, minLat, minLon, maxLat );
    double scale_lon = lon_m / (maxLon-minLon);
    double scale_lat = lat_m / (maxLat-minLat);
    
    double val;
    in_ptr = refs.begin();
    while (in_ptr != refs.end()) {

        val = (in_ptr->lat - minLat) * scale_lat;
        x.push_back (val);

        val = (in_ptr->lon - minLon) * scale_lon;
        y.push_back (val);

        in_ptr++;
    }

    for ( size_t i = 0; i < refs.size(); i++) {
        if ( i == 0 ) {
            s    = x[i] * ( y[n-1] - y[i+1] );
            res += s;
        } else
        if ( i == (n-1) ) {
            s    = x[i] * ( y[i-1] - y[0] );
            res += s;
        } else {
            s    = x[i] * ( y[i-1] - y[i+1] );
            res += s;
        }
    }

    res = abs ( res / 2 );

    return res;
}

//-----------------------------------------------------------------//

static void _log_area ( const char* name, const storeway_t& way ) {

    double area = _calc_area<list_storeinfo_t>(way.refs);

    _log_header(name, way.type, 1, way.id);
    _log_role(KEYPARAM_OUTER, way.type, area, way.refs);
    _log_footer();
    std::cout << std::endl;
}

static void _log_relation ( const char* name, const storerels_t& rel, const list_obj_way_t& out, const list_obj_way_t& inl ) {

    double area;
    size_t cnt = 0;

    cnt += out.size();
    cnt += inl.size();

    _log_header ( name, rel.type, cnt, rel.id );

    for (auto it = out.begin(); it != out.end(); it++ ) {
        area = _calc_area<list_obj_node_t> ( it->refs );
        _log_role ( KEYPARAM_OUTER, it->type, area, it->refs );
    }

    for (auto it = inl.begin(); it != inl.end(); it++) {
        area = _calc_area<list_obj_node_t> ( it->refs );
        _log_role ( KEYPARAM_INNER, it->type, area, it->refs );
    }

    _log_footer();

    std::cout << std::endl;
}

static void _log_road ( const storeway_t& way ) {

    size_t cnt = 1;

    _log_header ( KEYNAME_HIGHWAY, way.type, cnt, way.id );
    _log_key ( KEYNAME_ROLE, KEYPARAM_COORDS );

    auto it = way.refs.begin();
    while (it != way.refs.end()) {
        _log_position(it->lat, it->lon);
        it++;
    }

    _log_key ( KEYNAME_ROLE, KEYPARAM_END, true );
    _log_footer();

    std::cout << std::endl;
}

static void _log_road ( const osm_id_t id, const list_obj_way_t& ways ) {

    size_t cnt = 1;

    for (auto it = ways.cbegin(); it != ways.cend(); it++) {

        _log_header ( KEYNAME_HIGHWAY, it->type, cnt, id );
        _log_key ( KEYNAME_ROLE, KEYPARAM_COORDS );
        for ( auto coord_it = it->refs.cbegin(); coord_it != it->refs.cend(); coord_it++ ) {
            _log_position (coord_it->lat, coord_it->lon );

        }
        _log_key(KEYNAME_ROLE, KEYPARAM_END, true);
        _log_footer();
        std::cout << std::endl;
    }
}

//-----------------------------------------------------------------//

static void scan_child ( const storerels_t& rel ) {

    if (rel.type == DRAW_SKIP) {
        return;
    }

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

static void store_area ( const storeway_t& way ) {

    if (way.in_use) {
        return;
    }

    if (way.level != 0) {
        return;
    }

    if ((way.type <= DRAW_AREA_BEGIN) || (way.type >= DRAW_AREA_END)) {
        return;
    }

    _log_area(KEYNAME_AREA, way);
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

    _log_relation ( KEYNAME_AREA, rel, outers, inners );
}

static void store_building ( const storeway_t& way ) {

    if ( way.in_use ) {
        return;
    }

    if ( way.level != 0 ) {
        return;
    }

    if ( (way.type <= DRAW_BUILDING_BEGIN) || (way.type >= DRAW_BUILDING_END) ) {
        return;
    }

    _log_area ( KEYNAME_BUILDING, way );
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

    _log_relation ( KEYNAME_BUILDING, rel, outers, inners );
}

static void store_roads ( const storeway_t& way ) {

    static int stop_cnt = 0;

    if (way.id == 47586041) {
        stop_cnt++;
    }

    if (way.in_use) {
        return;
    }

    if (way.level != 0) {
        return;
    }

    if ((way.type <= DRAW_PATH_BEGIN) || (way.type >= DRAW_PATH_END)) {
        return;
    }

    _log_road(way);
}

static void store_rel_roads ( const storerels_t& rel ) {

    static int stop_cnt = 0;

    if (rel.level != 0) {
        return;
    }

    if (rel.id == 7822459) {
        stop_cnt++;
    }

    if ( (rel.type <= DRAW_REL_BEGIN) || (rel.type >= DRAW_REL_END)) {
        return;
    }

    list_rel_refs_t way_segments;

    for (auto it = rel.refs.begin(); it != rel.refs.end(); it++) {

        if (it->ref == REF_NODE) {
            continue;
        }

        switch (it->role) {
            case ROLE_STREET:
                way_segments.push_back(*it);
                break;
            default:
                break;
        }
    }

    list_obj_way_t  way;

    processor.reconstruct_way(way_segments, way);

    _log_road ( rel.id, way );
}


//-----------------------------------------------------------------//

int main ( int argc, char* argv[] ) {

    if ( argc != 2 ) {
        return -1;
    }

    processor.process_file ( argv[1] );
    
    processor.enum_rels  ( scan_child );
    processor.enum_ways  ( store_area );
    processor.enum_rels  ( store_rel_area );
    processor.enum_ways  ( store_building );
    processor.enum_rels  ( store_rel_building );
    processor.enum_ways  ( store_roads );
    processor.enum_rels  ( store_rel_roads );

    return 0;
}
