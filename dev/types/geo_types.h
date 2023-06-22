#ifndef __GEO_TYPES_H__
#define __GEO_TYPES_H__

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include <set>
#include <vector>
#include <list>
#include <string>

#include <clipper2/clipper.h>

#define CNT(x)          ( sizeof(x) / sizeof(x[0]) )
#define MSG_LEN         ( 128 )

typedef Clipper2Lib::PointD                         geo_coord_t;
typedef Clipper2Lib::PathD                          v_geo_coord_t;
typedef Clipper2Lib::PathsD                         vv_geo_coord_t;
typedef std::vector<vv_geo_coord_t>                 vvv_geo_coord_t;
typedef int32_t                                     geo_color_t;

typedef uint32_t                                    geo_offset_t;
typedef std::set<geo_offset_t>                      set_offset_t;
typedef std::vector<geo_offset_t>                   v_geo_offset_t;
typedef std::vector<v_geo_offset_t>                 vv_geo_offset_t;
typedef std::list<geo_offset_t>                     list_geo_offset_t;
typedef std::vector<uint32_t>                       v_uint32_t;
typedef std::vector<uint64_t>                       vector_uint64_t;

#define GEO_RGB(dst,la,lr,lg,lb)                    { dst.a=la; dst.r=lr; dst.g=lg; dst.b=lb; }

typedef enum tag_obj_type {

    OBJID_UNDEF,

    OBJID_TYPE_ASPHALT,
    OBJID_TYPE_WATER,
    OBJID_TYPE_FOREST,
    OBJID_TYPE_GRASS,
    OBJID_TYPE_GENERAL,
    OBJID_TYPE_MOUNTAIN,
    OBJID_TYPE_STONE,
    OBJID_TYPE_SAND,
    OBJID_TYPE_UNDEFINED,
    OBJID_TYPE_BUILDING,
    OBJID_TYPE_FOOTWAY,
    OBJID_TYPE_ROAD,
    OBJID_TYPE_SECONDARY,
    OBJID_TYPE_TRUNK,
    OBJID_TYPE_MOTORWAY,
    OBJID_TYPE_PRIMARY,
    OBJID_TYPE_TERTIARY,
    OBJID_TYPE_RAILWAY,
    OBJID_TYPE_RIVER,
    OBJID_TYPE_BRIDGE,
    OBJID_TYPE_TUNNEL,

    OBJID_RECORD_AREA,
    OBJID_RECORD_BUILDING,
    OBJID_RECORD_HIGHWAY,

        OBJID_XTYPE,
        OBJID_XCNT,
        OBJID_XREF,

        OBJID_ROLE_OUTER,
        OBJID_ROLE_INNER,

            OBJID_TYPE,

            OBJID_SIZE,

            OBJID_COORDS,

        OBJID_ROLE_END,

    OBJID_RECORD_END,

    OBJID_IDX_BEGIN,
    OBJID_IDX_RECT,
    OBJID_IDX_MEMBERS_BEGIN,
    OBJID_IDX_MEMBER,
    OBJID_IDX_MEMBERS_END,
    OBJID_IDX_END,

    OBJID_LAST_ID

}   obj_type_t;

typedef std::vector<obj_type_t>   v_geo_obj_t;

class paint_wnd_t {
    public:
        int32_t   x;
        int32_t   y;
};

typedef std::vector<paint_wnd_t>    v_paint_wnd_t;
typedef std::vector<v_paint_wnd_t>  vv_paint_wnd_t;

class geo_record_t {

    public:
        uint32_t                entry_offset;
        obj_type_t              m_geo_type;
        obj_type_t              m_prime_type;
        geo_offset_t            m_prime_off;
        size_t                  m_record_id;
        uint64_t                m_osm_ref;

        v_geo_obj_t             m_child_roles;
        v_geo_obj_t             m_child_types;
        v_uint32_t              m_child_areas;
        vv_geo_coord_t          m_child_lines;
        vv_paint_wnd_t          m_child_wnd_lines;
        v_geo_coord_t           m_paint_pos;

    public:
        void clear() {
            entry_offset   = 0;
            m_prime_off    = 0;
            m_record_id    = 0;
            m_prime_type   = OBJID_UNDEF;
            m_child_roles.clear();
            m_child_types.clear();
            m_child_areas.clear();
            m_child_lines.clear();
        }

        bool operator==(const uint32_t rhs) const {
            return this->entry_offset == rhs;
        }

};

typedef std::list<geo_record_t>     l_geo_record_t;
typedef std::vector<geo_record_t>   v_geo_record_t;

typedef struct tag_lex_ctx {
    const char* p;
    const char* v;
    obj_type_t  n;
}   lex_ctx_t;

class geo_lex_t {

