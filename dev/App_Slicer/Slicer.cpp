#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <sstream>
#include <future>
#include <cassert>
#include <mutex>
#include <algorithm>

#include "thread_pool.h"

#include "MemMapper.h"

#include <geo_types.h>
#include <lex_keys.h>

#define CNT(x)   ( sizeof(x) / sizeof(x[0]) )

std::mutex          g_pages_mutex;

geo_offset_t        g_file_offset       = 0;
geo_parser_t        g_geo_parser;

int                 g_geo_scale         = 500;
double              g_x_min             = 0;
double              g_x_max             = 0;
double              g_y_min             = 0;
double              g_y_max             = 0;
bool                g_geo_dir           = false;
double              g_step_ver          = 0;
double              g_step_hor          = 0;

std::atomic<size_t>         g_pending_cnt  = 0;
geo_record_t                g_geo_record;
l_geo_record_t              g_geo_record_list;
vv_geo_offset_t             g_scan_result;

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

    bool first_entry = true;

    auto enum_coords = [&first_entry] ( const geo_coord_t& coord ) {
        if ( first_entry ) {
            first_entry = false;
            g_x_min = g_x_max = coord.x;
            g_y_min = g_y_max = coord.y;
        } else {
            g_x_min = std::min ( g_x_min, coord.x );
            g_x_max = std::max ( g_x_max, coord.x );
            g_y_min = std::min ( g_y_min, coord.y );
            g_y_max = std::max ( g_y_max, coord.y );
        }
    };

    auto enum_lines = [enum_coords] ( const v_geo_coord_t& geo_path ) {
        std::for_each(geo_path.cbegin(), geo_path.cend(), enum_coords);
    };

    auto enum_geo_records = [enum_lines] ( const geo_record_t& geo_record ) {
        std::for_each (
            geo_record.m_geo_lines.cbegin(),
            geo_record.m_geo_lines.cend(),
            enum_lines 
        );
    };

    std::for_each ( g_geo_record_list.cbegin(), g_geo_record_list.cend(), enum_geo_records );
}

static void _find_scale ( void ) {

    {   double  len_left   =  _delta ( g_x_min, g_y_min, g_x_min, g_y_max );
        double  len_right  =  _delta ( g_x_max, g_y_min, g_x_max, g_y_max );
        double  len_ver    =  std::min ( len_left, len_right );
        double  ver_cnt    =  len_ver / g_geo_scale;
        double  delta_ver  =  std::max ( g_y_max, g_y_min ) - std::min ( g_y_max, g_y_min );

        g_step_ver = delta_ver / ver_cnt;
    }

    {   double  len_bottom =  _delta ( g_x_min, g_y_min, g_x_max, g_y_min );
        double  len_top    =  _delta ( g_x_min, g_y_max, g_x_max, g_y_max );
        double  len_hor    =  std::min ( len_bottom, len_top );
        double  hor_cnt    =  len_hor / g_geo_scale;
        double  delta_hor  =  std::max ( g_x_max, g_x_min ) - std::min ( g_x_max, g_x_min );

        g_step_hor = delta_hor / hor_cnt;
    }

}

static void _log_index ( const vv_geo_coord_t& in_rect, size_t id ) {

    char tmp[80];

    v_geo_offset_t*  res_ptr = nullptr;

    res_ptr = &g_scan_result[id];

    if ( res_ptr->size() == 0 ) {
        return;
    }

    sprintf_s(tmp, sizeof(tmp), "%lf %lf %lf %lf", in_rect[0][3].x, in_rect[0][3].y, in_rect[0][1].x, in_rect[0][1].y);

    _log_pair ( KEYNAME_INDEX,    KEYPARAM_RECT,  true );
    _log_pair ( KEYNAME_POSITION, tmp,            true );
    _log_pair ( KEYNAME_OFFSETS,  KEYPARAM_BEGIN, false );


    auto item_ptr = res_ptr->cbegin();
    while ( item_ptr != res_ptr->cend() ) {
        _log_pair(KEYNAME_MEMBER, *item_ptr, false);
        item_ptr++;
    }

    _log_pair ( KEYNAME_OFFSETS, KEYPARAM_END, true );
    _log_pair ( KEYNAME_INDEX,   KEYPARAM_END, true );

    std::cout << std::endl;
}

static void _scan_rect ( const vv_geo_coord_t& in_rect, size_t id ) {

    vv_geo_coord_t  tmp;

    auto it_ptr = g_geo_record_list.cbegin();

    while ( it_ptr != g_geo_record_list.cend() ) {

        tmp = Clipper2Lib::Intersect ( it_ptr->m_geo_lines, in_rect, Clipper2Lib::FillRule::NonZero, 13 );

        if ( tmp.size() > 0 ) {
            g_scan_result[id].push_back(it_ptr->m_prime_off);
        }

        it_ptr++;
    }

    g_pending_cnt--;

    {   std::lock_guard<std::mutex> guard(g_pages_mutex);
        std::cerr << "Pendging cnt: " << g_pending_cnt << "    \r";
    }
}

static void _slicing ( void ) {

    double      x_min   = g_x_min;
    double      x_max   = g_x_max - g_step_hor;
    double      x_step  = g_step_hor;

    double      y_min   = g_y_min;
    double      y_max   = g_y_max - g_step_ver;
    double      y_step  = g_step_ver;

    geo_coord_t         pt;
    vv_geo_coord_t      rect;
    vvv_geo_coord_t     slicer_rects;

    rect.clear();
    rect.resize(1);
    rect[0].resize(5);

    for (double y = y_min; y <= y_max; y += y_step) {
        for (double x = x_min; x <= x_max; x += x_step) {

            pt.x = x + x_step; pt.y = y;               rect[0][0] = pt;
                                                       rect[0][4] = pt;
            pt.x = x + x_step; pt.y = y + g_step_ver;  rect[0][1] = pt;
            pt.x = x;          pt.y = y + g_step_ver;  rect[0][2] = pt;
            pt.x = x;          pt.y = y;               rect[0][3] = pt;

            slicer_rects.push_back ( rect );
        }
    }

    g_pending_cnt = slicer_rects.size();

    g_scan_result.resize ( g_pending_cnt );

    thread_pool pool;

    for (int id = 0; id < slicer_rects.size(); id++) {
        pool.push_task( [slicer_rects, id] { _scan_rect ( slicer_rects[id], id ); });
    }

    pool.wait_for_tasks();

    for (size_t i = 0; i < slicer_rects.size(); i++) {
        _log_index ( slicer_rects[i], i );
    }
}

int main ( int argc, char* argv[] ) {

    file_mapper     file;
    uint64_t        file_size;

    char            ch  = 0;
    bool            eor = false;
    bool            eoc = false;

    geo_record_t    geo_record;

    if ( argc != 2 ) {
        return (-1);
    }

    file.Init ( argv[1] );
    file_size = file.GetSize();

    for ( g_file_offset = 0; g_file_offset < file_size; g_file_offset++ ) {

        ch = file [ g_file_offset ];

        g_geo_parser.load_param ( ch, eoc, g_file_offset );

        if ( eoc ) {
            g_geo_parser.process_map ( geo_record, eor );
            if (eor) {
                g_geo_record_list.push_back(geo_record);
            }
        }

    }

    _find_box();
    _find_scale();

    _slicing();

    return 0;
}
