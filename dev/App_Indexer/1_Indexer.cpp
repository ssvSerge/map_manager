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

static void _get_first_last ( const list_nodes_t& refs, osm_id_t& f, osm_id_t& l ) {

    f = -1;
    l = -1;

    if ( refs.size() < 2 ) {
        return;
    }

    f = refs.front();
    l = refs.back();
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

static bool _merge_type1 ( const list_nodes_t& src, list_nodes_t& dst ) {

    // 
    // h1_l == h2_f
    // 
    // 100, 101, 102
    //           102, 103, 104
    // 

    auto next_node = src.begin();

    if (dst.size() > 0) {
        if ( (*next_node) == dst.back() ) {
            next_node++;
        }
    }

    while ( next_node != src.end() ) {
        dst.push_back( *next_node );
        next_node++;
    }

    return true;
}

static bool _merge_type2 ( const list_nodes_t& src, list_nodes_t& dst ) {

    //
    // h1_l == h2_l
    // 
    // 100, 101, 102
    //           104, 103, 102
    // 

    auto next_node = src.rbegin();

    if (dst.size() > 0) {
        if ( *next_node == dst.back()) {
            next_node++;
        }
    }

    while ( next_node != src.rend() ) {
        dst.push_back( *next_node );
        next_node++;
    }

    return true;
}

static bool _merge_type3 ( const list_nodes_t& src, list_nodes_t& dst ) {

    // 
    // h1_f == h2_f
    // 
    //           105, 106, 107
    // 105, 104, 103
    // 

    auto next_node = src.begin();

    if (dst.size() > 0) {
        if ( *next_node == dst.front()) {
            next_node++;
        }
    }

    while (next_node != src.end()) {
        dst.push_front( *next_node );
        next_node++;
    }

    return true;
}

static bool _merge_type4 ( const list_nodes_t& src, list_nodes_t& dst ) {

    // 
    // h1_f == h2_l
    // 
    //            105, 106, 107
    //  103, 104, 105
    // 

    auto next_node = src.rbegin();

    if (dst.size() > 0) {
        if ( *next_node == dst.front()) {
            next_node++;
        }
    }

    while (next_node != src.rend()) {
        dst.push_front( *next_node );
        next_node++;
    }

    return true;
}

static void _load_segment_data ( osm_id_t id, list_nodes_t& data ) {

    static int err_cnt = 0;
    data.clear();

    auto way = g_ways_list.find ( id );
    if ( way == g_ways_list.end() ) {
        std::cout << "-> Cannot find object: " << id << std::endl;
        err_cnt++;
        return;
    }

    for (size_t i = 0; i < way->second.refs.size(); i++) {
        data.push_back(way->second.refs[i].id );
    }
}

static void _reconstruct ( list_nodes_t& ids, osm_draw_type_t draw_type, vector_storewayex_t& ways ) {

    osm_id_t     h1_f;
    osm_id_t     h1_l;
    osm_id_t     h2_f;
    osm_id_t     h2_l;

    if ( ids.size() == 0 ) {
        return;
    }

    osm_id_t        segment_id;
    list_nodes_t    segment_refs_a;
    list_nodes_t    segment_refs_b;

    segment_id = ids.front();
    ids.erase ( ids.begin() );

    _load_segment_data ( segment_id, segment_refs_a );

    bool retry_required = true;
    bool is_merged      = false;

    while (retry_required) {

        retry_required = false;

        auto segment_ptr = ids.begin();
        while ( segment_ptr != ids.end() ) {

            segment_id = *segment_ptr;

            _load_segment_data ( segment_id, segment_refs_b );

            _get_first_last ( segment_refs_a, h1_f, h1_l );
            _get_first_last ( segment_refs_b, h2_f, h2_l );

            is_merged = false;

            if ( h1_l == h2_f ) {
                // 100, 101, 102
                //           102, 103, 104
                _merge_type1 ( segment_refs_b, segment_refs_a );
                is_merged = true;
            } else
            if ( h1_l == h2_l ) {
                // 100, 101, 102
                //           104, 103, 102
                _merge_type2 ( segment_refs_b, segment_refs_a );
                is_merged = true;
            } else
            if ( h1_f == h2_f ) {
                //            105, 106, 107
                //  105, 104, 103
                _merge_type3 ( segment_refs_b, segment_refs_a );
                is_merged = true;
            } else
            if ( h1_f == h2_l ) {
                //            105, 106, 107
                //  103, 104, 105
                _merge_type4 ( segment_refs_b, segment_refs_a );
                is_merged = true;
            }

            if ( !is_merged ) {
                segment_ptr++;
                continue;
            }

            retry_required = true;
            segment_ptr = ids.erase (segment_ptr);
        }
    }

    assert ( ids.size() == 0 );

    storewayex_t new_way;
    new_way.type = draw_type;

    auto it = segment_refs_a.begin();
    while ( it != segment_refs_a.end() ) {
        auto node_ref = g_nodes_list.find( (*it) );
        if (node_ref != g_nodes_list.end()) {
            storenode_t new_node;
            new_node.info = node_ref->second.info;
            new_way.ref.push_back(new_node);
        }
        it++;
    }

    ways.push_back ( new_way );
}

static void _merge_areas ( list_nodes_t& ids, osm_draw_type_t draw_type, vector_storewayex_t& ways ) {

    _cut_areas   ( ids, ways );
    _reconstruct ( ids, draw_type, ways );
}

static void _get_role ( const ref_way_t& way, ref_role_t& role ) {

    role = ROLE_UNKNOWN;

    switch ( way.role ) {
        case ROLE_PART:
        case ROLE_OUTER:
            role = ROLE_OUTER;
            return;
        case ROLE_INNER:
            role = ROLE_INNER;
            return;
        case ROLE_UNKNOWN:
            break;
        default:
            assert(false);
            return;
    }

    auto way_info = g_ways_list.find( way.id );
    if ( way_info == g_ways_list.end() ) {
        return;
    }

    if ( (way_info->second.type >= DRAW_BUILDING_BEGIN) && (way_info->second.type <= DRAW_BUILDING_END) ) {
        role = ROLE_OUTER;
        return;
    }

    assert(false);
}

static void _process_rel_building ( bool& is_processed, const storerels_t& obj ) {

    list_nodes_t            rel_outer_way;
    vector_storewayex_t     out_outer_list;
    list_nodes_t            rel_inner_way;
    vector_storewayex_t     out_inner_list;
    ref_role_t              way_role;

    if ( is_processed ) {
        return;
    }

    if ( ( obj.type < DRAW_BUILDING_BEGIN ) && (obj.type > DRAW_BUILDING_END) ) {
        return;
    }

    is_processed = true;

    for ( size_t i = 0; i < obj.refs.size(); i++ ) {

        if ( obj.refs[i].ref != REF_WAY ) {
            continue;
        }

        _get_role ( obj.refs[i], way_role );

        if ( way_role == ROLE_OUTER ) {
            rel_outer_way.push_back ( obj.refs[i].id );
            continue;
        }

        if ( way_role == ROLE_INNER ) {
            rel_inner_way.push_back ( obj.refs[i].id );
            continue;
        }               

        if ( way_role == ROLE_UNKNOWN ) {
            assert ( false );
        }
    }

    for ( auto it : rel_outer_way ) {
        auto way_ptr = g_ways_list.find (it);
        if ( way_ptr != g_ways_list.end() ) {
            way_ptr->second.in_use = true;
        }
    }

    for (auto it : rel_inner_way) {
        auto way_ptr = g_ways_list.find(it);
        if (way_ptr != g_ways_list.end()) {
            way_ptr->second.in_use = true;
        }
    }

    assert ( rel_outer_way.size() > 0 );

    _merge_areas ( rel_outer_way, DRAW_BUILDING_OUTER, out_outer_list );
    _merge_areas ( rel_inner_way, DRAW_BUILDING_INNER, out_inner_list );
}

static void process_rels() {

    auto pos = g_rels_list.begin();
    bool is_processed = false;

    while ( pos != g_rels_list.end() ) {

        if ( pos->second.bad ) {
            pos++;
            continue;
        }

        if ( pos->second.type < DRAW_START ) {
            pos++;
            continue;
        }

        is_processed = false;
        _process_rel_building ( is_processed, pos->second );
        if ( !is_processed ) {
            is_processed = false;
        }
        // DRAW_AREA_UNKNOWN,
        // DRAW_AREA_WATER,
        // DRAW_AREA_ASPHALT,
        // DRAW_AREA_GRASS,
        // DRAW_AREA_FORSET,
        // DRAW_AREA_SAND,
        // DRAW_AREA_MOUNTAIN,
        // DRAW_AREA_STONE,


        pos++;

        // DRAW_REL_STREET,
        // DRAW_REL_WATERWAY,
        // DRAW_REL_BRIDGE,
        // DRAW_REL_TUNNEL,

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
    // process_ways();
    // process_nodes();

    return 0;
}