    public:
        geo_lex_t() {
            msg[0] = 0;
            pos    = 0;
            off    = 0;
        }

    public:

        void reset ( void ) {
            msg[0] = 0;
            pos    = 0;
        }

        void store ( char ch, geo_offset_t offset ) {

            if ( pos == 0 ) {
                off = offset;
            }

            msg[pos] = ch;
            pos++;
            msg[pos] = 0;
        }

    public:
        char            msg[MSG_LEN];
        int             pos;
        geo_offset_t    off;
};

class geo_param_t {

    public:
        geo_param_t() {
            direction = false;
        }

    public:
        void reset ( void ) {
            direction = false;
            param.reset();
            value.reset();
        }

        void store ( char ch, geo_offset_t offset ) {
            if ( !direction ) {
                param.store(ch, offset);
            } else {
                value.store(ch, offset);
            }
        }

        void _load_lex ( char ch, bool& eo_cmd, geo_offset_t offset ) {

            eo_cmd = false;

            if (ch == 0x0D) {
                return;
            }
            if (ch == 0x0A) {
                return;
            }

            if (ch == ':') {
                direction = true;
                return;
            }
            if (ch == ';') {
                eo_cmd = true;
                return;
            }

            store(ch, offset);
        }

    public:
        geo_lex_t    param;
        geo_lex_t    value;

    private:
        bool         direction;
};

class geo_rect_t {

    public:
        void clear() {
            min_lon = -333;
            min_lat = -333;
            max_lon = -333;
            max_lat = -333;
        }

        void load(const char* const val) {
            (void)sscanf_s( val, "%lf %lf %lf %lf", &min_lon, &min_lat, &max_lon, &max_lat);
        }

    public:
        double   min_lon = -333;
        double   min_lat = -333;
        double   max_lon = -333;
        double   max_lat = -333;
};

class window_rect_t {

    public:
        window_rect_t() {
            min_x = 0;
            min_y = 0;
            max_x = 0;
            max_y = 0;
        }

        int32_t width() const {
            return (max_x - min_x);
        }

        int32_t height() const {
            return (max_y - min_y);
        }

    public:
        int32_t  min_x;
        int32_t  min_y;
        int32_t  max_x;
        int32_t  max_y;
};

class geo_idx_rec_t {

    public:
        geo_rect_t         m_rect;
        v_geo_offset_t     m_list_off;
        uint64_t           m_osm_id;

    public:
        void clear() {
            m_rect.clear();
            m_list_off.clear();
            m_osm_id = 0;
        }
};

typedef std::list<geo_idx_rec_t>    list_geo_idx_rec_t;
typedef std::vector<geo_idx_rec_t>  vector_geo_idx_rec_t;

static const lex_ctx_t g_lex_map[] = {
    { "RECORD",     "AREA",         OBJID_RECORD_AREA },
    { "RECORD",     "BUILDING",     OBJID_RECORD_BUILDING },
    { "RECORD",     "HIGHWAY",      OBJID_RECORD_HIGHWAY },
    { "XTYPE",      "",             OBJID_XTYPE },
    { "XCNT",       "",             OBJID_XCNT },
    { "ROLE",       "OUTER",        OBJID_ROLE_OUTER },
    { "ROLE",       "INNER",        OBJID_ROLE_INNER },
    { "TYPE",       "",             OBJID_TYPE },
    { "SIZE",       "",             OBJID_SIZE },
    { "C",          "",             OBJID_COORDS },
    { "ROLE",       "END",          OBJID_ROLE_END },
    { "RECORD",     "END",          OBJID_RECORD_END },
    { "REF",        "",             OBJID_XREF },
};

static const lex_ctx_t g_lex_types[] = {
    { "",  "ASPHALT",               OBJID_TYPE_ASPHALT   },
    { "",  "WATER",                 OBJID_TYPE_WATER     },
    { "",  "FOREST",                OBJID_TYPE_FOREST    },
    { "",  "GRASS",                 OBJID_TYPE_GRASS     },
    { "",  "GENERAL",               OBJID_TYPE_GENERAL   },
    { "",  "MOUNTAIN",              OBJID_TYPE_MOUNTAIN  },
    { "",  "STONE",                 OBJID_TYPE_STONE     },
    { "",  "SAND",                  OBJID_TYPE_SAND      },
    { "",  "UNDEFINED",             OBJID_TYPE_UNDEFINED },
    { "",  "BUILDING",              OBJID_TYPE_BUILDING  },
    { "",  "FOOTWAY",               OBJID_TYPE_FOOTWAY   },
    { "",  "ROAD",                  OBJID_TYPE_ROAD      },
    { "",  "SECONDARY",             OBJID_TYPE_SECONDARY },
    { "",  "TRUNK",                 OBJID_TYPE_TRUNK     },
    { "",  "MOTORWAY",              OBJID_TYPE_MOTORWAY  },
    { "",  "PRIMARY",               OBJID_TYPE_PRIMARY   },
    { "",  "TERTIARY",              OBJID_TYPE_TERTIARY  },
    { "",  "RAILWAY",               OBJID_TYPE_RAILWAY   },
    { "",  "RIVER",                 OBJID_TYPE_RIVER     },
    { "",  "BRIDGE",                OBJID_TYPE_BRIDGE    },
    { "",  "TUNNEL",                OBJID_TYPE_TUNNEL    },
};

