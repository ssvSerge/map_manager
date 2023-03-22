#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <list>
#include <sstream>
#include <map>
#include <iomanip>

#include "..\common\osm_processor.h"

class storenode_t {

    public:
        storenode_t() {

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

class storeway_t {

    public:
        storeway_t() {
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
        vector_ways_t       refs;
};

class storerels_t {

    public:
        storerels_t() {
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
        vector_ways_t       refs;
};

typedef std::map<osm_id_t, storenode_t>         map_storenode_t;
typedef map_storenode_t::iterator               map_storenode_pos_t;

typedef std::list<storenode_t>                  list_storenode_t;
typedef list_storenode_t::iterator              list_storenode_pos_t;

typedef std::list< list_storenode_t>            list_list_storenode_t;
typedef list_list_storenode_t::iterator         list_list_storenode_pos_t;

typedef std::map<osm_id_t, storeway_t>          map_storeway_t;
typedef map_storeway_t::const_iterator          map_storeway_pos_t;

typedef std::list<storeway_t>                   list_storeway_t;
typedef list_storeway_t::iterator               list_storeway_pos_t;

typedef std::map<osm_id_t, storerels_t>         map_storerel_t;
typedef map_storerel_t::iterator                map_storerel_pos_t;

class storewayex_t {
    public:
        storewayex_t() {
            type = DRAW_UNKNOWN;
        }

    public:
        osm_draw_type_t     type;
        list_storenode_t    ref;
};

typedef std::vector<storewayex_t>               vector_storewayex_t;
typedef vector_storewayex_t::iterator           vector_storewayex_pos_t;
typedef std::list<vector_storewayex_t>          list_vector_storewayex_t;
typedef list_vector_storewayex_t::iterator      list_vector_storewayex_pos_t;

const map_storenode_t       g_nodes_list;
const map_storeway_t        g_ways_list;
const map_storerel_t        g_rels_list;

osm_processor_t             processor;

static void _fix_area ( osm_obj_info_t& info ) {

    bool is_area = false;

    if ((info.node_info.type >= DRAW_AREA_BEGIN) && (info.node_info.type <= DRAW_AREA_END)) {
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

    storenode_t new_obj;

    new_obj.info = info.node_info;

    map_storenode_t* ptr = const_cast<map_storenode_t*>(&g_nodes_list);
    (*ptr)[info.node_info.id] = new_obj;
}

static void _add_way ( osm_obj_info_t& info ) {

    static bool found_res = 0;

    storeway_t new_info;

    if (info.node_info.type >= DRAW_START) {
        _fix_area(info);
    }

    new_info.id = info.node_info.id;
    new_info.type = info.node_info.type;
    new_info.refs = info.refs;

    map_storeway_t* ptr = const_cast<map_storeway_t*>(&g_ways_list);
    (*ptr)[new_info.id] = new_info;
}

static void _add_rel ( osm_obj_info_t& info ) {

    storerels_t new_rel;

    new_rel.in_use = false;
    new_rel.id     = info.node_info.id;
    new_rel.type   = info.node_info.type;
    new_rel.refs   = info.refs;

    map_storerel_t* ptr = const_cast<map_storerel_t*>(&g_rels_list);
    (*ptr)[info.node_info.id] = new_rel;
}

#if 0

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

static bool _merge_ways ( ways_list_t& ways_list, list_list_ways_t& refs_list ) {

    bool         ret_val = true;

    #if 0
    bool         is_processed = false;

    list_ways1_t refs;

    osm_id_t     h1_f;
    osm_id_t     h1_l;
    osm_id_t     h2_f;
    osm_id_t     h2_l;

    if ( ways_list.size() == 0 ) {
        return true;
    }

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

    refs_list.push_back(refs);

    #endif

    return ret_val;
}

#endif

static void _get_first_last ( const list_nodes_t& ref, osm_id_t& f, osm_id_t& l ) {

    f = -1;
    l = -1;

    assert ( ref.size() > 2 );

    f = ref.front();
    l = ref.back();
}

static void _get_first_last ( const vector_ways_t& refs, osm_id_t& f, osm_id_t& l ) {

    f = -1;
    l = -1;

    if ( refs.size() < 2 ) {
        return;
    }
    
    f = refs.front().id;
    l = refs.back().id;
}

static void _cut_areas ( list_nodes_t& ids, vector_storewayex_t& ways ) {

    auto it = ids.begin();

    while ( it != ids.end() ) {

        auto id = (*it);

        auto way_info = g_ways_list.find ( id );

        if ( way_info == g_ways_list.end() ) {
            it++;
            continue;
        }

        if ( way_info->second.refs.size() < 2 ) {
            it++;
            continue;
        }

        if (way_info->second.refs.front().id != way_info->second.refs.back().id ) {
            it++;
            continue;
        }

        bool is_failed = false;

        storenode_t     new_node;
        storewayex_t    new_way;

        new_way.type = way_info->second.type;

        for ( size_t i = 0; i < way_info->second.refs.size(); i++ ) {
        
            auto node_id = way_info->second.refs[i].id;

            auto node_ptr = g_nodes_list.find ( node_id );
            if ( node_ptr == g_nodes_list.end() ) {
                is_failed = true;
                break;
            }

            new_node.info = node_ptr->second.info;
            new_way.ref.push_back(new_node);
        }

        if ( !is_failed ) {
            ways.push_back(new_way);
        }

        it = ids.erase ( it );
    }
}

static bool _merge_type1 ( const vector_ways_t& src, list_nodes_t& dst ) {

    // 100, 101, 102
    //           102, 103, 104

#if 0
    auto next_node = way->second.refs.begin();

    while (next_node != way->second.refs.end()) {

        if (next_node == way->second.refs.begin()) {
            if (refs.size() > 0) {
                if (next_node->id == refs.back()) {
                    next_node++;
                    continue;
                }
            }
        }

        refs.push_back(next_node->id);
        next_node++;
    }

#endif

    return true;
}

static bool _merge_type2 ( const vector_ways_t& src, list_nodes_t& dst ) {

    // 100, 101, 102
    //           104, 103, 102

    auto next_node = src.rbegin();

    while ( next_node != src.rend() ) {
        if ( next_node == src.rbegin()) {
            if ( dst.size() > 0 ) {
                if ( next_node->id == dst.back() ) {
                    next_node++;
                    continue;
                }
            }
        }

        dst.push_back(next_node->id);
        next_node++;
    }

    return true;
}

static bool _merge_type3 ( const vector_ways_t& src, list_nodes_t& dst ) {

    //            105, 106, 107
    //  105, 104, 103

    auto next_node = src.begin();

    while (next_node != src.end()) {

        if ( next_node == src.begin() ) {
            if (dst.size() > 0) {
                if (next_node->id == dst.front()) {
                    next_node++;
                    continue;
                }
            }
        }

        dst.push_front(next_node->id);
        next_node++;
    }

    return true;
}

static bool _merge_type4 ( const vector_ways_t& src, list_nodes_t& dst ) {

    //            105, 106, 107
    //  103, 104, 105

#if 0
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

#endif

    return true;
}

static void _reconstruct ( list_nodes_t& ids, vector_storewayex_t& ways ) {

    osm_id_t     h1_f;
    osm_id_t     h1_l;
    osm_id_t     h2_f;
    osm_id_t     h2_l;

    list_nodes_t new_way;

    if ( ids.size() == 0 ) {
        return;
    }

    auto first_entry = ids.front();
    ids.erase( ids.begin() );

    bool is_processed;

    while ( ids.size() > 0 ) {

        auto segment = ids.front();

        auto src_way_ptr = g_ways_list.find(segment);
        assert ( src_way_ptr != g_ways_list.end() );

        const vector_ways_t& refs = src_way_ptr->second.refs;

        _get_first_last ( new_way, h1_f, h1_l );
        _get_first_last ( refs,    h2_f, h2_l );

        is_processed = false;

        if ( h1_l == h2_f ) {
            // 100, 101, 102
            //           102, 103, 104
            _merge_type1 ( refs, new_way );
            is_processed = true;
            break;
        } else
        if ( h1_l == h2_l ) {
            // 100, 101, 102
            //           104, 103, 102
            _merge_type2 ( refs, new_way );
            is_processed = true;
            break;
        } else
        if ( h1_f == h2_f ) {
            //            105, 106, 107
            //  105, 104, 103
            _merge_type3 ( refs, new_way );
            is_processed = true;
            break;
        } else
        if ( h1_f == h2_l ) {
            //            105, 106, 107
            //  103, 104, 105
            _merge_type4 ( refs, new_way );
            is_processed = true;
            break;
        }

        assert(is_processed);

        ids.erase ( ids.begin() );
    }

}

static void _merge_areas ( list_nodes_t& ids, vector_storewayex_t& ways ) {

    _cut_areas   ( ids, ways );
    _reconstruct ( ids, ways );
}

static bool _process_relation_building(const storerels_t& obj) {

    bool ret_val = true;

//  std::stringstream   building_shape;
//  std::string         part;

    list_nodes_t            rel_outer_way;
    vector_storewayex_t     out_outer_list;
    list_nodes_t            rel_inner_way;
    vector_storewayex_t     out_inner_list;

    for (size_t i = 0; i < obj.refs.size(); i++) {
        if (obj.refs[i].role == ROLE_OUTER) {
            rel_outer_way.emplace_back ( obj.refs[i].id );
        } else
        if (obj.refs[i].role == ROLE_INNER) {
            rel_inner_way.emplace_back ( obj.refs[i].id );
        } else {
            ret_val = false;
            assert(false);
        }
    }

    if (rel_outer_way.size() == 0) {
        ret_val = false;
        assert(false);
    }

    _merge_areas ( rel_outer_way, out_outer_list );
    _merge_areas ( rel_inner_way, out_inner_list );

    #if 0
    nodes_list_t refs;
    bool io_res = _merge_ways(outer_list, refs);

    if (!io_res) {
        ret_val = false;
    }
    else {

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
    #endif

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
