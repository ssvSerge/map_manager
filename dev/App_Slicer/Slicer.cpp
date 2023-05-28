#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <sstream>
#include <future>
#include <cassert>
#include <mutex>

#include "thread_pool.h"

#include <geo/geo_types.h>
#include <geo/geo_lex.h>
#include <geo/geo_obj.h>
#include <geo/geo_param.h>
#include <geo/geo_line.h>
#include <geo/geo_parser.h>

#include "MemMapper.h"

#include "..\common\lex_keys.h"

#define CNT(x)   ( sizeof(x) / sizeof(x[0]) )

geo_obj_map_t   g_geo_obj;
std::mutex      g_pages_mutex;

geo_offset_t    g_file_offset       = 0;
geo_parser_t    g_geo_parser;

int             g_geo_scale         = 500;
double          g_x_min             = 0;
double          g_x_max             = 0;
double          g_y_min             = 0;
double          g_y_max             = 0;
bool            g_geo_dir           = false;
double          g_step_ver          = 0;
double          g_step_hor          = 0;

std::atomic<size_t>  g_pending_cnt  = 0;
list_geo_objs_t      g_geo_obj_list;


static void _log_pair ( const char* const key, const char* const val, bool cr ) {

    std::cout << key << ":" << val << ";";

    if ( cr ) {
        std::cout << std::endl;
    }
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

    double R  = 6371e3;
    double f1 = toRadians(lat1);
    double f2 = toRadians(lat2);
    double dF = toRadians(lat2 - lat1);
    double dL = toRadians(lon2 - lon1);

    double a = sin(dF / 2) * sin(dF / 2) + cos(f1) * cos(f2) * sin(dL / 2) * sin(dL / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double d = R * c;

    return d;
}

static void _find_box ( void ) {

    bool first_entry  = true;

    auto it_obj = g_geo_obj_list.begin();
    while ( it_obj != g_geo_obj_list.end() ) {

        auto it_line = it_obj->m_lines.begin();
        while ( it_line != it_obj->m_lines.end() ) {

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

static void _log_index ( const geo_set_t& in_rect, const list_offset_t& items_list ) {

    char tmp[80];

    if ( items_list.size() == 0 ) {
        return;
    }

    sprintf_s ( tmp, sizeof(tmp), "%lf %lf %lf %lf", in_rect[0][3].x, in_rect[0][3].y, in_rect[0][1].x, in_rect[0][1].y );

    _log_pair ( KEYNAME_INDEX,    KEYPARAM_RECT,  true );
    _log_pair ( KEYNAME_POSITION, tmp,            true );
    _log_pair ( KEYNAME_OFFSETS,  KEYPARAM_BEGIN, false );

        auto item_ptr = items_list.begin();
        while ( item_ptr != items_list.end() ) {
            _log_pair( KEYNAME_MEMBER, *item_ptr, false);
            item_ptr++;
        }

    _log_pair ( KEYNAME_OFFSETS, KEYPARAM_END, true );
    _log_pair ( KEYNAME_INDEX,   KEYPARAM_END, true );

    std::cout << std::endl;
}

static void _scan_rect ( const geo_set_t& in_rect, list_offset_t& res ) {

    list_offset_t   out_list;

    geo_set_t       in_area;
    geo_set_t       out_res;
    int             item_id = 0;

    res.clear();

    auto geo_obj_it = g_geo_obj_list.begin();

    while ( geo_obj_it != g_geo_obj_list.end() ) {

        in_area.clear();

        auto geo_path_it = geo_obj_it->m_lines.begin();
        while ( geo_path_it != geo_obj_it->m_lines.end() ) {
            in_area.push_back(geo_path_it->coords);
            geo_path_it++;
        }

        out_res = Clipper2Lib::Intersect ( in_area, in_rect, Clipper2Lib::FillRule::NonZero, 13 );
        if ( out_res.size() > 0 ) {
            out_list.push_back ( geo_obj_it->m_off );
        }

        item_id++;

        geo_obj_it++;
    }

    res = out_list;

    g_pending_cnt--;

    {   std::lock_guard<std::mutex> guard(g_pages_mutex);
        std::cerr << "Pendging cnt: " << g_pending_cnt << "    \r";
    }
}

static void _slicing ( void ) {

    geo_coords_t      pt;
    geo_path_t        path;
    geo_set_t         rect;
    geo_set_t         res;

    double  x_min   = g_x_min;
    double  x_max   = g_x_max - g_step_hor;
    double  x_step  = g_step_hor;

    double  y_min   = g_y_min;
    double  y_max   = g_y_max - g_step_ver;
    double  y_step  = g_step_ver;

    geo_map_t               map_pages;
    vector_list_offset_t    list_res;

    for ( double y = y_min; y <= y_max; y += y_step ) {

        for ( double x = x_min; x <= x_max; x += x_step ) {

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

    g_pending_cnt = map_pages.size();
    list_res.resize(g_pending_cnt);

    thread_pool pool;

    for (int id = 0; id < map_pages.size(); id++ ) {
        pool.push_task([map_pages, &list_res, id] { _scan_rect( map_pages[id], list_res[id] ); });
    }

    pool.wait_for_tasks();

    for ( size_t i = 0; i < list_res.size(); i++ ) {
        _log_index ( map_pages[i], list_res[i] );
    }

}

int main ( int argc, char* argv[] ) {

    file_mapper     file;
    uint64_t        file_size;

    char            ch;
    geo_obj_map_t   geo_obj;
    bool            eor;
    bool            eoc;

    if (argc != 2) {
        return -1;
    }

    file.Init ( argv[1] );
    file_size = file.GetSize();

    for ( g_file_offset = 0; g_file_offset < file_size; g_file_offset++ ) {

        ch = file [ g_file_offset ];

        g_geo_parser.load_param ( ch, eoc, g_file_offset );

        if ( eoc ) {
            g_geo_parser.process_map ( geo_obj, eor );
            if (eor) {
                g_geo_obj_list.push_back(geo_obj);
                geo_obj.clear();
            }
        }

    }

    _find_box();
    _find_scale();

    _slicing();

    return 0;
}
