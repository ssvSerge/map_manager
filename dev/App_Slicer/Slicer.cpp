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
#include <geo_tools.h>
#include <lex_keys.h>
#include <geo_projection.h>


#define CNT(x)      ( sizeof(x) / sizeof(x[0]) )

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
geo_entry_t                 g_geo_record;
l_geo_entry_t               g_geo_record_list;
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

static double _gps_distance ( double lon1, double lat1, double lon2, double lat2 ) {

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

    map_pos_t p_next;

    bool first_entry = true;

    for ( auto record = g_geo_record_list.cbegin(); record != g_geo_record_list.cend(); record++ ) {
        for ( auto line = record->m_lines.cbegin(); line != record->m_lines.cend(); line++ ) {
            for ( auto coord = line->m_coords.cbegin(); coord != line->m_coords.cend(); coord++ ) {

                coord->get ( p_next, POS_TYPE_GPS );

                if ( first_entry ) {
                    first_entry = false;
                    g_x_min = g_x_max = p_next.x;
                    g_y_min = g_y_max = p_next.y;
                } else {
                    g_x_min = std::min ( g_x_min, p_next.x );
                    g_x_max = std::max ( g_x_max, p_next.x );
                    g_y_min = std::min ( g_y_min, p_next.y );
                    g_y_max = std::max ( g_y_max, p_next.y );
                }
            }
        }
    }

}

static void _find_scale ( void ) {

    {   double  len_left   =  _gps_distance ( g_x_min, g_y_min, g_x_min, g_y_max );
        double  len_right  =  _gps_distance ( g_x_max, g_y_min, g_x_max, g_y_max );
        double  len_ver    =  std::min ( len_left, len_right );
        double  ver_cnt    =  len_ver / g_geo_scale;
        double  delta_ver  =  std::max ( g_y_max, g_y_min ) - std::min ( g_y_max, g_y_min );

        g_step_ver = delta_ver / ver_cnt;
    }

    {   double  len_bottom  =  _gps_distance ( g_x_min, g_y_min, g_x_max, g_y_min );
        double  len_top     =  _gps_distance ( g_x_min, g_y_max, g_x_max, g_y_max );
        double  len_hor     =  std::min ( len_bottom, len_top );
        double  hor_cnt     =  len_hor / g_geo_scale;
        double  delta_hor   =  std::max ( g_x_max, g_x_min ) - std::min ( g_x_max, g_x_min );

        g_step_hor = delta_hor / hor_cnt;
    }
}

static void _log_index ( const geo_rect_t& in_rect, size_t id ) {

    map_pos_t gps_min, map_min;
    map_pos_t gps_max, map_max;

    char tmp[160] = { 0 };

    v_geo_offset_t*  res_ptr = nullptr;

    res_ptr = &g_scan_result[id];

    if ( res_ptr->size() == 0 ) {
        return;
    }

    in_rect.min.get ( gps_min, POS_TYPE_GPS );
    in_rect.max.get ( gps_max, POS_TYPE_GPS );
    in_rect.min.get ( map_min, POS_TYPE_MAP );
    in_rect.max.get ( map_max, POS_TYPE_MAP );

    sprintf_s ( 
        tmp, sizeof(tmp)-1, 
        "%lf %lf %lf %lf %lf %lf %lf %lf", 
        gps_min.y, gps_min.x, gps_max.y, gps_max.x, 
        map_min.y, map_min.x, map_max.y, map_max.x 
    );

    _log_pair ( KEYNAME_INDEX,    KEYPARAM_RECT,  true );
    _log_pair ( KEYNAME_POSITION, tmp,            true );
    _log_pair ( KEYNAME_OFFSETS,  KEYPARAM_BEGIN, false );

    auto item_ptr = res_ptr->cbegin();
    while ( item_ptr != res_ptr->cend() ) {
        _log_pair ( KEYNAME_MEMBER, *item_ptr, false );
        item_ptr++;
    }

    _log_pair ( KEYNAME_OFFSETS, KEYPARAM_END, true );
    _log_pair ( KEYNAME_INDEX,   KEYPARAM_END, true );

    std::cout << std::endl;
}

static void _scan_rect ( const geo_rect_t& in_rect, size_t id ) {

    vv_geo_coord_t  tmp;

    geo_coord_t     dummy_pt;
    v_geo_coord_t   dummy_rect;

    #if 0
    {   map_pos_t    pos_min, pos_max;
        
        in_rect.min.get ( pos_min, POS_TYPE_GPS );
        in_rect.max.get ( pos_max, POS_TYPE_GPS );

        dummy_pt.set(  );

        dummy_pt.lon = in_rect.min.lon; 
        dummy_pt.lat = in_rect.min.lat; 
        dummy_rect.push_back(dummy_pt);

        dummy_pt.lon = in_rect.min.lon; 
        dummy_pt.lat = in_rect.max.lat; 
        dummy_rect.push_back(dummy_pt);

        dummy_pt.lon = in_rect.max.lon;
        dummy_pt.lat = in_rect.max.lat;
        dummy_rect.push_back(dummy_pt);

        dummy_pt.lon = in_rect.max.lon;
        dummy_pt.lat = in_rect.min.lat;
        dummy_rect.push_back(dummy_pt);

        dummy_pt.lon = in_rect.min.lon;
        dummy_pt.lat = in_rect.min.lat;
        dummy_rect.push_back(dummy_pt);
    }
    #endif

    for ( auto record = g_geo_record_list.cbegin(); record != g_geo_record_list.cend(); record++ ) {
        for ( auto line = record->m_lines.cbegin(); line != record->m_lines.cend(); line++ ) {
            
            geo_intersect ( line->m_coords, in_rect, POS_TYPE_GPS, tmp );
            if ( tmp.size() > 0 ) {
                g_scan_result[id].push_back ( record->m_data_off );
            }

        }
    }

    g_pending_cnt--;

    {   std::lock_guard<std::mutex> guard(g_pages_mutex);
        std::cerr << "Pendging cnt: " << g_pending_cnt << "    \r";
    }
}

static void _slicing ( void ) {

    double  x_min   = g_x_min;
    double  x_max   = g_x_max - g_step_hor;
    double  x_step  = g_step_hor;

    double  y_min   = g_y_min;
    double  y_max   = g_y_max - g_step_ver;
    double  y_step  = g_step_ver;

    map_pos_t       pos;
    geo_rect_t      rect;
    v_geo_rect_t    slicer_rects;

    rect.clear();

    for ( double y = y_min; y <= y_max; y += y_step ) {
        for ( double x = x_min; x <= x_max; x += x_step ) {

            pos.x = x;
            pos.y = y;

            rect.min.set ( pos, POS_TYPE_GPS );
            _pt_to_projection(pos);
            rect.min.set(pos, POS_TYPE_MAP);
            rect.min.reset_angle();


            pos.x = x + x_step;
            pos.y = y + y_step;

            rect.max.set ( pos, POS_TYPE_GPS );
            _pt_to_projection(pos);
            rect.max.set(pos, POS_TYPE_MAP);
            rect.max.reset_angle();

            slicer_rects.push_back ( rect );
        }
    }

    g_pending_cnt = slicer_rects.size();

    g_scan_result.resize ( g_pending_cnt );

    thread_pool pool;
    for (int id = 0; id < slicer_rects.size(); id++) {
        pool.push_task ( [slicer_rects, id] { _scan_rect ( slicer_rects[id], id ); });
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

    geo_entry_t     geo_record;

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