static const lex_ctx_t g_lex_idx[] = {
    { "IDX",        "RECT",         OBJID_IDX_BEGIN },
    { "POSITION",   "",             OBJID_IDX_RECT },
    { "OFF",        "BEGIN",        OBJID_IDX_MEMBERS_BEGIN },
    { "M",          "",             OBJID_IDX_MEMBER },
    { "REF",        "",             OBJID_XREF },
    { "OFF",        "END",          OBJID_IDX_MEMBERS_END },
    { "IDX",        "END",          OBJID_IDX_END },
};

class geo_parser_t {

    private:
        geo_param_t m_geo_param;
 
    public:
        void load_param ( char ch, bool& eo_cmd, geo_offset_t file_offset ) {
            m_geo_param._load_lex ( ch, eo_cmd, file_offset );
        }

        void process_map ( geo_record_t& geo_obj, bool& eor ) {

            obj_type_t         code = OBJID_UNDEF;
            geo_coord_t        geo_coords;
            v_geo_coord_t      empty_path;
            obj_type_t         type;
            uint64_t           osm_id;
            size_t             record_id = geo_obj.m_record_id;

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
                    geo_obj.m_geo_type = code;
                    geo_obj.m_prime_off  = m_geo_param.param.off;
                    break;

                case OBJID_XTYPE:
                    _map_type(m_geo_param.value.msg, type);
                    geo_obj.m_prime_type = type;
                    break;

               case OBJID_XREF:
                    sscanf_s ( m_geo_param.value.msg, "%lld", &osm_id );
                    geo_obj.m_osm_ref = osm_id;
                    break;

                case OBJID_XCNT:
                    uint32_t cnt;
                    sscanf_s ( m_geo_param.value.msg, "%d", &cnt );
                    geo_obj.m_child_roles.resize ( cnt );
                    geo_obj.m_child_types.resize ( cnt );
                    geo_obj.m_child_areas.resize ( cnt );
                    geo_obj.m_child_lines.resize ( cnt );
                    break;

                case OBJID_ROLE_OUTER:
                case OBJID_ROLE_INNER:
                    geo_obj.m_child_roles[record_id] = code;
                    break;

                case OBJID_TYPE:
                    _map_type(m_geo_param.value.msg, type);
                    if ( type == OBJID_UNDEF ) {
                        type = geo_obj.m_prime_type;
                    }
                    geo_obj.m_child_types[record_id] = type;
                    break;

                case OBJID_SIZE:
                    uint32_t size;
                    sscanf_s(m_geo_param.value.msg, "%d", &size );
                    geo_obj.m_child_areas[record_id] = size;
                    break;

                case OBJID_COORDS:
                    _load(geo_coords);
                    geo_obj.m_child_lines[record_id].push_back(geo_coords);
                    break;

                case OBJID_ROLE_END:
                    geo_obj.m_record_id++;
                    break;

                case OBJID_RECORD_END:  
                    eor = true;
                    break;
            }

            m_geo_param.reset();
        }

        void process_idx ( geo_idx_rec_t& geo_idx, bool& eor ) {

            uint64_t    ref;
            obj_type_t  code = OBJID_UNDEF;

            eor = false;

            for (size_t i = 0; i < CNT(g_lex_idx); i++) {
                if (_cmp(&g_lex_idx[i], code)) {
                    break;
                }
            }

            switch ( code ) {

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

                case OBJID_XREF:
                    sscanf_s(m_geo_param.value.msg, "%lld", &ref);
                    geo_idx.m_osm_id = ref;
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
        bool _cmp ( const lex_ctx_t* inp, obj_type_t& val ) {

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

        void _load ( geo_coord_t& coords ) {
            sscanf_s ( m_geo_param.value.msg, "%lf %lf", &coords.y, &coords.x );
        }

        void _load ( uint32_t& geo_coords ) {
            sscanf_s ( m_geo_param.value.msg, "%d", &geo_coords );
        }

        void _map_type ( const char* const key, obj_type_t& type ) {
            for (size_t i = 0; i < CNT(g_lex_types); i++) {
                if (strcmp(g_lex_types[i].v, key) == 0) {
                    type = g_lex_types[i].n;
                    break;
                }
            }
        }

};

#endif
