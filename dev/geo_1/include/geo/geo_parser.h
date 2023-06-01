#ifndef __GEO_PARSER_H__
#define __GEO_PARSER_H__

#include "geo_param.h"
#include "geo_obj.h"

#define CNT(x)   ( sizeof(x) / sizeof(x[0]) )

static const lex_ctx_t g_lex_map[] = {

    { "RECORD",     "AREA",        OBJID_RECORD_AREA },
    { "RECORD",     "BUILDING",    OBJID_RECORD_BUILDING },
    { "RECORD",     "HIGHWAY",     OBJID_RECORD_HIGHWAY },
    { "RECORD",     "END",         OBJID_RECORD_END },

    { "XTYPE",      "",            OBJID_COORD },
    { "XCNT",       "",            OBJID_COORD },

    { "ROLE",       "OUTER",       OBJID_ROLE_OUTER },
    { "ROLE",       "INNER",       OBJID_ROLE_INNER },
    { "ROLE",       "COORDS",      OBJID_ROLE_COORDS },
    { "ROLE",       "END",         OBJID_ROLE_END },

    { "TYPE",       "ASPHALT",     OBJID_TYPE_ASPHALT },
    { "TYPE",       "WATER",       OBJID_TYPE_WATER },
    { "TYPE",       "FOREST",      OBJID_TYPE_FOREST },
    { "TYPE",       "GRASS",       OBJID_TYPE_GRASS },
    { "TYPE",       "GENERAL",     OBJID_TYPE_GENERAL },
    { "TYPE",       "MOUNTAIN",    OBJID_TYPE_MOUNTAIN },
    { "TYPE",       "STONE",       OBJID_TYPE_STONE },
    { "TYPE",       "SAND",        OBJID_TYPE_SAND },
    { "TYPE",       "UNDEFINED",   OBJID_TYPE_UNDEFINED },
    { "TYPE",       "BUILDING",    OBJID_TYPE_BUILDING },
    { "TYPE",       "FOOTWAY",     OBJID_TYPE_FOOTWAY },
    { "TYPE",       "ROAD",        OBJID_TYPE_ROAD },
    { "TYPE",       "SECONDARY",   OBJID_TYPE_SECONDARY },
    { "TYPE",       "TRUNK",       OBJID_TYPE_TRUNK },
    { "TYPE",       "MOTORWAY",    OBJID_TYPE_MOTORWAY },
    { "TYPE",       "PRIMARY",     OBJID_TYPE_PRIMARY },
    { "TYPE",       "TERTIARY",    OBJID_TYPE_TERTIARY },
    { "TYPE",       "RAILWAY",     OBJID_TYPE_RAILWAY },
    { "TYPE",       "RIVER",       OBJID_TYPE_RIVER },
    { "TYPE",       "BRIDGE",      OBJID_TYPE_BRIDGE },
    { "TYPE",       "TUNNEL",      OBJID_TYPE_TUNNEL },
    { "SIZE",       "",            OBJID_AREA_SIZE },
    { "C",          "",            OBJID_COORD }

};

static const lex_ctx_t g_lex_idx[] = {

    { "IDX",        "RECT",        OBJID_IDX_BEGIN },
    { "POSITION",   "",            OBJID_IDX_RECT },
    { "OFF",        "BEGIN",       OBJID_IDX_MEMBERS_BEGIN },
    { "M",          "",            OBJID_IDX_MEMBER },
    { "OFF",        "END",         OBJID_IDX_MEMBERS_END },
    { "IDX",        "END",         OBJID_IDX_END },
};


class geo_parser_t {

    public:
        void load_param ( char ch, bool& eo_cmd, geo_offset_t file_offset ) {
            m_geo_param._load_lex ( ch, eo_cmd, file_offset );
        }

