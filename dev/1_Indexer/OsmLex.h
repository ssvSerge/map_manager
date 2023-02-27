#ifndef __OSMLEX_H__
#define __OSMLEX_H__

#include <string>
#include <vector>

#include "..\common\osm_idx.h"
#include "..\common\osm_types.h"

typedef std::vector<std::string>     osm_param_t;
typedef std::vector<std::string>     osm_tokens_t;

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
            bool ParseWay   ( const osm_tag_ctx_t& node_info );
    
        private:
            bool  find_key ( const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1, osm_str_t v2 = nullptr, osm_str_t v3 = nullptr );
            bool  map_type ( const osm_tag_ctx_t& node_info, osm_str_t k, const osm_mapper_t* const v_map, size_t cnt, osm_draw_type_t& type );

        private:
            bool _process_area ( const osm_tag_ctx_t& node_info, osm_draw_type_t& draw_type);
            bool _process_highway ( const osm_tag_ctx_t& node_info, osm_draw_type_t& draw_type );

        private:
            std::string     m_cfg_text;
            uint32_t        m_cfg_offset;
    };

}

#endif
