#ifndef __OSM_PROCESSOR_H__
#define __OSM_PROCESSOR_H__

#include "..\types\osm_types.h"
#include "..\common\ssearch.h"
#include "..\common\libhpxml.h"

typedef void (*osm_object_t) ( const osm_obj_info_t& info );

class osm_processor_t {

    public:
        osm_processor_t();

    public:
        void configure ( osm_object_t add_node, osm_object_t add_way, osm_object_t add_rel );
        bool process_file(const char* const file_name);

    private:
        void bor_init(const osm_mapper_t* const lex_list, size_t cnt, ssearcher& bor);
        void store_attr(int attr_cnt, const hpx_attr_t* new_item);
        void load_skiplist(const char* const file_name, ssearcher& bor);
        void cp_val(const hpx_attr_t& src, alloc_str_t& dst);
        void store_ref(const bstring_t& value, ref_type_t ref_type);
        void osm_push(osm_node_t next_node);
        void osm_pop(osm_node_t osm_node);
        void process_open(int attr_cnt, const hpx_attr_t* attr_list);
        void test_and_add(const char* const key);
        void key_cmp(const char* const key, const char* const val, bool& r1, bool& r2);
        bool find_key(const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1 = nullptr, osm_str_t v2 = nullptr, osm_str_t v3 = nullptr);
        void map_type(osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info, const ssearcher& bor);
        bool is_area(const osm_tag_ctx_t& xml_tags);

    private:
        void process_root_node(int attr_cnt, const hpx_attr_t* attr_list);
        void process_node_tag(int attr_cnt, const hpx_attr_t* attr_list);

    private:
        void process_root_way(int attr_cnt, const hpx_attr_t* attr_list);
        void process_way_nd(int attr_cnt, const hpx_attr_t* attr_list);
        void process_way_tag(int attr_cnt, const hpx_attr_t* attr_list);

    private:
        void process_root_rel(int attr_cnt, const hpx_attr_t* attr_list);
        void process_rel_member(int attr_cnt, const hpx_attr_t* attr_list);
        void process_rel_tag(int attr_cnt, const hpx_attr_t* attr_list);

        void process_osm_param(osm_draw_type_t& draw_type, osm_draw_type_t new_type, const char* const name);
        void process_building(osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info);
        void process_unused(osm_draw_type_t& draw_type, const osm_tag_ctx_t& xml_tags, const ssearcher& bor);
        void log_node(osm_id_t id, const osm_tag_ctx_t& node_info);

        void node_expand_tags(void);
        void node_resolve_type(void);
        void node_store_info(void);
        void node_clean_info(void);

        void ways_init(void);
        void ways_expand_tags(void);
        void ways_resolve_type(void);
        void ways_store_info(void);
        void ways_clean_info(void);

        void rel_expand_tags(void);
        void rel_resolve_type(void);
        void rel_store_info(void);
        void rel_clean_info(void);

        void process_item(int xml_type, const bstring_t& name, int attr_cnt, const hpx_attr_t* attr_list);

    private:
        osm_object_t     m_callback_node;
        osm_object_t     m_callback_way;
        osm_object_t     m_callback_rel;

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
        ssearcher        skiplist_nodes;
        ssearcher        skiplist_ways;
        ssearcher        skiplist_rels;

    private:
        osm_node_t       xml_ctx[4];
        int              xml_ctx_cnt;

    private:
        osm_obj_info_t   osm_info;

};

#endif


