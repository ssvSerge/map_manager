#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <list>
#include <sstream>
#include <map>
#include <iomanip>

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

typedef std::map<osm_id_t, store_info_node_t>  nodes_map_t;
typedef std::map<osm_id_t, store_info_way_t>   ways_map_t;
typedef std::map<osm_id_t, store_info_rels_t>  rels_map_t;
typedef nodes_map_t::iterator                  nodes_pos_t;
typedef ways_map_t::const_iterator             ways_map_pos_t;
typedef rels_map_t::iterator                   rels_pos_t;
typedef std::list<ref_way_t>                   ways_list_t;
typedef ways_list_t::iterator                  ways_list_pos_t;
typedef std::list<osm_id_t>                    nodes_list_t;
typedef nodes_list_t::iterator                 nodes_list_pos_t;

const nodes_map_t    g_nodes_list;
const ways_map_t     g_ways_list;
const rels_map_t     g_rels_list;

osm_processor_t processor;

static bool _get_first_last ( const ways_list_pos_t& way, osm_id_t& first, osm_id_t& last ) {

    osm_id_t way_id = way->id;

    ways_map_pos_t it = g_ways_list.find(way_id);
    if ( it == g_ways_list.end() ) {
        return false;
    }

    const ref_list_nodes_t& refs = it->second.refs;

    if ( refs.size() < 2 ) {
        return false;
    }

    size_t b = 0;
    size_t e = refs.size() - 1;

    first = refs[b].id;
    last  = refs[e].id;

    return true;
}

static bool _way_append_tail_forward ( const ref_way_t& src, ways_list_t& dst ) {

    #if 0
    osm_id_t way_id = src->id;

    auto ptr = g_ways_list.find(way_id);

    if ( ptr == g_ways_list.end() ) {
        return false;
    }

    for ( size_t id = 1; id < ptr->second.refs.size(); id++ ) {
        dst.push_back( ptr->second.refs[id] );
    }
    #endif

    return true;
}

static bool _way_append_tail_backward ( const ways_list_pos_t& src, ways_list_t& dst ) {

    #if 0
    osm_id_t way_id = src->id;

    auto ptr = g_ways_list.find(way_id);

    if (ptr == g_ways_list.end()) {
        return false;
    }

    for ( int64_t id = ptr->second.refs.size()-2; id >= 0; id-- ) {
        dst.push_back(ptr->second.refs[id]);
    }
    #endif
        
    return true;
}

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

    nodes_map_t* ptr = const_cast<nodes_map_t*>(&g_nodes_list);
    (*ptr)[info.node_info.id] = new_obj;
}

static void _add_way ( osm_obj_info_t& info ) {

    static bool found_res = 0;

    store_info_way_t new_info;

    if (info.node_info.type >= DRAW_START) {
        _fix_area(info);
    }

    new_info.id   = info.node_info.id;
    new_info.type = info.node_info.type;
    new_info.refs = info.refs;

    ways_map_t* ptr = const_cast<ways_map_t*>(&g_ways_list);
    (*ptr)[new_info.id] = new_info;
}

static void _add_rel ( osm_obj_info_t& info ) {

    store_info_rels_t new_rel;

    new_rel.in_use  = false;
    new_rel.id      = info.node_info.id;
    new_rel.type    = info.node_info.type;
    new_rel.refs    = info.refs;

    rels_map_t* ptr = const_cast<rels_map_t*>(&g_rels_list);
    (*ptr)[info.node_info.id] = new_rel;
}

static bool _load_way ( const store_info_way_t& way, std::string& coord_list ) {

    bool ret_val = true;
    osm_id_t node_id;

    coord_list.clear();

    for (size_t i = 0; i < way.refs.size(); i++) {

        node_id = way.refs[i].id;

        auto it = g_nodes_list.find(node_id);

        if ( it == g_nodes_list.end() ) {
            ret_val = false;
            break;
        }

        // coord_list += "(";
        // coord_list += std::to_string(it->second.info.lat);
        // coord_list += " ";
        // coord_list += std::to_string(it->second.info.lon);
        // coord_list += ")";

        coord_list += "(";
        coord_list += std::to_string (node_id);
        coord_list += ")";
    }
    
    return ret_val;
}

static bool _merge_type1 ( const ref_way_t& src, nodes_list_t& refs ) {

    auto way = g_ways_list.find(src.id);
    if (way == g_ways_list.end()) {
        return false;
    }

    // 100, 101, 102
    //           102, 103, 104

    auto next_node = way->second.refs.begin();

    while (next_node != way->second.refs.end() ) {

        if ( next_node == way->second.refs.begin() ) {
            if (refs.size() > 0) {
                if (next_node->id == refs.back() ) {
                    next_node++;
                    continue;
                }
            }
        }

        refs.push_back(next_node->id);  
        next_node++;
    }

    return true;
}

