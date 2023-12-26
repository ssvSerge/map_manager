#include <cstdint>
#include <algorithm>
#include <mutex>
#include <vector>
#include <set>

#include <lex_keys.h>
#include <geo_types.h>
#include <geo_projection.h>
#include <geo_processor.h>
#include <thread_pool.h>
#include <MemMapper.h>

#ifdef min
    #undef min
#endif

#ifdef max
    #undef max
#endif


typedef std::vector<size_t>         v_list_t;
typedef std::list<size_t>           l_list_t;

typedef struct tag_scan_rect {
    v_list_t            obj;
    l_list_t            res;
    geo_rect_t          rect;
}   scan_rect_t;

typedef std::vector<scan_rect_t>    v_scan_rect_t;
typedef std::list<scan_rect_t>      l_scan_rect_t;

geo_parser_t            g_geo_parser;
v_geo_entry_t           g_geo_record_list;
int                     g_geo_scale     = 500;
int32_t                 g_step_ver      =   0;
int32_t                 g_step_hor      =   0;
geo_offset_t            g_file_offset   =   0;
geo_processor_t         g_geo_processor;

std::mutex              g_pages_mutex;
l_scan_rect_t           g_slicer_rects;

geo_pos_t               g_gps_min;
geo_pos_t               g_gps_max;
map_pos_t               g_map_min;
map_pos_t               g_map_max;


static std::string _to_str_d ( double val ) {

    std::string ret_val;
    char tmp[50];

    sprintf_s ( tmp, sizeof(tmp) - 1, "%.7lf", val );
    ret_val = tmp;

    return ret_val;
}

static std::string _to_str_i ( size_t  val ) {

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
    str = _to_str_i ( val );
    _log_pair ( key, str.c_str(), cr);
}

static void _log_index ( const scan_rect_t& in_rect ) {

    map_pos_t   map_min, map_max;
    geo_pos_t   gps_min, gps_max;
    
    std::string val;
    
    gps_min = in_rect.rect.min.geo;
    gps_max = in_rect.rect.max.geo;
    map_min = in_rect.rect.min.map;
    map_max = in_rect.rect.max.map;

    val += _to_str_d ( gps_min.y );  val += " ";
    val += _to_str_d ( gps_min.x );  val += " ";
    val += _to_str_d ( gps_max.y );  val += " ";
    val += _to_str_d ( gps_max.x );  val += " ";
    val += _to_str_i ( map_min.y );  val += " ";
    val += _to_str_i ( map_min.x );  val += " ";
    val += _to_str_i ( map_max.y );  val += " ";
    val += _to_str_i ( map_max.x );
    
    _log_pair ( KEYNAME_INDEX,    KEYPARAM_RECT,  true );
    _log_pair ( KEYNAME_POSITION, val.c_str(),    true );
    _log_pair ( KEYNAME_OFFSETS,  KEYPARAM_BEGIN, false );
    
    auto item_ptr = in_rect.res.cbegin();
    while ( item_ptr != in_rect.res.cend() ) {
        _log_pair_i ( KEYNAME_MEMBER, *item_ptr, false );
        item_ptr++;
    }
    
    _log_pair ( KEYNAME_OFFSETS, KEYPARAM_END, true );
    _log_pair ( KEYNAME_INDEX,   KEYPARAM_END, true );
    
    std::cout << std::endl;
}

static void _scan_rect ( scan_rect_t* const in_rect ) {

    v_geo_line_t    tmp;
    bool            close_area;
    size_t          obj_id;

    for ( size_t pos = 0; pos < in_rect->obj.size(); pos++ ) {
        
        obj_id = in_rect->obj[pos];

        auto record = &g_geo_record_list [obj_id];

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

        for ( auto line = record->m_lines.begin(); line != record->m_lines.end(); line++ ) {

            if ( line->m_coords.size() == 0 ) {
                continue;
            }

            g_geo_processor.geo_intersect ( POS_TYPE_MAP, close_area, *line, in_rect->rect, tmp );
                
            if ( tmp.size() > 0 ) {
                in_rect->res.push_back(record->m_data_off);
                break;
            }

        }

    }

}

