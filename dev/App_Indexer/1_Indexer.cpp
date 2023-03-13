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

std::map<osm_id_t, store_info_node_t>   g_nodes_list;
std::map<osm_id_t, store_info_way_t>    g_ways_list;
std::map<osm_id_t, store_info_rels_t>   g_rels_list;

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

int main() {

    processor.configure (_add_node, _add_way, _add_rel );
    processor.process_file("D:\\OSM_Extract\\prague.osm");

    return 0;
}
