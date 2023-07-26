#include <iostream>
#include <cassert>
#include <string>
#include <sstream>
#include <limits>
#include <vector>

#include "..\common\osm_processor.h"
#include "..\common\lex_keys.h"

map_storenode_t       g_nodes_list;
map_storeway_t        g_ways_list;
map_storerel_t        g_rels_list;

osm_processor_t       processor;

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

static double toRadians ( double val ) {

    return val / 57.295779513082325;
}

static double _delta ( double lon1, double lat1, double lon2, double lat2 ) {

    double R  = 6371e3;
    double f1 = toRadians(lat1);
    double f2 = toRadians(lat2);
    double dF = toRadians(lat2 - lat1);
    double dL = toRadians(lon2 - lon1);

    double a = sin(dF/2)*sin(dF/2) + cos(f1)*cos(f2) * sin(dL/2)*sin(dL/2);
    double c = 2 * atan2 ( sqrt(a), sqrt(1-a) );
    double d = R * c;

    return d;
}

static void _log_position ( osm_lat_t lat, osm_lon_t lon ) {

    char position[80];
    sprintf_s ( position, "%.7f %.7f", lat, lon );
    _log_key ( KEYNAME_COORDINATES, position);
}

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

    double lon_m     = _delta ( minLon, minLat, maxLon, minLat );
    double lat_m     = _delta ( minLon, minLat, minLon, maxLat );
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

template<typename Type>
static void _log_role ( const char* const out_type, draw_type_t draw_type, double area, const Type& refs ) {

    _log_key ( KEYNAME_ROLE,  out_type );
    _log_key ( KEYNAME_OTYPE, _type_to_str(draw_type) );
    _log_key ( KEYNAME_SIZE,  (int)(area) );

    auto it = refs.begin();
    while (it != refs.end()) {
        _log_position(it->lat, it->lon);
        it++;
    }

    _log_key ( KEYNAME_ROLE, KEYPARAM_END, true );
}

static void _log_header ( const char* const name, draw_type_t draw_type, size_t cnt, uint64_t osm_ref ) {

    if (cnt != 1) {
        cnt = cnt;
    }

    _log_key ( KEYNAME_RECORD,  name );
    _log_key ( KEYNAME_XTYPE,   _type_to_str(draw_type) );
    _log_key ( KEYNAME_CONTER,  cnt);
    _log_key ( KEYNAME_OSM_REF, osm_ref, true );
}

static void _log_footer() {
    _log_key ( KEYNAME_RECORD, KEYPARAM_END, true );
}

static void _log_area ( const char* name, const storeway_t& way ) {

    double area = _calc_area<list_storeinfo_t> ( way.refs );

    _log_header ( name, way.type, 1, way.id );
    _log_role   ( KEYPARAM_OUTER, way.type, area, way.refs );
    _log_footer ();
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

    size_t cnt = 0;

    cnt += way.refs.size();

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

    _log_area ( KEYNAME_AREA, way );
}

static void scan_child ( const storerels_t& rel ) {

    if ( rel.type == DRAW_SKIP ) {
        return;
    }

    for ( auto it = rel.refs.begin(); it != rel.refs.end(); it++ ) {

        switch ( it->ref ) {
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

    if ( way.id == 177921365 ) {
        stop_cnt++;
    }

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

    return 0;
}
