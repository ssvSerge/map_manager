#include <cstdint>
#include <algorithm>
#include <mutex>

#include <lex_keys.h>
#include <geo_types.h>
#include <geo_projection.h>
#include <geo_processor.h>
#include <thread_pool.h>
#include <MemMapper.h>

geo_parser_t            g_geo_parser;
l_geo_entry_t           g_geo_record_list;
int                     g_geo_scale     = 500;
double                  g_step_ver      =   0;
double                  g_step_hor      =   0;
geo_offset_t            g_file_offset   =   0;
geo_processor_t         g_geo_processor;

std::mutex              g_pages_mutex;
v_geo_rect_t            g_slicer_rects;
vv_geo_offset_t         g_scan_result;
size_t                  g_scan_id = 0;

map_pos_t               g_gps_min;
map_pos_t               g_gps_max;
map_pos_t               g_map_min;
map_pos_t               g_map_max;


#if 0
#define CNT(x)      ( sizeof(x) / sizeof(x[0]) )


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

#endif


static std::string _to_str ( double val ) {

    std::string ret_val;
    char tmp[50];

    sprintf_s ( tmp, sizeof(tmp) - 1, "%.7lf", val );
    ret_val = tmp;

    return ret_val;
}

static std::string _to_str ( size_t  val ) {

    std::string ret_val;
    char tmp[50];

    sprintf_s(tmp, sizeof(tmp) - 1, "%zi", val);
    ret_val = tmp;

    return ret_val;
}

static void _log_pair ( const char* const key, const char* const val, bool cr ) {

    std::cout << key << ":" << val << ";";

    if ( cr ) {
        std::cout << std::endl;
    }
}

static void _log_pair_i ( const char* const key, size_t val, bool cr ) {

    std::string str;
    str = _to_str ( val );
    _log_pair ( key, str.c_str(), cr);
}

#if 0
static void _log_pair_d ( const char* const key, double val, bool cr ) {

    std::string str;
    str = _to_str ( val );
    _log_pair ( key, str.c_str(), cr);
}
#endif

static void _log_index ( const geo_rect_t& in_rect, size_t id ) {

    map_pos_t gps_min, map_min;
    map_pos_t gps_max, map_max;

    std::string val;

    v_geo_offset_t*  res_ptr = nullptr;

    res_ptr = &g_scan_result[id];

    if ( res_ptr->size() == 0 ) {
        return;
    }

    in_rect.min.get ( POS_TYPE_GPS, gps_min );
    in_rect.max.get ( POS_TYPE_GPS, gps_max );
    in_rect.min.get ( POS_TYPE_MAP, map_min );
    in_rect.max.get ( POS_TYPE_MAP, map_max );

    val += _to_str ( gps_min.y );  val += " ";
    val += _to_str ( gps_min.x );  val += " ";
    val += _to_str ( gps_max.y );  val += " ";
    val += _to_str ( gps_max.x );  val += " ";
    val += _to_str ( map_min.y );  val += " ";
    val += _to_str ( map_min.x );  val += " ";
    val += _to_str ( map_max.y );  val += " ";
    val += _to_str ( map_max.x );

    _log_pair ( KEYNAME_INDEX,    KEYPARAM_RECT,  true );
    _log_pair ( KEYNAME_POSITION, val.c_str(),    true );
    _log_pair ( KEYNAME_OFFSETS,  KEYPARAM_BEGIN, false );

    auto item_ptr = res_ptr->cbegin();
    while ( item_ptr != res_ptr->cend() ) {
        _log_pair_i ( KEYNAME_MEMBER, *item_ptr, false );
        item_ptr++;
    }

    _log_pair ( KEYNAME_OFFSETS, KEYPARAM_END, true );
    _log_pair ( KEYNAME_INDEX,   KEYPARAM_END, true );

    std::cout << std::endl;
}