static bool _merge_type2 ( const ref_way_t& src, nodes_list_t& refs ) {

    auto way = g_ways_list.find(src.id);
    if (way == g_ways_list.end()) {
        return false;
    }

    // 
    // 100, 101, 102
    //           104, 103, 102
    //

    auto next_node = way->second.refs.rbegin();

    while ( next_node != way->second.refs.rend() ) {

        if ( next_node == way->second.refs.rbegin() ) {
            if (refs.size() > 0) {
                if (next_node->id == refs.back() ) {
                    next_node++;
                    continue;
                }
            }
        }

        refs.push_back(next_node->id);  
        next_node++;
    }

    return true;
}

static bool _merge_type3 ( const ref_way_t& src, nodes_list_t& refs ) {

    auto way = g_ways_list.find(src.id);
    if (way == g_ways_list.end()) {
        return false;
    }

    // 
    //            105, 106, 107
    //  105, 104, 103
    // 

    auto next_node = way->second.refs.begin();

    while (next_node != way->second.refs.end()) {

        if (next_node == way->second.refs.begin()) {
            if (refs.size() > 0) {
                if (next_node->id == refs.front()) {
                    next_node++;
                    continue;
                }
            }
        }

        refs.push_front(next_node->id);
        next_node++;
    }

    return true;
}

static bool _merge_type4 ( const ref_way_t& src, nodes_list_t& refs ) {

    auto way = g_ways_list.find(src.id);
    if (way == g_ways_list.end()) {
        return false;
    }

    // 
    //            105, 106, 107
    //  103, 104, 105
    // 

    auto next_node = way->second.refs.rbegin();

    while (next_node != way->second.refs.rend()) {

        if (next_node == way->second.refs.rbegin()) {
            if (refs.size() > 0) {
                if (next_node->id == refs.front()) {
                    next_node++;
                    continue;
                }
            }
        }

        refs.push_front(next_node->id);
        next_node++;
    }

    return true;
}

static bool _merge_ways ( const ways_list_t& list, nodes_list_t& refs ) {

    bool        ret_val = true;
    bool        is_processed = false;

    osm_id_t    h1_f;
    osm_id_t    h1_l;
    osm_id_t    h2_f;
    osm_id_t    h2_l;

    ways_list_t ways_list = list;

    assert ( ways_list.size() > 0 );

    {   auto way = ways_list.front();
        ways_list.erase(ways_list.begin());
        _merge_type1(way, refs);
    }

    while ( ways_list.size() > 0 ) {

        h1_f = refs.front();
        h1_l = refs.back();

        auto way = ways_list.begin();

        is_processed = false;

        while ( way != ways_list.end() ) {

            _get_first_last ( way, h2_f, h2_l );

            if ( h1_l == h2_f ) {
                // 
                // 100, 101, 102
                //           102, 103, 104
                // 
                _merge_type1 ( (*way), refs );
                ways_list.erase(way);
                is_processed = true;
                break;
            } else
            if ( h1_l == h2_l ) {
                // 
                // 100, 101, 102
                //           104, 103, 102
                //
                _merge_type2 ( (*way), refs );
                ways_list.erase(way);
                is_processed = true;
                break;
            } else
            if ( h1_f == h2_f ) {
                // 
                //            105, 106, 107
                //  105, 104, 103
                // 
                _merge_type3 ( (*way), refs );
                ways_list.erase(way);
                is_processed = true;
                break;
            } else
            if ( h1_f == h2_l ) {
                // 
                //            105, 106, 107
                //  103, 104, 105
                // 
                _merge_type4 ( (*way), refs );
                ways_list.erase(way);
                is_processed = true;
                break;
            }

            way++;
        }

        if ( ! is_processed ) {
            ret_val = false;
            break;
        }

    }

    return ret_val;
}

static bool _process_relation_building ( const store_info_rels_t& obj ) {

    bool ret_val = true;

    std::stringstream building_shape;
    std::string       part;

    ways_list_t  outer_list;
    ways_list_t  inner_list;

    for ( size_t i = 0; i<obj.refs.size(); i++ ) {
        if (obj.refs[i].role == ROLE_OUTER) {
            outer_list.push_back( obj.refs[i] );
        } else
        if (obj.refs[i].role == ROLE_INNER) {
            inner_list.push_back(obj.refs[i] );
        } else {
            ret_val = false;
            assert(false);
        }
    }

    if (outer_list.size() == 0) {
        ret_val = false;
        assert(false);
    }

    nodes_list_t refs;
    bool io_res = _merge_ways ( outer_list, refs );

    if ( ! io_res ) {
        ret_val = false;
    }  else  {

        building_shape << "[BUILDING]" << std::endl;
        building_shape << "[ID " << obj.id << "]" << std::endl;
        building_shape << "[OUTER ";

        for (auto outer = refs.begin(); outer != refs.end(); outer++) {

            auto pos = g_nodes_list.find(*outer);
            if (pos == g_nodes_list.end()) {
                ret_val = false;
                break;
            }

            building_shape << "(" << std::setprecision(10) << pos->second.info.lat << " " << pos->second.info.lon << ") ";
            // building_shape << "(" << pos->second.info.id << ") ";
        }
        building_shape << std::endl;
        building_shape << "[END]" << std::endl << std::endl;
    }

    std::cout << building_shape.str();

    return ret_val;
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
        pos++;
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
