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
#include <cassert>

#include <geo_projection.h>
#include <lex_keys.h>

#define CNT(x)          ( sizeof(x) / sizeof(x[0]) )
#define MSG_LEN         ( 128 )

typedef enum tag_pos_type_t {
    POS_TYPE_UNKNOWN,
    POS_TYPE_GPS,
    POS_TYPE_MAP,
    POS_TYPE_ANGLE,    
}   pos_type_t;

class geo_pos_t {
    public:
        geo_pos_t() {
            clear();
        }

        void clear() {
            x = y = 0;
        }

    public:
        double x;
        double y;
};

class map_pos1_t {
    public:
        map_pos1_t() {
            clear();
        }

        void clear() {
            x = y = 0;
        }

        void operator= ( const map_pos1_t& _src ) {
            this->x = _src.x;
            this->y = _src.y;
        }

    public:
        int32_t x;
        int32_t y;
};


class geo_point_t {

    private:
        map_pos1_t  map;
        map_pos1_t  ang;
        geo_pos_t   gps;
        pos_type_t  src;

    public:
        geo_point_t() {
            clear();
        }

        void clear() {
            gps.clear();
            map.clear();
            ang.clear();
            src = POS_TYPE_UNKNOWN;
        }

        void reset_angle() {
            ang = map;
        }

        void set_src ( pos_type_t _src ) {
            src = _src;
        }

        double get_geo_x ( void ) const {
            return gps.x;
        }

        double get_geo_y ( void ) const {
            return gps.y;
        }

        int32_t get_map_x ( void ) const {
            return map.x;
        }

        int32_t get_map_y ( void ) const {
            return map.y;
        }

        //--------------------------------------------------//

        void get_geo ( geo_pos_t& _dst ) const {
            _dst.x = gps.x;
            _dst.y = gps.y;
        }

        void set_geo ( const geo_pos_t& _src ) {
            this->gps = _src;
        }

        void set_geo_x ( double val ) {
            gps.x = val;
        }

        void set_geo_y ( double val ) {
            gps.y = val;
        }

        //--------------------------------------------------//

        void get_map ( map_pos1_t& _dst ) const {
            _dst.x = map.x;
            _dst.y = map.y;
        }

        void set_map ( const map_pos1_t& _src ) {
            this->map = _src;
        }

        void set_map_x ( int32_t val ) {
            map.x = val;
        }

        void set_map_y ( int32_t val ) {
            map.y = val;
        }

        //--------------------------------------------------//

        void set_angle ( const map_pos1_t& _ref ) {
            this->ang = _ref;
        }

        void get_angle ( map_pos1_t& _dst ) const {
            _dst = this->ang;
        }

        //--------------------------------------------------//

        void map_to_geo ( void ) {
            proj_2_geo ( map.x, map.y, gps.x, gps.y );
        }

        bool operator== ( const geo_point_t& _ref ) const {

            bool ret_val = true;

            if ( src == POS_TYPE_GPS ) {
                if ( this->gps.x != _ref.gps.x ) {
                    ret_val = false;
                } else
                if ( this->gps.y != _ref.gps.y ) {
                    ret_val = false;
                }
            } else
            if ( src == POS_TYPE_MAP ) {
                if ( this->map.x != _ref.map.x ) {
                    ret_val = false;
                } else
                if ( this->map.y != _ref.map.y ) {
                    ret_val = false;
                }
            } else {
                assert ( false );
            }

            return ret_val;
        }

        bool operator!= ( const geo_point_t& _ref ) const {
            return !this->operator== ( _ref );
        };

        void operator= ( const geo_point_t& _ref ) {

            this->src  = _ref.src;
            this->gps  = _ref.gps;
            this->map  = _ref.map;
            this->ang  = _ref.ang;
        }

};

typedef geo_point_t                                 geo_coord_t;
typedef std::vector<geo_coord_t>                    v_geo_coord_t;
typedef std::vector<v_geo_coord_t>                  vv_geo_coord_t;
typedef std::vector<vv_geo_coord_t>                 vvv_geo_coord_t;
typedef int32_t                                     geo_color_t;

