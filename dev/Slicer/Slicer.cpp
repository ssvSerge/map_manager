#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <sstream>
#include <future>
#include <cassert>

#include <clipper2/clipper.h>

#include "geo_lex.h"
#include "geo_obj.h"
#include "thread_pool.h"

#include "MemMapper.h"

#define CNT(x)   ( sizeof(x) / sizeof(x[0]) )

typedef std::list<size_t>  list_offset_t;

typedef enum tag_obj_id {

    OBJID_UNDEF, 

    OBJID_RECORD_AREA,
    OBJID_RECORD_BUILDING,
    OBJID_RECORD_HIGHWAY,
    OBJID_RECORD_END,
    OBJID_ROLE_OUTER,
    OBJID_ROLE_INNER,
    OBJID_ROLE_COORDS,
    OBJID_ROLE_END,
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
    OBJID_AREA_SIZE,
    OBJID_COORD,

    OBJID_LAST_ID

}   obj_id_t;

typedef struct tag_lex_ctx {
    const char*     p;
    const char*     v;
    obj_id_t        n;
}   lex_ctx_t;

typedef Clipper2Lib::PointD   geo_coords_t;
typedef Clipper2Lib::PathD    geo_path_t;
typedef Clipper2Lib::PathsD   geo_set_t;

class geo_line_t {

    public:
        uint32_t            off;

    public:
        obj_id_t            role;
        obj_id_t            type;
        uint32_t            area;
        geo_path_t          coords;

    public:
        void clear() {
            role    = OBJID_UNDEF;
            type    = OBJID_UNDEF;
            area    = 0;
            coords.clear();
        }
};

typedef std::list<geo_line_t>   list_geo_lines_t;

class geo_obj_t {

    public:
        obj_id_t            record;
        obj_id_t            type;
        list_geo_lines_t    lines;
        size_t              off;

    public:
        void clear() {
            record  = OBJID_UNDEF;
            type    = OBJID_UNDEF;
            lines.clear();
        }
};

typedef std::list<geo_obj_t>            list_geo_objs_t;
typedef std::future<list_offset_t>      future_res_t;
typedef std::vector<geo_set_t>          geo_map_t;
typedef std::vector<future_res_t>       list_future_res_t;


lex_t                       g_param;
lex_t                       g_value;

geo_line_t                  g_geo_line;
geo_obj_t                   g_geo_obj;
list_geo_objs_t             g_geo_obj_list;

size_t                      g_file_offset       = 0;
int                         g_geo_scale         = 500;
bool                        g_direction         = false;
double                      g_x_min             = 0;
double                      g_x_max             = 0;
double                      g_y_min             = 0;
double                      g_y_max             = 0;
bool                        g_geo_dir           = false;
double                      g_step_ver          = 0;
double                      g_step_hor          = 0;
std::atomic<size_t>         g_active_cnt        = 0;


static void _log_pair ( const char* const key, const char* const val, bool cr ) {

    std::cout << key << ":" << val << ";";

    if ( cr ) {
        std::cout << std::endl;
    }
}

static void _log_pair ( const char* const key, double val, bool cr ) {

    char fmt[50];

    sprintf_s ( fmt, sizeof(fmt), "%.7f", val );

    _log_pair( key, fmt, cr );
}

static void _log_pair ( const char* const key, size_t val, bool cr ) {

    char fmt[50];

    sprintf_s(fmt, sizeof(fmt), "%zd", val);

    _log_pair(key, fmt, cr);
}

static double toRadians ( double val ) {

    return val / 57.295779513082325;
}