        void process_map ( geo_obj_map_t& geo_obj, bool& eor ) {

            obj_id_t        code = OBJID_UNDEF;
            geo_coords_t    geo_coords;
            uint32_t        geo_area;

            eor = false;

            for (size_t i = 0; i < CNT(g_lex_map); i++) {
                if (_cmp(&g_lex_map[i], code)) {
                    break;
                }
            }

            switch ( code ) {

                case OBJID_RECORD_AREA:
                case OBJID_RECORD_BUILDING:
                case OBJID_RECORD_HIGHWAY:
                    geo_obj.clear();
                    geo_obj.m_off = m_geo_param.param.off;
                    geo_obj.m_record = code;
                    m_dir_to_object = true;
                    break;

                case OBJID_RECORD_END:
                    eor = true;
                    break;

                case OBJID_ROLE_OUTER:
                case OBJID_ROLE_INNER:
                case OBJID_ROLE_COORDS:
                    m_geo_line.role = code;
                    m_dir_to_object = false;
                    break;

                case OBJID_COORD:
                    _load(geo_coords);
                    m_geo_line.coords.push_back ( geo_coords );
                    break;

                case OBJID_ROLE_END:
                    geo_obj.m_lines.push_back(m_geo_line);
                    m_geo_line.clear();
                    break;

                case OBJID_TYPE_ASPHALT:
                case OBJID_TYPE_WATER:
                case OBJID_TYPE_FOREST:
                case OBJID_TYPE_GRASS:
                case OBJID_TYPE_GENERAL:
                case OBJID_TYPE_MOUNTAIN:
                case OBJID_TYPE_STONE:
                case OBJID_TYPE_SAND:
                case OBJID_TYPE_UNDEFINED:
                case OBJID_TYPE_BUILDING:
                case OBJID_TYPE_FOOTWAY:
                case OBJID_TYPE_ROAD:
                case OBJID_TYPE_SECONDARY:
                case OBJID_TYPE_TRUNK:
                case OBJID_TYPE_MOTORWAY:
                case OBJID_TYPE_PRIMARY:
                case OBJID_TYPE_TERTIARY:
                case OBJID_TYPE_RAILWAY:
                case OBJID_TYPE_RIVER:
                case OBJID_TYPE_BRIDGE:
                case OBJID_TYPE_TUNNEL:
                    if ( m_dir_to_object ) {
                        geo_obj.m_type = code;
                    } else {
                        m_geo_line.m_type = code;
                    }
                    break;

                case OBJID_AREA_SIZE:
                    _load ( geo_area );
                    m_geo_line.area = geo_area;
                    break;


            }

            m_geo_param.reset();
        }

        void process_idx ( geo_idx_rec_t& geo_idx, bool& eor ) {

            obj_id_t        code = OBJID_UNDEF;

            eor = false;

            for (size_t i = 0; i < CNT(g_lex_idx); i++) {
                if ( _cmp(&g_lex_idx[i], code) ) {
                    break;
                }
            }

            switch (code) {

                case OBJID_IDX_BEGIN:
                    geo_idx.clear();
                    break;

                case OBJID_IDX_RECT:
                    geo_idx.m_rect.load(m_geo_param.value.msg);
                    break;

                case OBJID_IDX_MEMBERS_BEGIN:
                    geo_idx.m_list_off.clear();
                    break;

                case OBJID_IDX_MEMBER:
                    geo_offset_t off;
                    sscanf_s(m_geo_param.value.msg, "%d", &off);
                    geo_idx.m_list_off.push_back(off);
                    break;

                case OBJID_IDX_MEMBERS_END:
                    break;

                case OBJID_IDX_END:
                    eor = true;
                    break;
            }

            m_geo_param.reset();
        }

    private:
        bool _cmp ( const lex_ctx_t* inp, obj_id_t& val ) {

            if (strcmp(inp->p, m_geo_param.param.msg) != 0) {
                return false;
            }

            if (inp->v[0] != 0) {
                if (strcmp(inp->v, m_geo_param.value.msg) != 0) {
                    return false;
                }
            }

            val = inp->n;
            return true;
        }

        void _load ( geo_coords_t& coords ) {
            sscanf_s ( m_geo_param.value.msg, "%lf %lf", &coords.y, &coords.x );
        }

        void _load ( uint32_t& geo_coords ) {
            sscanf_s ( m_geo_param.value.msg, "%d", &geo_coords );
        }

    public:
        geo_param_t         m_geo_param;
        geo_line_t          m_geo_line;
        bool                m_dir_to_object;
};

#endif