typedef uint32_t                                    geo_offset_t;
typedef std::set<geo_offset_t>                      set_offset_t;
typedef std::vector<geo_offset_t>                   v_geo_offset_t;
typedef std::vector<v_geo_offset_t>                 vv_geo_offset_t;
typedef std::vector<vv_geo_offset_t>                vvv_geo_offset_t;
typedef std::list<geo_offset_t>                     list_geo_offset_t;
typedef std::vector<uint32_t>                       v_uint32_t;
typedef std::vector<uint64_t>                       vector_uint64_t;

typedef enum tag_obj_type {

    OBJID_ERROR,

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

    OBJID_TYPE_SERVICE,
    OBJID_TYPE_UNCLIASSIFIED,
    OBJID_TYPE_RESIDENTIAL,
    OBJID_TYPE_STREET,
    OBJID_TYPE_PEDISTRAN,
    OBJID_TYPE_TRACK,
    OBJID_TYPE_STEPS,
    OBJID_TYPE_PATH,

    OBJID_TYPE_TERTIARY,
    OBJID_TYPE_RAILWAY,
    OBJID_TYPE_RIVER,
    OBJID_TYPE_BRIDGE,
    OBJID_TYPE_TUNNEL,
    OBJID_TYPE_TREEROW,

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

    OBJID_IDX_MAP_BEGIN,
        OBJID_IDX_MAP_MIN,
        OBJID_IDX_MAP_MAX,
        OBJID_IDX_MAP_XCNT,
        OBJID_IDX_MAP_YCNT,
        OBJID_IDX_MAP_XSTEP,
        OBJID_IDX_MAP_YSTEP,
    OBJID_IDX_MAP_END,

    OBJID_IDX_BEGIN,
        OBJID_IDX_RECT,
        OBJID_IDX_MEMBERS_BEGIN,
        OBJID_IDX_MEMBER,
        OBJID_IDX_MEMBERS_END,
    OBJID_IDX_END,

    OBJID_LAST_ID

}   obj_type_t;

typedef std::vector<obj_type_t>       v_geo_obj_t;

class geo_rect_t {

    public:
        void clear ( void ) {
            min.clear();
            max.clear();
        }

        void set_src ( pos_type_t pos_type ) {
            min.set_src ( pos_type );
            max.set_src ( pos_type );
        }

        void load ( const char* const val ) {

            double   v1, v2, v3, v4;
            int32_t  v5, v6, v7, v8;

            sscanf_s ( val, "%lf %lf %lf %lf %d %d %d %d", &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8 );

            min.set_geo_y ( v1 );
            min.set_geo_x ( v2 );
            max.set_geo_y ( v3 );
            max.set_geo_x ( v4 );

            min.set_map_y ( v5 );
            min.set_map_x ( v6 );
            max.set_map_y ( v7 );
            max.set_map_x ( v8 );

            min.reset_angle ();
            max.reset_angle ();
        }

        bool operator== ( const geo_rect_t& ref ) const {

            if ( ref.min != this->min ) {
                return false;
            }
            if ( ref.max != this->max ) {
                return false;
            }
            return true;
        }

        bool operator!= ( const geo_rect_t& ref ) const {
            bool ret_val = this->operator== (ref);
            return (!ret_val);
        }

        double width ( pos_type_t type ) const {

            double ret_val = 0;

            if ( type == POS_TYPE_GPS ) {
                ret_val = max.get_geo_x() - min.get_geo_x();
            } else 
            if ( type == POS_TYPE_MAP ) {
                ret_val =  max.get_map_x() - min.get_map_x();
            } else {
                assert ( false );
            }

            return ret_val;
        }

        double height ( pos_type_t type ) const {

            double ret_val = 0;

            if ( type == POS_TYPE_GPS ) {
                ret_val = max.get_geo_y() - min.get_geo_y();
            } else 
            if ( type == POS_TYPE_MAP ) {
                ret_val =  max.get_map_y() - min.get_map_y();
            } else {
                assert ( false );
            }

            return ret_val;
        }