static double _delta ( double lon1, double lat1, double lon2, double lat2 ) {

    double R = 6371e3;
    double f1 = toRadians(lat1);
    double f2 = toRadians(lat2);
    double dF = toRadians(lat2 - lat1);
    double dL = toRadians(lon2 - lon1);

    double a = sin(dF / 2) * sin(dF / 2) + cos(f1) * cos(f2) * sin(dL / 2) * sin(dL / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double d = R * c;

    return d;
}

static void _reset ( lex_t& lex ) {
    lex.msg[0] = 0;
    lex.pos = 0;
    return;
}

static void _reset ( void ) {

    g_direction = false;

    _reset ( g_param );
    _reset ( g_value );
}

static void _store ( lex_t& lex, char ch ) {

    if ( lex.pos == 0 ) {
        lex.off = g_file_offset;
    }

    lex.msg[lex.pos] = ch;
    lex.pos++;
    lex.msg[lex.pos] = 0;
}

static void _load_lex ( char ch, bool& eo_cmd ) {

    eo_cmd  = false;

    if ( ch == 0x0D ) {
        return;
    }
    if ( ch == 0x0A ) {
        return;
    }

    if ( ch == ':' ) {
        g_direction = true;
        return;
    }
    if ( ch == ';' ) {
        eo_cmd = true;
        return;
    }

    if ( !g_direction ) {
        _store ( g_param, ch );
    } else {
        _store ( g_value, ch );
    }

    return;
}

static bool _cmp ( const lex_ctx_t* inp, obj_id_t& val ) {

    if (strcmp ( inp->p, g_param.msg) != 0) {
        return false;
    }

    if ( inp->v[0] != 0 ) {
        if (strcmp(inp->v, g_value.msg) != 0) {
            return false;
        }
    }

    val = inp->n;
    return true;
}

static void _load ( geo_coords_t& coords ) {

    sscanf_s ( g_value.msg, "%lf %lf", &coords.y, &coords.x );
}

static void _load ( uint32_t& geo_coords ) {

    sscanf_s ( g_value.msg, "%d", &geo_coords );
}

static void _commit_lex ( void ) {

    const lex_ctx_t l_lex[] = {

        { "RECORD",     "AREA",        OBJID_RECORD_AREA },
        { "RECORD",     "BUILDING",    OBJID_RECORD_BUILDING },
        { "RECORD",     "HIGHWAY",     OBJID_RECORD_HIGHWAY },
        { "RECORD",     "END",         OBJID_RECORD_END },

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
        { "C",          "",            OBJID_COORD },
    };

    obj_id_t        code = OBJID_UNDEF;
    geo_coords_t    geo_coords;
    uint32_t        geo_area;

    for (size_t i = 0; i < CNT(l_lex); i++ ) {

        if ( _cmp( &l_lex[i], code) ) {
            break;
        }
    }

    switch ( code ) {

        case OBJID_RECORD_AREA:
        case OBJID_RECORD_BUILDING:
        case OBJID_RECORD_HIGHWAY:
            g_geo_obj.off = g_param.off;
            g_geo_obj.record = code;
            g_geo_dir = true;
            break;

        case OBJID_RECORD_END:
            g_geo_obj_list.push_back(g_geo_obj);
            g_geo_obj.clear();
            break;

        case OBJID_ROLE_OUTER:
        case OBJID_ROLE_INNER:
        case OBJID_ROLE_COORDS:
            g_geo_line.role = code;
            g_geo_dir = false;
            break;

        case OBJID_COORD:
            _load ( geo_coords );
            g_geo_line.coords.push_back(geo_coords);
            break;

        case OBJID_ROLE_END:
            g_geo_obj.lines.push_back(g_geo_line);
            g_geo_line.clear();
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
            if (g_geo_dir) {
                g_geo_obj.type = code;
            } else {
                g_geo_line.type = code;
            }
            break;

        case OBJID_AREA_SIZE:
            _load ( geo_area );
            g_geo_line.area = geo_area;
            break;

    }

    _reset();
}

static void _find_box ( void ) {

    bool first_entry  = true;

    auto it_obj = g_geo_obj_list.begin();
    while ( it_obj != g_geo_obj_list.end() ) {

        auto it_line = it_obj->lines.begin();
        while ( it_line != it_obj->lines.end() ) {

            auto it_coord = it_line->coords.begin();
            while ( it_coord != it_line->coords.end() ) {

                if ( first_entry ) {

                    first_entry = false;

                    g_x_min = g_x_max = it_coord->x;
                    g_y_min = g_y_max = it_coord->y;

                } else {

                    g_x_min = min ( g_x_min, it_coord->x );
                    g_x_max = max ( g_x_max, it_coord->x );

                    g_y_min = min ( g_y_min, it_coord->y );
                    g_y_max = max ( g_y_max, it_coord->y );
                }

                it_coord++;

            }

            it_line++;
        }

        it_obj++;

    }

}

static void _find_scale ( void ) {

    {   double  len_left   =  _delta ( g_x_min, g_y_min, g_x_min, g_y_max );
        double  len_right  =  _delta ( g_x_max, g_y_min, g_x_max, g_y_max );
        double  len_ver    =  min ( len_left, len_right );
        double  ver_cnt    =  len_ver / g_geo_scale;
        double  delta_ver  =  max ( g_y_max, g_y_min ) - min ( g_y_max, g_y_min );

        g_step_ver = delta_ver / ver_cnt;
    }

    {   double  len_bottom =  _delta ( g_x_min, g_y_min, g_x_max, g_y_min );
        double  len_top    =  _delta ( g_x_min, g_y_max, g_x_max, g_y_max );
        double  len_hor    =  min ( len_bottom, len_top );
        double  hor_cnt    =  len_hor / g_geo_scale;
        double  delta_hor  =  max ( g_x_max, g_x_min ) - min ( g_x_max, g_x_min );

        g_step_hor = delta_hor / hor_cnt;
    }

}

static void _log_index ( const geo_set_t& in_rect, const std::list<size_t>& items_list ) {

    if ( items_list.size() == 0 ) {
        return;
    }

    _log_pair ( "IDX", "RECT", true );

        _log_pair ( "X1", in_rect[0][3].x, false );
        _log_pair ( "Y1", in_rect[0][3].y, false );
        _log_pair ( "X2", in_rect[0][1].x, false );
        _log_pair ( "Y2", in_rect[0][1].y, true  );

        _log_pair ( "OFF", "BEGIN", false );

            auto item_ptr = items_list.begin();
            while ( item_ptr != items_list.end() ) {
                _log_pair("O", *item_ptr, false);
                item_ptr++;
            }

        _log_pair ( "OFF", "END", true );

    _log_pair ( "IDX", "END", true );

    std::cout << std::endl;
}

static list_offset_t _scan_rect ( const geo_set_t& in_rect ) {

    list_offset_t   out_list;

    geo_set_t       in_area;
    geo_set_t       out_res;
    int             item_id = 0;

    auto geo_obj_it = g_geo_obj_list.begin();

    while ( geo_obj_it != g_geo_obj_list.end() ) {

        in_area.clear();

        auto geo_path_it = geo_obj_it->lines.begin();
        while ( geo_path_it != geo_obj_it->lines.end() ) {
            in_area.push_back(geo_path_it->coords);
            geo_path_it++;
        }

        out_res = Clipper2Lib::Intersect ( in_area, in_rect, Clipper2Lib::FillRule::NonZero, 13 );
        if ( out_res.size() > 0 ) {
            out_list.push_back ( geo_obj_it->off );
        }

        item_id++;

        geo_obj_it++;
    }

    // _log_index ( in_rect, out_list );
    g_active_cnt--;

    return out_list;
}

static void _slicing ( void ) {

    geo_coords_t      pt;
    geo_path_t        path;
    geo_set_t         rect;
    geo_set_t         res;

    int     ok      = 0;

    double  x_min   = g_x_min;
    double  x_max   = g_x_max - g_step_hor;
    double  x_step  = g_step_hor;

    double  y_min   = g_y_min;
    double  y_max   = g_y_max - g_step_ver;
    double  y_step  = g_step_ver;

    geo_map_t           map_pages;
    future_res_t        slice_res;
    list_future_res_t   list_res;

    for (double y = y_min; y <= y_max; y += g_step_ver) {

        for (double x = x_min; x <= x_max; x += x_step) {

            rect.clear();
            path.clear();

            pt.x = x + x_step; pt.y = y;               path.push_back(pt);
            pt.x = x + x_step; pt.y = y + g_step_ver;  path.push_back(pt);
            pt.x = x;          pt.y = y + g_step_ver;  path.push_back(pt);
            pt.x = x;          pt.y = y;               path.push_back(pt);
            pt.x = x + x_step; pt.y = y;               path.push_back(pt);

            rect.push_back ( path );

            map_pages.push_back(rect);
        }

    }

    g_active_cnt = map_pages.size();

    for (size_t i = 0; i < map_pages.size(); i++) {
        list_res.push_back ( std::async(_scan_rect, map_pages[i]) );
    }

    while (g_active_cnt > 0) {
        Sleep(1000);
        std::cerr << "Pendging cnt: " << g_active_cnt << "      \r";
    }

    for ( size_t i = 0; i < map_pages.size(); i++ ) {
        list_offset_t out_res = list_res[i].get();
        _log_index ( map_pages[i], out_res );
    }

}

int main ( int argc, char* argv[] ) {

    file_mapper file;
    uint64_t    file_size;
    bool        eo_cmd;
    char        ch;

    if (argc != 2) {
        return -1;
    }

    file.Init ( argv[1] );
    file_size = file.GetSize();

    for ( g_file_offset = 0; g_file_offset < file_size; g_file_offset++ ) {

        ch = file [ g_file_offset ];
        _load_lex ( ch, eo_cmd );

        if ( eo_cmd ) {
            _commit_lex ();
        }

    }

    _find_box();
    _find_scale();

    _slicing();

    return 0;
}
