#ifndef __OSM_PROCESSOR_H__
#define __OSM_PROCESSOR_H__

#include <osm_types_idx.h>
#include <ssearch.h>
#include <libhpxml.h>

typedef void (*osm_object_t) ( osm_obj_info_t& info );

typedef void (*node_info_callback_t) ( const storenode_t& node );
typedef void (*way_info_callback_t)  ( const storeway_t&   way );
typedef void (*rel_info_callback_t)  ( const storerels_t&  rel );

class osm_processor_t {

    public:
        osm_processor_t();

    public:
        bool process_file     ( const char* const file_name );
                              
        bool enum_nodes       ( node_info_callback_t callback );
        bool enum_ways        ( way_info_callback_t  callback );
        bool enum_rels        ( rel_info_callback_t  callback );
                              
        bool load_node        ( osm_obj_type_t _id, storenode_t& node );
        bool load_way         ( osm_obj_type_t _id, storeway_t&   way );
        bool load_rel         ( osm_obj_type_t _id, storerels_t&  rel );
                              
        bool populate_node    ( osm_obj_type_t _id, bool err_allowed, obj_node_t& node );
        bool populate_way     ( osm_obj_type_t _id, bool err_allowed, obj_way_t& way );
        bool populate_rel     ( osm_obj_type_t _id, bool err_allowed, obj_rel_t& rel );
                              
        void mark_relation    ( osm_obj_type_t _id );
        void mark_way         ( osm_obj_type_t _id );
        void mark_nodes       ( osm_obj_type_t _id );

        void reconstruct_way  ( list_rel_refs_t& in_list, list_obj_way_t& out_list );

    private:
        void bor_init ( const osm_mapper_t* const lex_list, size_t cnt, ssearcher& bor );
        void store_attr ( int attr_cnt, const hpx_attr_t* new_item );
        void load_skiplist ( const char* const file_name, ssearcher& bor );
        void cp_val ( const hpx_attr_t& src, alloc_str_t& dst );
        void store_ref ( const bstring_t& value, ref_type_t ref_type );
        void osm_push ( osm_node_t next_node );
        void osm_pop ( osm_node_t osm_node );
        void process_open ( int attr_cnt, const hpx_attr_t* attr_list );
        void test_and_add ( const char* const key );
        void key_cmp ( const char* const key, const char* const val, bool& r1, bool& r2 );
        bool find_key ( const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1 = nullptr, osm_str_t v2 = nullptr, osm_str_t v3 = nullptr );
        void map_type ( draw_type_t& draw_type, const osm_tag_ctx_t& node_info, const ssearcher& bor );
        bool is_area ( const osm_tag_ctx_t& xml_tags );
        void map_ref ( const hpx_attr_t* attr, ref_type_t& ref );
        void map_role ( const hpx_attr_t* attr, ref_role_t& role );
        void clean_info ( void );

        void _merge_type1 ( const obj_way_t& src, obj_way_t& dst );
        void _merge_type2 ( const obj_way_t& src, obj_way_t& dst );
        void _merge_type3 ( const obj_way_t& src, obj_way_t& dst );
        void _merge_type4 ( const obj_way_t& src, obj_way_t& dst );

    private:
        void process_root_node ( int attr_cnt, const hpx_attr_t* attr_list );
        void process_node_tag ( int attr_cnt, const hpx_attr_t* attr_list );

    private:
        void process_root_way ( int attr_cnt, const hpx_attr_t* attr_list );
        void process_way_nd ( int attr_cnt, const hpx_attr_t* attr_list );
        void process_way_tag ( int attr_cnt, const hpx_attr_t* attr_list );

    private:
        void process_root_rel ( int attr_cnt, const hpx_attr_t* attr_list );
        void process_rel_member ( int attr_cnt, const hpx_attr_t* attr_list );
        void process_rel_tag ( int attr_cnt, const hpx_attr_t* attr_list );

        void process_osm_param ( draw_type_t& draw_type, draw_type_t new_type, const char* const name );
        void process_building ( draw_type_t& draw_type, const osm_tag_ctx_t& node_info );
        void process_unused ( draw_type_t& draw_type, const osm_tag_ctx_t& xml_tags, const ssearcher& bor );
        void log_node ( const char* const name, osm_id_t id, const osm_tag_ctx_t& node_info );

        void nodes_init ( void );
        void node_expand_tags ( void );
        void node_resolve_type ( void );
        void node_store_info ( void );

        void ways_init ( void );
        void ways_expand_tags ( void );
        void ways_resolve_type ( void );
        void ways_store_info ( void );

        void rels_init ( void );
        void rel_expand_tags ( void );
        void rel_resolve_type ( void );
        void rel_store_info ( void );

        void resolve_level ( int& level );

        void process_item ( int xml_type, const bstring_t& name, int attr_cnt, const hpx_attr_t* attr_list );

        void reorder ( const obj_way_t& in, obj_way_t& out );

        bool load_osm ( const char* const file_name );

    private:
        void fix_area ( osm_obj_info_t& info );
        void add_node ( osm_obj_info_t& info );
        void add_way  ( osm_obj_info_t& info );
        void add_rel  ( osm_obj_info_t& info );

    private:
        ssearcher        area_landuse;
        ssearcher        area_leisure;
        ssearcher        area_natural;
        ssearcher        area_amenity;
        ssearcher        area_waterway;
        ssearcher        area_manmade;
        ssearcher        area_highway;
        ssearcher        area_transport;
        ssearcher        area_water;
        ssearcher        area_railway;
        ssearcher        area_boundary;
        ssearcher        area_place;

    private:
        ssearcher        path_highway;
        ssearcher        path_railway;
        ssearcher        path_waterway;
        ssearcher        path_natural;
        ssearcher        path_transport;
        ssearcher        path_power;
        ssearcher        path_bridge;
        ssearcher        path_stairwell;
        ssearcher        path_barrier;
        ssearcher        path_golf;
        ssearcher        path_shop;
        ssearcher        ways_ignored;

    private:
        ssearcher        rels_routes;
        ssearcher        rels_boundary;
        ssearcher        rels_landuse;
        ssearcher        rels_leisure;
        ssearcher        rels_restriction;
        ssearcher        rels_type;

    private:
        ssearcher        skiplist_nodes;
        ssearcher        skiplist_ways;
        ssearcher        skiplist_rels;

    private:
        osm_node_t       xml_ctx[4];
        int              xml_ctx_cnt;

    private:
        osm_obj_info_t   osm_info;

    private:
        map_storenode_t  nodes_list;
        map_storeway_t   ways_list;
        map_storerel_t   rels_list;
};

#endif


