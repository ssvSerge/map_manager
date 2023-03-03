#ifndef __OSMLEX_H__
#define __OSMLEX_H__

#include <string>
#include <vector>

#include "..\common\osm_idx.h"
#include "..\common\osm_types.h"

namespace cfg {

    typedef const char* const osm_str_t;

    typedef struct tag_osm_mapper {
        osm_str_t               k;
        osm_draw_type_t         v;
    }   osm_mapper_t;

    class OsmCfgParser {

        public:
            OsmCfgParser();

        public:
            bool LoadConfig ( const char* const file_name );
            bool ParseWay   ( const osm_tag_ctx_t& node_info, osm_id_t id, ref_list_t& ref_list, osm_draw_type_t& draw_type );
    
        private:
            bool  find_key ( const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1, osm_str_t v2 = nullptr, osm_str_t v3 = nullptr );
            void  map_type ( osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info, osm_str_t k, const osm_mapper_t* const v_map, size_t cnt, osm_draw_type_t& type );

        private:
            void _process_area     ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type );
            void _process_highway  ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type );
            void _process_waterway ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type );
            void _process_building ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type );
            void _process_landuse  ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type );
            void _process_amenity  ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type );
            void _process_unused   ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type );

        private:
            std::string     m_cfg_text;
            uint32_t        m_cfg_offset;
    };

}

#endif