static void _log_region ( const geo_coord_t& coord_min, const geo_coord_t& coord_max, int x_pages, int y_pages ) {

    #if 0
        std::string pos_min;
        std::string pos_max;

        pos_min += _to_str ( coord_min.y(POS_TYPE_GPS) );  pos_min += " ";
        pos_min += _to_str ( coord_min.x(POS_TYPE_GPS) );  pos_min += " ";
        pos_min += _to_str ( coord_min.y(POS_TYPE_MAP) );  pos_min += " ";
        pos_min += _to_str ( coord_min.x(POS_TYPE_MAP) );

        pos_max += _to_str ( coord_max.y(POS_TYPE_GPS) );  pos_max += " ";
        pos_max += _to_str ( coord_max.x(POS_TYPE_GPS) );  pos_max += " ";
        pos_max += _to_str ( coord_max.y(POS_TYPE_MAP) );  pos_max += " ";
        pos_max += _to_str ( coord_max.x(POS_TYPE_MAP) );

        _log_pair   ( KEYNAME_MAP,       KEYPARAM_BEGIN,  true );
        _log_pair   ( KEYNAME_MAP_MAX,   pos_max.c_str(), true );
        _log_pair   ( KEYNAME_MAP_MIN,   pos_min.c_str(), true );
        _log_pair_i ( KEYNAME_MAP_XCNT,  x_pages,         true );
        _log_pair_i ( KEYNAME_MAP_YCNT,  y_pages,         true );
        _log_pair_d ( KEYNAME_MAP_XSTEP, g_step_hor,      true );
        _log_pair_d ( KEYNAME_MAP_YSTEP, g_step_ver,      true );
        _log_pair   ( KEYNAME_MAP,       KEYPARAM_END,    true );

        std::cout << std::endl;

    #else
        (void) (coord_min);
        (void) (coord_max);
        (void) (x_pages);
        (void) (y_pages);
    #endif
}

static void _scan_rect ( const geo_rect_t& in_rect, v_geo_offset_t& result ) {

    vv_geo_coord_t  tmp;

    geo_coord_t     dummy_pt;
    v_geo_coord_t   dummy_rect;
    bool            close_area;
    int             item_id = 0;

    result.clear();

    for ( auto record = g_geo_record_list.cbegin(); record != g_geo_record_list.cend(); record++ ) {

        switch (record->m_record_type) {
            case OBJID_RECORD_AREA:
                close_area = true;
                break;
            case OBJID_RECORD_BUILDING:
                close_area = true;
                break;
            case OBJID_RECORD_HIGHWAY:
                close_area = false;
                break;
            default:
                close_area = false;
                break;
        }

        for ( auto line = record->m_lines.cbegin(); line != record->m_lines.cend(); line++ ) {

            if ( line->m_coords.size() == 0 ) {
                continue;
            }

            g_geo_processor.geo_intersect ( line->m_coords, in_rect, POS_TYPE_MAP, close_area, tmp );
            if (tmp.size() > 0) {
                result.push_back(record->m_data_off);
            }

        }

        item_id++;
    }

}

static void _workder_func ( void ) {

    for ( ; ; ) {

        size_t          scan_id = 0;
        geo_rect_t      rect;
        v_geo_offset_t  result;

        {   // Load next Object
            std::lock_guard<std::mutex> guard(g_pages_mutex);
            if ( g_scan_id >= g_slicer_rects.size()) {
                break;
            }
            scan_id = g_scan_id;
            rect = g_slicer_rects[scan_id];
            g_scan_id++;
        }

        _scan_rect ( rect, result );

        {   // Load next Object
            std::lock_guard<std::mutex> guard(g_pages_mutex);
            g_scan_result[scan_id] = result;
        }

        {   // Report progres
            std::lock_guard<std::mutex> guard(g_pages_mutex);
            std::cerr << "Pendging cnt: " << g_slicer_rects.size() - scan_id - 1 << "    \r";
        }

    }
}