        bool is_overlapped ( const pos_type_t src, const geo_rect_t& slice ) const {

            bool ret_val = true;

            if ( src == POS_TYPE_MAP ) {
                if ( this->min.get_map_x() > slice.max.get_map_x() ) { ret_val = false; } else
                if ( this->max.get_map_x() < slice.min.get_map_x() ) { ret_val = false; } else
                if ( this->min.get_map_y() > slice.max.get_map_y() ) { ret_val = false; } else
                if ( this->max.get_map_y() < slice.min.get_map_y() ) { ret_val = false; }
            } else 
            if ( src == POS_TYPE_GPS ) {
                if ( this->min.get_geo_x() > slice.max.get_geo_x() ) { ret_val = false; } else
                if ( this->max.get_geo_x() < slice.min.get_geo_x() ) { ret_val = false; } else
                if ( this->min.get_geo_y() > slice.max.get_geo_y() ) { ret_val = false; } else
                if ( this->max.get_geo_y() < slice.min.get_geo_y() ) { ret_val = false; }
            } else {
                assert ( false );
            }

            return ret_val;
        }

    public:
        geo_coord_t min;
        geo_coord_t max;
};

typedef std::vector<geo_rect_t>       v_geo_rect_t;

#if 0

class paint_rect_t {

    public:
        paint_rect_t() {
            min.x = 0;
            min.y = 0;
            max.x = 0;
            max.y = 0;
        }

        int32_t width() const {
            return (max.x - min.x);
        }

        int32_t height() const {
            return (max.y - min.y);
        }

        bool operator== ( const paint_rect_t& ref ) const {

            if ( ref.min != this->min ) {
                return false;
            }
            if ( ref.max != this->max ) {
                return false;
            }
            return true;
        }

        bool operator!= (const paint_rect_t& ref) const {
            bool ret_val = this->operator== (ref);
            return (!ret_val);
        }

        void clear() {
            min.clear();
            max.clear();
        }

    public:
        geo_coord_t min;
        geo_coord_t max;
};

#endif


class geo_line_t {
    public:
        geo_line_t() {
            clear();
        }

        void clear() {
            m_role = OBJID_ERROR;
            m_type = OBJID_ERROR;
            m_area = 0;
            m_coords.clear();
        }

    public:
        obj_type_t              m_role;      // ROLE:OUTER;ROLE:INNER
        obj_type_t              m_type;      // TYPE:ASPHALT
        uint32_t                m_area;      // SIZE:33429
        v_geo_coord_t           m_coords;    // coords
};

typedef std::vector<geo_line_t>       v_geo_line_t;

class paint_line_t {

    public:
        paint_line_t() {
            clear();
        }

        void clear() {
            m_role = OBJID_ERROR;
            m_type = OBJID_ERROR;
            m_paint.clear();
        }

    public:
        obj_type_t              m_role;      // ROLE:OUTER;ROLE:INNER
        obj_type_t              m_type;      // TYPE:ASPHALT
        vv_geo_coord_t          m_paint;     // paint coords
     // v_paint_coord_t         m_fill;      // fill-in coords
     // v_paint_coord_t         m_paint;     // paint coords
};

typedef std::vector<paint_line_t>       v_paint_line_t;

class geo_entry_t {
    public:
        geo_entry_t() {
            clear();
        }

        void clear() {
            m_default_type = OBJID_ERROR;
            m_record_type  = OBJID_ERROR;
            m_data_off     = 0;
            m_osm_ref      = 0;
            m_to_display   = false;
            m_lines.clear();
        }

    public:
        obj_type_t              m_record_type;      // RECORD:AREA
        obj_type_t              m_default_type;     // XTYPE:ASPHALT
        uint64_t                m_osm_ref;          // REF:8094759
        geo_offset_t            m_data_off;         // Offset in data file.
        bool                    m_to_display;       // TRUE if required to draw.
        int32_t                 m_miss_cnt;         // 
        v_geo_line_t            m_lines;            // RECORDS
};

