#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>

#include "geo_lex.h"
#include "geo_obj.h"

#include "MemMapper.h"

#define CNT(x)   ( sizeof(x) / sizeof(x[0]) )

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

class geo_coords_t {
    public:
        double  lon;
        double  lat;

    public:
        double x(void)  { return lon; }
        double y(void)  { return lat; }
};

typedef std::vector<geo_coords_t>   list_geo_ccords_t;

class geo_line_t {

    public:
        obj_id_t            role;
        obj_id_t            type;
        uint32_t            area;
        list_geo_ccords_t   coords;

    public:
        void clear() {
            role    = OBJID_UNDEF;
            type    = OBJID_UNDEF;
            area    = 0;
            coords.clear();
        }
};

typedef std::list<geo_line_t>   list_geo_line_t;

class geo_obj_t {

    public:
        obj_id_t            record;
        obj_id_t            type;
        list_geo_line_t     lines;
        size_t              off;

    public:
        void clear() {
            record  = OBJID_UNDEF;
            type    = OBJID_UNDEF;
            lines.clear();
        }
};

typedef std::list<geo_obj_t>   list_geo_obj_t;

lex_t               g_param;
lex_t               g_value;

geo_line_t          g_geo_line;
geo_obj_t           g_geo_obj;
list_geo_obj_t      g_geo_obj_list;

bool                g_direction     = false;
double              g_lon_min       = 0;
double              g_lon_max       = 0;
double              g_lat_min       = 0;
double              g_lat_max       = 0;
size_t              g_file_offset   = 0;
bool                g_geo_dir       = false;
int                 g_geo_scale     = 500;
double              g_step_ver      = 0;
double              g_step_hor      = 0;

