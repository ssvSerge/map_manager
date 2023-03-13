#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <map>

#include "..\common\osm_processor.h"

class store_info_node_t {

    public:
        store_info_node_t() {

            in_use = false;

            info.id   = 0;
            info.lat  = 0;
            info.lon  = 0;
            info.name = 0;
            info.type = DRAW_UNKNOWN;
        }

    public:
        bool               in_use;
        osm_node_info_t    info;
};

class store_info_way_t {

    public:
        store_info_way_t() {
            in_use  = false;
            bad     = false;
            id      = 0;
            type    = DRAW_UNKNOWN;
        }

    public:
        bool                in_use;
        bool                bad;
        osm_id_t            id;
        osm_draw_type_t     type;
        ref_list_nodes_t    refs;
};

class store_info_rels_t {

    public:
        store_info_rels_t() {
            in_use  = false;
            bad     = false;
            id      = 0;
            type    = DRAW_UNKNOWN;
        }
    public:
        bool                in_use;
        bool                bad;
        osm_id_t            id;
        osm_draw_type_t     type;
        ref_list_nodes_t    refs;
};

typedef std::map<osm_id_t, store_info_node_t>  nodes_list_t;
typedef std::map<osm_id_t, store_info_way_t>   ways_list_t;
typedef std::map<osm_id_t, store_info_rels_t>  rels_list_t;
typedef nodes_list_t::iterator                 nodes_pos_t;
typedef ways_list_t::iterator                  ways_pos_t;
typedef rels_list_t::iterator                  rels_pos_t;

nodes_list_t   g_nodes_list;
ways_list_t    g_ways_list;
rels_list_t    g_rels_list;

osm_processor_t processor;

static void _fix_area ( osm_obj_info_t& info ) {

    bool is_area = false;

    if ( (info.node_info.type >= DRAW_AREA_BEGIN) && (info.node_info.type <= DRAW_AREA_END) ) {
        is_area = true;
    }

    if (is_area) {
        if (info.refs.size() > 3) {

            size_t i1 = 0;
            size_t i2 = info.refs.size() - 1;

            if (info.refs[i1].id != info.refs[i2].id) {
                info.refs.push_back(info.refs[i1]);
            }
        }
    }

}

static void _add_node ( osm_obj_info_t& info ) {

    store_info_node_t new_obj;

    new_obj.info = info.node_info;
    g_nodes_list[info.node_info.id] = new_obj;
}

static void _add_way ( osm_obj_info_t& info ) {

    store_info_way_t new_info;

    if (info.node_info.type >= DRAW_START) {
        _fix_area(info);
    }

    new_info.id   = info.node_info.id;
    new_info.type = info.node_info.type;
    new_info.refs = info.refs;

    g_ways_list[new_info.id] = new_info;
}

static void _add_rel ( osm_obj_info_t& info ) {

    store_info_rels_t new_rel;

    new_rel.in_use  = false;
    new_rel.id      = info.node_info.id;
    new_rel.type    = info.node_info.type;
    new_rel.refs    = info.refs;

    g_rels_list[info.node_info.id] = new_rel;
}

static bool _load_way( const ways_pos_t& pos, std::string& coord_list ) {

    coord_list.clear();

    for (size_t i = 0; i < pos->second.refs.size(); i++) {

        pos->second.refs[i].id;


    }
    
}

static void _process_relation_building ( const store_info_rels_t& obj ) {

    std::vector<ref_way_t>  outer_list;
    std::vector<ref_way_t>  inner_list;

    for ( size_t i = 0; i<obj.refs.size(); i++ ) {
        if (obj.refs[i].role == ROLE_OUTER) {
            outer_list.push_back( obj.refs[i] );
        } else
        if (obj.refs[i].role == ROLE_INNER) {
            inner_list.push_back(obj.refs[i] );
        } else {
            assert(false);
        }
    }

    if (outer_list.size() == 0) {
        assert(false);
    }
    if (inner_list.size() == 0) {
        assert(false);
    }

    std::string  header;
    std::string  part;
    std::string  outer;
    std::string  inner;
    std::string  footer;

    header = "[BUILDING]";
    footer = "[END]";

    for (size_t i = 0; i < outer_list.size(); i++) {

        ways_pos_t building_outer = g_ways_list.find(outer_list[i].ref);
        if ( building_outer == g_ways_list.end() ) {
            break;
        }

        if ( !_load_way(building_outer, part) ) {
            break;
        }

    }
}

static void process_rels() {

    auto pos = g_rels_list.begin();

    while ( pos != g_rels_list.end() ) {

        if (pos->second.bad) {
            pos++;
            continue;
        }

        if (pos->second.in_use) {
            pos++;
            continue;
        }

        if ( pos->second.type < DRAW_START ) {
            pos++;
            continue;
        }

        _process_relation_building ( pos->second );
    }

}

static void process_ways() {

}

static void process_nodes() {

}

int main() {

    processor.configure (_add_node, _add_way, _add_rel );
    processor.process_file("D:\\OSM_Extract\\prague.osm");

    process_rels();
    process_ways();
    process_nodes();

    return 0;
}