typedef std::vector<geo_entry_t>      v_geo_entry_t;
typedef std::list<geo_entry_t>        l_geo_entry_t;

#if 0
class paint_entry_t {

    public:
        paint_entry_t () {
            clear();
        }

        void clear() {
            m_record_type  = OBJID_ERROR;           // RECORD:AREA
            m_default_type = OBJID_ERROR;           // XTYPE:ASPHALT
            m_lines.clear();
        }

    public:
        obj_type_t              m_record_type;      // RECORD:AREA
        obj_type_t              m_default_type;     // XTYPE:ASPHALT
        v_paint_line_t          m_lines;            // 
};

typedef std::vector<paint_entry_t>      v_paint_entry_t;
typedef std::list<paint_entry_t>        l_paint_entry_t;

#endif


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

class geo_idx_rec_t {

    public:
        geo_rect_t         m_rect;
        v_geo_offset_t     m_list_off;

    public:
        void clear() {
            m_rect.clear();
            m_list_off.clear();
        }
};

typedef std::list<geo_idx_rec_t>       l_geo_idx_rec_t;
typedef std::vector<geo_idx_rec_t>     v_geo_idx_rec_t;
typedef std::vector<v_geo_idx_rec_t>   vv_geo_idx_rec_t;

typedef std::vector<geo_offset_t>      v_geo_offset_t;
typedef std::vector<v_geo_offset_t>    vv_geo_offset_t;

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
    { "",  "ASPHALT",               OBJID_TYPE_ASPHALT          },
    { "",  "WATER",                 OBJID_TYPE_WATER            },
    { "",  "FOREST",                OBJID_TYPE_FOREST           },
    { "",  "GRASS",                 OBJID_TYPE_GRASS            },
    { "",  "GENERAL",               OBJID_TYPE_GENERAL          },
    { "",  "MOUNTAIN",              OBJID_TYPE_MOUNTAIN         },
    { "",  "STONE",                 OBJID_TYPE_STONE            },
    { "",  "SAND",                  OBJID_TYPE_SAND             },
    { "",  "UNDEFINED",             OBJID_TYPE_UNDEFINED        },
    { "",  "BUILDING",              OBJID_TYPE_BUILDING         },
    { "",  "FOOTWAY",               OBJID_TYPE_FOOTWAY          },
    { "",  "MOUNTAIN",              OBJID_TYPE_MOUNTAIN         },
    { "",  "ROAD",                  OBJID_TYPE_ROAD             },
    { "",  "SECONDARY",             OBJID_TYPE_SECONDARY        },
    { "",  "SERVICE",               OBJID_TYPE_SERVICE          },
    { "",  "TRUNK",                 OBJID_TYPE_TRUNK            },
    { "",  "MOTORWAY",              OBJID_TYPE_MOTORWAY         },
    { "",  "PRIMARY",               OBJID_TYPE_PRIMARY          },
    { "",  "RESIDENTIAL",           OBJID_TYPE_RESIDENTIAL      },
    { "",  "STREET",                OBJID_TYPE_STREET           },
    { "",  "UNCLIASSIFIED",         OBJID_TYPE_UNCLIASSIFIED    },
    { "",  "PEDISTRAN",             OBJID_TYPE_PEDISTRAN        },
    { "",  "TRACK",                 OBJID_TYPE_TRACK            },
    { "",  "STEPS",                 OBJID_TYPE_STEPS            },
    { "",  "PATH",                  OBJID_TYPE_PATH             },
    { "",  "TERTIARY",              OBJID_TYPE_TERTIARY         },
    { "",  "RAILWAY",               OBJID_TYPE_RAILWAY          },
    { "",  "RIVER",                 OBJID_TYPE_RIVER            },
    { "",  "BRIDGE",                OBJID_TYPE_BRIDGE           },
    { "",  "TUNNEL",                OBJID_TYPE_TUNNEL           },
    { "",  "TREEROW",               OBJID_TYPE_TREEROW          },
};