static void _find_box ( void ) {

    map_pos_t next_gps;
    map_pos_t next_map;

    bool first_entry = true;

    for ( auto record = g_geo_record_list.cbegin(); record != g_geo_record_list.cend(); record++ ) {
        for ( auto line = record->m_lines.cbegin(); line != record->m_lines.cend(); line++ ) {
            for ( auto coord = line->m_coords.cbegin(); coord != line->m_coords.cend(); coord++ ) {

                coord->get ( POS_TYPE_GPS, next_gps );
                coord->get ( POS_TYPE_MAP, next_map );

                if ( first_entry ) {

                    first_entry = false;

                    g_gps_min = g_gps_max = next_gps;
                    g_map_min = g_map_max = next_map;

                } else {

                    g_gps_min.x = min ( g_gps_min.x, next_gps.x );
                    g_gps_min.y = min ( g_gps_min.y, next_gps.y );
                    g_gps_max.x = max ( g_gps_max.x, next_gps.x );
                    g_gps_max.y = max ( g_gps_max.y, next_gps.y );

                    g_map_min.x = min ( g_map_min.x, next_map.x );
                    g_map_min.y = min ( g_map_min.y, next_map.y );
                    g_map_max.x = max ( g_map_max.x, next_map.x );
                    g_map_max.y = max ( g_map_max.y, next_map.y );

                }
            }
        }
    }

}

static void _find_scale ( void ) {

    #if 0
        {   double  len_left    =  gps_distance ( g_gps_min.x, g_gps_min.y, g_gps_min.x, g_gps_max.y );
            double  len_right   =  gps_distance ( g_gps_max.x, g_gps_min.y, g_gps_max.x, g_gps_max.y );
            double  len_ver     =  min ( len_left, len_right );
            double  ver_cnt     =  len_ver / g_geo_scale;
            double  delta_ver   =  max ( g_gps_max.y, g_gps_min.y ) - min ( g_gps_max.y, g_gps_min.y );

            g_step_ver = delta_ver / ver_cnt;
        }

        {   double  len_bottom  =  gps_distance ( g_gps_min.x, g_gps_min.y, g_gps_max.x, g_gps_min.y );
            double  len_top     =  gps_distance ( g_gps_min.x, g_gps_max.y, g_gps_max.x, g_gps_max.y );
            double  len_hor     =  min ( len_bottom, len_top );
            double  hor_cnt     =  len_hor / g_geo_scale;
            double  delta_hor   =  max ( g_gps_max.x, g_gps_min.x ) - min ( g_gps_max.x, g_gps_min.x );

            g_step_hor = delta_hor / hor_cnt;
        }
    #else

        const int scale = 1;
        g_step_ver = scale * 320;
        g_step_hor = scale * 240;

    #endif
}

static void _slicing ( void ) {

    geo_rect_t      map_rect;
    int             x_pages = 0;
    int             y_pages = 0;

    for ( double y = g_map_min.y; y <= g_map_max.y; y += g_step_ver ) {

        y_pages++;
        x_pages = 0;

        for ( double x = g_map_min.x; x <= g_map_max.x; x += g_step_hor ) {

            map_rect.min.set_x ( pos_type_t::POS_TYPE_MAP, x );
            map_rect.min.set_y ( pos_type_t::POS_TYPE_MAP, y );
            map_rect.max.set_x ( pos_type_t::POS_TYPE_MAP, x + g_step_hor );
            map_rect.max.set_y ( pos_type_t::POS_TYPE_MAP, y + g_step_ver );

            map_rect.min.map_to_geo();
            map_rect.max.map_to_geo();

            g_slicer_rects.push_back ( map_rect );

            x_pages++;
        }
    }

    _log_region ( map_rect.min, map_rect.max, x_pages, y_pages );

    g_scan_result.resize ( g_slicer_rects.size() );

    int max_workers_cnt = std::thread::hardware_concurrency() - 2;
    if ( max_workers_cnt < 1 ) {
        max_workers_cnt = 1;
    }

    std::vector<std::thread*>  workders_list;

    for ( int worker = 0; worker < max_workers_cnt; worker++ ) {
        std::thread* th = nullptr;
        th = new std::thread ( _workder_func );
        workders_list.push_back ( th );
    }

    for ( size_t id = 0; id < workders_list.size(); id++ ) {
        workders_list[id]->join();
    }

    for ( size_t i = 0; i < workders_list.size(); i++ ) {
        delete workders_list[i];
    }

    for (size_t i = 0; i < g_slicer_rects.size(); i++) {
        _log_index ( g_slicer_rects[i], i );
    }

}

int main ( int argc, char* argv[] ) {

    file_mapper_t   file;
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
                g_geo_record_list.push_back ( geo_record );
            }
        }

    }

    _find_box();
    _find_scale();
    _slicing();

    return 0;
}