static void _find_box ( void ) {

    geo_pos_t  next_gps;
    map_pos_t  next_map;

    bool first_entry = true;

    for ( auto record = g_geo_record_list.cbegin(); record != g_geo_record_list.cend(); record++ ) {
        for ( auto line = record->m_lines.cbegin(); line != record->m_lines.cend(); line++ ) {
            for ( auto coord = line->m_coords.cbegin(); coord != line->m_coords.cend(); coord++ ) {

                next_gps = coord->geo;
                next_map = coord->map;

                if ( first_entry ) {

                    first_entry = false;

                    g_gps_min = next_gps;
                    g_gps_max = next_gps;

                    g_map_min = next_map;
                    g_map_max = next_map;

                } else {

                    g_gps_min.x = std::min ( g_gps_min.x, next_gps.x );
                    g_gps_min.y = std::min ( g_gps_min.y, next_gps.y );
                    g_gps_max.x = std::max ( g_gps_max.x, next_gps.x );
                    g_gps_max.y = std::max ( g_gps_max.y, next_gps.y );

                    g_map_min.x = std::min ( g_map_min.x, next_map.x );
                    g_map_min.y = std::min ( g_map_min.y, next_map.y );
                    g_map_max.x = std::max ( g_map_max.x, next_map.x );
                    g_map_max.y = std::max ( g_map_max.y, next_map.y );

                }
            }
        }
    }

}

static void _find_scale ( void ) {

    const int scale = 1;

    g_step_ver = scale * 320;
    g_step_hor = scale * 240;
}

static bool _is_overlapped ( const geo_rect_t& rect_scan, const geo_rect_t& rect_obj ) {

    if ( rect_scan.min.map.x > rect_obj.max.map.x ) {
        return false;
    }
    if ( rect_scan.max.map.x < rect_obj.min.map.x ) {
        return false;
    }

    if ( rect_scan.min.map.y > rect_obj.max.map.y ) {
        return false;
    }
    if ( rect_scan.max.map.y < rect_obj.min.map.y ) {
        return false;
    }

    return true;
}

static void _create_slicer_rects() {

    scan_rect_t     map_rect;
    int             x_pages = 0;
    int             y_pages = 0;

    for (int32_t y = g_map_min.y; y <= g_map_max.y; y += g_step_ver) {

        y_pages++;
        x_pages = 0;

        for (int32_t x = g_map_min.x; x <= g_map_max.x; x += g_step_hor) {

            map_rect.rect.min.map.x = x;
            map_rect.rect.min.map.y = y;
            map_rect.rect.max.map.x = x + g_step_hor;
            map_rect.rect.max.map.y = y + g_step_ver;

            map_rect.rect.min.map_to_geo();
            map_rect.rect.min.reset_angle();

            map_rect.rect.max.map_to_geo();
            map_rect.rect.max.reset_angle();

            g_slicer_rects.push_back(map_rect);

            x_pages++;
        }
    }
}

static void _attach_objects () {

    geo_rect_t obj_rect;
    bool       is_first;

    for ( size_t i = 0; i < g_geo_record_list.size(); i++ ) {

        is_first = true;

        for (auto line_it = g_geo_record_list[i].m_lines.cbegin(); line_it != g_geo_record_list[i].m_lines.cend(); line_it++) {
            for (auto coord_it = line_it->m_coords.cbegin(); coord_it != line_it->m_coords.cend(); coord_it++) {

                if ( is_first ) {
                    is_first = false;
                    obj_rect.min.map.x = obj_rect.max.map.x = coord_it->map.x;
                    obj_rect.min.map.y = obj_rect.max.map.x = coord_it->map.y;
                } else {
                    obj_rect.min.map.x = std::min ( obj_rect.min.map.x, coord_it->map.x );
                    obj_rect.max.map.x = std::max ( obj_rect.min.map.x, coord_it->map.x );
                    obj_rect.min.map.y = std::min ( obj_rect.min.map.y, coord_it->map.y );
                    obj_rect.max.map.y = std::max ( obj_rect.min.map.y, coord_it->map.y );
                }

            }
        }

        for ( auto it = g_slicer_rects.begin(); it != g_slicer_rects.end(); it++ ) {
            if ( _is_overlapped( it->rect, obj_rect ) ) {
                it->obj.push_back(i);
            }
        }
    }

    return;
}

static void _remove_unused () {

    auto it = g_slicer_rects.begin();

    while ( it != g_slicer_rects.end() ) {
        if ( it->obj.size() == 0 ) {
            it = g_slicer_rects.erase(it);
        } else {
            it++;
        }
    }
}

static void _remove_missed() {

    auto it = g_slicer_rects.begin();

    while (it != g_slicer_rects.end()) {
        if (it->res.size() == 0) {
            it = g_slicer_rects.erase(it);
        } else {
            it++;
        }
    }
}

static void _slicing ( void ) {

    _create_slicer_rects();
    _attach_objects();
    _remove_unused();

    for ( auto it = g_slicer_rects.begin(); it != g_slicer_rects.end(); it++ ) {
        _scan_rect( & (*it) );
    }

    _remove_missed();

    for (auto it = g_slicer_rects.begin(); it != g_slicer_rects.end(); it++) {
        _log_index ( *it );
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