static const lex_ctx_t g_lex_idx[] = {

    { KEYNAME_MAP,          KEYPARAM_BEGIN,     OBJID_IDX_MAP_BEGIN     },
    { KEYNAME_MAP_MIN,      "",                 OBJID_IDX_MAP_MIN       },
    { KEYNAME_MAP_MAX,      "",                 OBJID_IDX_MAP_MAX       },
    { KEYNAME_MAP_XCNT,     "",                 OBJID_IDX_MAP_XCNT      },
    { KEYNAME_MAP_YCNT,     "",                 OBJID_IDX_MAP_YCNT      },
    { KEYNAME_MAP_XSTEP,    "",                 OBJID_IDX_MAP_XSTEP     },
    { KEYNAME_MAP_YSTEP,    "",                 OBJID_IDX_MAP_YSTEP     },
    { KEYNAME_MAP,          KEYPARAM_END,       OBJID_IDX_MAP_END       },

    { KEYNAME_INDEX,        KEYPARAM_RECT,      OBJID_IDX_BEGIN         },
    { KEYNAME_POSITION,     "",                 OBJID_IDX_RECT          },
    { KEYNAME_OFFSETS,      KEYPARAM_BEGIN,     OBJID_IDX_MEMBERS_BEGIN },
    { KEYNAME_MEMBER,       "",                 OBJID_IDX_MEMBER        },
    { KEYNAME_OSM_REF,      "",                 OBJID_XREF              },
    { KEYNAME_OFFSETS,      KEYPARAM_END,       OBJID_IDX_MEMBERS_END   },
    { KEYNAME_INDEX,        KEYPARAM_END,       OBJID_IDX_END           },
};

class geo_parser_t {

    private:
        geo_param_t     m_geo_param;
 
    public:
        void load_param ( char ch, bool& eo_cmd, geo_offset_t file_offset ) {
            m_geo_param._load_lex ( ch, eo_cmd, file_offset );
        }

        void process_map ( geo_entry_t& geo_obj, bool& eor ) {

            obj_type_t         code = OBJID_ERROR;
            geo_coord_t        geo_coords;
            v_geo_coord_t      empty_path;
            obj_type_t         type;
            uint64_t           osm_id;
            static size_t      record_id;   // = geo_obj.m_record_id;
            static int         stop_cnt = 0;

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
                    record_id = 0;
                    geo_obj.clear();
                    geo_obj.m_record_type = code;
                    geo_obj.m_data_off = m_geo_param.param.off;
                    break;

                case OBJID_XTYPE:
                    _map_type(m_geo_param.value.msg, type);
                    geo_obj.m_default_type = type;
                    break;

                case OBJID_XCNT:
                    uint32_t cnt;
                    sscanf_s ( m_geo_param.value.msg, "%d", &cnt );
                    if ( cnt != 1 ) {
                        stop_cnt++;
                    }
                    geo_obj.m_lines.resize ( cnt );
                    break;

                case OBJID_XREF:
                    sscanf_s(m_geo_param.value.msg, "%lld", &osm_id);
                    geo_obj.m_osm_ref = osm_id;
                    break;

                case OBJID_ROLE_OUTER:
                case OBJID_ROLE_INNER:
                    geo_obj.m_lines[record_id].m_role = code;
                    break;

                case OBJID_TYPE:
                    _map_type(m_geo_param.value.msg, type);
                    if ( type == OBJID_TYPE_UNDEFINED ) {
                        type = geo_obj.m_default_type;
                    }
                    geo_obj.m_lines[record_id].m_type = type;
                    break;

                case OBJID_SIZE:
                    uint32_t size;
                    sscanf_s(m_geo_param.value.msg, "%d", &size );
                    geo_obj.m_lines[record_id].m_area = size;
                    break;

                case OBJID_COORDS:
                    _load(geo_coords);
                    geo_obj.m_lines[record_id].m_coords.push_back(geo_coords);
                    break;

                case OBJID_ROLE_END:
                    record_id++;
                    break;

                case OBJID_RECORD_END:  
                    eor = true;
                    break;
            }