static double toRadians (double val) {

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

static void _load ( geo_coords_t& geo_coords ) {

    sscanf_s ( g_value.msg, "%lf %lf", &geo_coords.lat, &geo_coords.lon );
}

static void _load ( uint32_t& geo_coords ) {

    sscanf_s ( g_value.msg, "%d", &geo_coords );
}

static void _commit_lex () {

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

static void _find_box () {

    bool first_entry  = true;

    auto it_obj = g_geo_obj_list.begin();
    while ( it_obj != g_geo_obj_list.end() ) {

        auto it_line = it_obj->lines.begin();
        while ( it_line != it_obj->lines.end() ) {

            auto it_coord = it_line->coords.begin();
            while ( it_coord != it_line->coords.end() ) {

                if ( first_entry ) {

                    first_entry = false;

                    g_lon_min = g_lon_max = it_coord->lon;
                    g_lat_min = g_lat_max = it_coord->lat;

                } else {

                    g_lon_min = min(g_lon_min, it_coord->lon);
                    g_lon_max = max(g_lon_max, it_coord->lon);

                    g_lat_min = min(g_lat_min, it_coord->lat);
                    g_lat_max = max(g_lat_max, it_coord->lat);
                }

                it_coord++;

            }

            it_line++;
        }

        it_obj++;

    }

}

static void _find_scale () {

    {   double  len_left   =  _delta ( g_lon_min, g_lat_min, g_lon_min, g_lat_max );
        double  len_right  =  _delta ( g_lon_max, g_lat_min, g_lon_max, g_lat_max );
        double  len_ver    =  min ( len_left, len_right );
        double  ver_cnt    =  len_ver / g_geo_scale;
        double  delta_ver  =  max ( g_lat_max, g_lat_min ) - min ( g_lat_max, g_lat_min );

        g_step_ver = delta_ver / ver_cnt;
    }

    {   double  len_bottom =  _delta ( g_lon_min, g_lat_min, g_lon_max, g_lat_min );
        double  len_top    =  _delta ( g_lon_min, g_lat_max, g_lon_max, g_lat_max );
        double  len_hor    =  min ( len_bottom, len_top );
        double  hor_cnt    =  len_hor / g_geo_scale;
        double  delta_hor  =  max ( g_lon_max, g_lon_min ) - min ( g_lon_max, g_lon_min );

        g_step_hor = delta_hor / hor_cnt;
    }

}

static geo_coords_t getIntersection ( geo_coords_t A, geo_coords_t B, geo_coords_t C, geo_coords_t D ) {

    double  a1 = B.y() - A.y();
    double  b1 = A.x() - B.x();
    double  c1 = a1 * A.x() + b1 * A.y();

    double  a2 = D.y() - C.y();
    double  b2 = C.x() - D.x();
    double  c2 = a2 * C.x() + b2 * C.y();

    double det = a1 * b2 - a2 * b1;

    if (det == 0) {
        return { 0, 0 };
    } else {
        double x = (b2 * c1 - b1 * c2) / det;
        double y = (a1 * c2 - a2 * c1) / det;
        return { x, y };
    }
}

static list_geo_ccords_t clipAgainstEdge ( list_geo_ccords_t polygon, geo_coords_t edgeStart, geo_coords_t edgeEnd) {

    list_geo_ccords_t clippedPolygon;
    size_t    numVertices = polygon.size();
    geo_coords_t   S = polygon[numVertices - 1];

    for (int i = 0; i < numVertices; i++) {

        geo_coords_t E = polygon[i];

        double S_dist = (S.x() - edgeStart.x()) * (edgeEnd.y() - edgeStart.y()) - (S.y() - edgeStart.y()) * (edgeEnd.x() - edgeStart.x());
        double E_dist = (E.x() - edgeStart.x()) * (edgeEnd.y() - edgeStart.y()) - (E.y() - edgeStart.y()) * (edgeEnd.x() - edgeStart.x());
        if (E_dist < 0) {
            if (S_dist >= 0) {
                auto pt = getIntersection(S, E, edgeStart, edgeEnd);
                clippedPolygon.push_back(pt);
            }
            clippedPolygon.push_back(E);
        } else
        if (S_dist < 0) {
            auto pt = getIntersection(S, E, edgeStart, edgeEnd);
            clippedPolygon.push_back(pt);
        }

        S = E;
    }

    return clippedPolygon;
}

static list_geo_ccords_t _clip ( const list_geo_ccords_t& subjectPolygon, const list_geo_ccords_t& clipPolygon ) {

    list_geo_ccords_t clippedPolygon = subjectPolygon;

    for (int i = 0; i < clipPolygon.size() - 1; i++) {

        geo_coords_t edgeStart = clipPolygon[i + 0];
        geo_coords_t edgeEnd = clipPolygon[i + 1];

        clippedPolygon = clipAgainstEdge ( clippedPolygon, edgeStart, edgeEnd );

        if (clippedPolygon.size() == 0) {
            break;
        }
    }

    return clippedPolygon;
}

static void _check_line ( const list_geo_ccords_t& coords, list_geo_ccords_t& resPolygon ) {

    list_geo_ccords_t rect;
    list_geo_ccords_t res;
    geo_coords_t      pt;
    int               ok = 0;

    double x_min  = g_lon_min;
    double x_max  = g_lon_max - g_step_hor;
    double x_step = g_step_hor;

    double y_min  = g_lat_min;
    double y_max  = g_lat_max - g_step_ver;
    double y_step = g_step_ver;

    for ( double y = y_min; y <= g_lat_max; y += g_step_ver ) {
        for ( double x = x_min; x <= x_max; x += x_step ) {

            rect.clear();

            pt.lon = x + x_step; pt.lat = y;               rect.push_back(pt);
            pt.lon = x + x_step; pt.lat = y + g_step_ver;  rect.push_back(pt);
            pt.lon = x;          pt.lat = y + g_step_ver;  rect.push_back(pt);
            pt.lon = x;          pt.lat = y;               rect.push_back(pt);
            pt.lon = x + x_step; pt.lat = y;               rect.push_back(pt);

            res = _clip ( coords, rect );

            if (res.size() != 0) {
                ok++;
            }

        }
    }
}

static void _slicing() {

    list_geo_ccords_t resPolygon;

    auto geo_obj_it = g_geo_obj_list.begin();

    while ( geo_obj_it != g_geo_obj_list.end() ) {
        auto line_it = geo_obj_it->lines.begin();
        while ( line_it != geo_obj_it->lines.end() ) {
            _check_line ( line_it->coords, resPolygon );
            line_it++;
        }
        geo_obj_it++;
    }
}

int main() {

    file_mapper file;
    uint64_t    file_size;
    bool        eo_cmd;
    char        ch;

    file.Init ( "C:\\GitHub\\map_manager\\dev\\_bin\\log_short.txt" );
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