            m_geo_param.reset();
        }

        void process_idx ( geo_idx_rec_t& geo_idx, bool& eor ) {

            obj_type_t     code = OBJID_ERROR;
            // uint64_t    ref;
            // double      p1, p2, p3, p4;

            eor = false;

            for (size_t i = 0; i < CNT(g_lex_idx); i++) {
                if (_cmp(&g_lex_idx[i], code)) {
                    break;
                }
            }

            switch ( code ) {

                #if 0

                case OBJID_IDX_MAP_BEGIN:
                    geo_idx.m_map.min.clear();
                    geo_idx.m_map.max.clear();
                    geo_idx.m_map_x_cnt  = 0;
                    geo_idx.m_map_x_step = 0;
                    geo_idx.m_map_y_cnt  = 0;
                    geo_idx.m_map_y_step = 0;
                    break;

                case OBJID_IDX_MAP_MIN:
                    sscanf_s ( m_geo_param.value.msg, "%lf %lf %lf %lf", &p1, &p2, &p3, &p4 );
                    geo_idx.m_map.min.set_y ( pos_type_t::POS_TYPE_GPS, p1 );
                    geo_idx.m_map.min.set_x ( pos_type_t::POS_TYPE_GPS, p2 );
                    geo_idx.m_map.min.set_y ( pos_type_t::POS_TYPE_MAP, p3 );
                    geo_idx.m_map.min.set_x ( pos_type_t::POS_TYPE_MAP, p4 );
                    break;

                case OBJID_IDX_MAP_MAX:
                    sscanf_s ( m_geo_param.value.msg, "%lf %lf %lf %lf", &p1, &p2, &p3, &p4 );
                    geo_idx.m_map.max.set_y ( pos_type_t::POS_TYPE_GPS, p1 );
                    geo_idx.m_map.max.set_x ( pos_type_t::POS_TYPE_GPS, p2 );
                    geo_idx.m_map.max.set_y ( pos_type_t::POS_TYPE_MAP, p3 );
                    geo_idx.m_map.max.set_x ( pos_type_t::POS_TYPE_MAP, p4 );
                    break;

                case OBJID_IDX_MAP_XCNT:
                    sscanf_s ( m_geo_param.value.msg, "%d", &geo_idx.m_map_x_cnt );
                    break;

                case OBJID_IDX_MAP_YCNT:
                    sscanf_s ( m_geo_param.value.msg, "%d", &geo_idx.m_map_y_cnt );
                    break;

                case OBJID_IDX_MAP_XSTEP:
                    sscanf_s ( m_geo_param.value.msg, "%lf", &geo_idx.m_map_x_step );
                    break;

                case OBJID_IDX_MAP_YSTEP:
                    sscanf_s ( m_geo_param.value.msg, "%lf", &geo_idx.m_map_y_step );
                    break;

                #endif

                case OBJID_IDX_MAP_END:
                    eor = true;
                    break;

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

                #if 0
                case OBJID_XREF:
                    sscanf_s(m_geo_param.value.msg, "%lld", &ref);
                    geo_idx.m_osm_id = ref;
                    break;
                #endif

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

            double  gps_y, gps_x;
            int32_t map_y, map_x;

            sscanf_s ( m_geo_param.value.msg,  "%lf %lf %d %d",  &gps_y, &gps_x, &map_y, &map_x ); 

            coords.set_geo_y ( gps_y );
            coords.set_geo_x ( gps_x );
            coords.set_map_y ( map_y );
            coords.set_map_x ( map_x );

            coords.reset_angle();
        }

        void _map_type ( const char* const key, obj_type_t& type ) {

            static int stop_cnt = 0;

            type = OBJID_ERROR;
            for (size_t i = 0; i < CNT(g_lex_types); i++) {
                if (strcmp(g_lex_types[i].v, key) == 0) {
                    type = g_lex_types[i].n;
                    break;
                }
            }

            if ( type == OBJID_ERROR ) {
                stop_cnt++;
            }

            assert ( type != OBJID_ERROR );
        }

};

#endif
