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
    geo_rect_t          rect;
    l_list_t            res;
}   scan_rect_t;

typedef std::vector<scan_rect_t>    v_scan_rect_t;
typedef std::vector<v_scan_rect_t>  vv_scan_rect_t;
typedef std::list<scan_rect_t>      l_scan_rect_t;


geo_parser_t    g_geo_parser;
vv_scan_rect_t  g_slicer_rects;
 
v_geo_entry_t   g_obj_list;
v_geo_rect_t    g_obj_rect_list;
int32_t         g_step_ver      =   0;
int32_t         g_step_hor      =   0;
geo_offset_t    g_file_offset   =   0;
geo_processor_t g_geo_processor;
geo_pos_t       g_gps_min;
geo_pos_t       g_gps_max;
map_pos_t       g_map_min;
map_pos_t       g_map_max;

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

        auto record = &g_obj_list[obj_id];

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

static void _attach_objects ( void ) {

    int32_t     x_size;
    int32_t     y_size;
    int32_t     x_pos;
    int32_t     y_pos;
    int32_t     x_idx_min;
    int32_t     x_idx_max;
    int32_t     y_idx_min;
    int32_t     y_idx_max;
    bool        overlap;
    uint32_t    added_cnt;
    geo_rect_t  obj_rect;
    uint32_t    delta;

    static int  stop_cnt = 0;

    std::cerr << "Preprocessing... \r";

    added_cnt = 0;

    for ( size_t i=0; i< g_obj_rect_list.size(); i++ ) {

        obj_rect = g_obj_rect_list[i];

        x_pos  = obj_rect.min.map.x - g_map_min.x;
        y_pos  = obj_rect.min.map.y - g_map_min.y;
        x_size = obj_rect.max.map.x - obj_rect.min.map.x;
        y_size = obj_rect.max.map.y - obj_rect.min.map.y;

        delta      =  ( obj_rect.min.map.x - g_map_min.x );
        x_idx_min  =  delta / g_step_hor;
        if ( (delta % g_step_hor) == 0) {
            if (x_idx_min > 0) {
                x_idx_min--;
            }
        }

        delta      =  obj_rect.max.map.x - g_map_min.x;
        x_idx_max  =  delta / g_step_hor;


        delta      =  obj_rect.min.map.y - g_map_min.y;
        y_idx_min  =  delta / g_step_ver;
        if ((delta % g_step_ver) == 0) {
            if (y_idx_min > 0) {
                y_idx_min--;
            }
        }

        delta      =  obj_rect.max.map.y - g_map_min.y;
        y_idx_max  =  delta / g_step_ver;

        int32_t x;
        int32_t y;

        y = y_idx_min - 1;
        if ( y >= 0 ) {
            for ( x = x_idx_min; x <= x_idx_max; x++ ) {
                overlap = _is_overlapped(g_slicer_rects[y][x].rect, obj_rect);
                assert(overlap == false);
            }
        }

        for ( y = y_idx_min; y <= y_idx_max; y++ ) {

            x = x_idx_min - 1;
            if ( x  >=  0 ) {
                overlap = _is_overlapped(g_slicer_rects[y][x].rect, obj_rect);
                assert (overlap == false);
            }

            for ( x = x_idx_min; x <= x_idx_max; x++ ) {

                overlap = _is_overlapped ( g_slicer_rects[y][x].rect, obj_rect);
                assert(overlap);

                added_cnt++;
                g_slicer_rects[y][x].obj.push_back(i);
            }

            x++;
            if ( x  <  g_slicer_rects[y].size() ) {
                overlap = _is_overlapped(g_slicer_rects[y][x].rect, obj_rect);
                assert(overlap == false);
            }
        }

        if ( y < g_slicer_rects.size() ) {
            for (x = x_idx_min; x <= x_idx_max; x++) {
                overlap = _is_overlapped(g_slicer_rects[y][x].rect, obj_rect);
                assert(overlap == false);
            }
        }

    }

    std::cerr << "Preprocessing: Done.    \n";
    return;
}

static void _create_rects ( void ) {

    bool       is_first;
    geo_rect_t obj_rect;

    size_t obj_cnt = g_obj_list.size();

    std::cerr << "Obj Rects... \r";

    g_obj_rect_list.resize(obj_cnt);

    for ( size_t i = 0; i < g_obj_list.size(); i++ ) {

        is_first = true;

        for (auto line_it = g_obj_list[i].m_lines.cbegin(); line_it != g_obj_list[i].m_lines.cend(); line_it++) {
            for (auto coord_it = line_it->m_coords.cbegin(); coord_it != line_it->m_coords.cend(); coord_it++) {
                if (is_first) {
                    is_first = false;
                    obj_rect.min.map.x = obj_rect.max.map.x = coord_it->map.x;
                    obj_rect.min.map.y = obj_rect.max.map.y = coord_it->map.y;
                } else {
                    obj_rect.min.map.x = std::min(obj_rect.min.map.x, coord_it->map.x);
                    obj_rect.max.map.x = std::max(obj_rect.max.map.x, coord_it->map.x);
                    obj_rect.min.map.y = std::min(obj_rect.min.map.y, coord_it->map.y);
                    obj_rect.max.map.y = std::max(obj_rect.max.map.y, coord_it->map.y);
                }

            }
        }

        obj_rect.reset_angle();

        g_obj_rect_list[i] = obj_rect;
    }

    std::cerr << "Obj Rects: Done.   \n";

}

static void _slicing_rects( void ) {

    size_t y_cnt;
    size_t x_cnt;
    size_t cnt;

    y_cnt = g_slicer_rects.size();
    x_cnt = g_slicer_rects[0].size();

    cnt = 0;

    for (size_t y = 0; y < y_cnt; y++) {
        for (size_t x = 0; x < x_cnt; x++) {

            cnt++;

            if (g_slicer_rects[y][x].obj.size() == 0) {
                continue;
            }

            if ( (cnt % 100) == 0 ) {
                std::cerr << "Geometry: " << (x_cnt*y_cnt) - cnt << "   \r";
            }

            _scan_rect ( &g_slicer_rects[y][x] );

        }
    }

    std::cerr << "Geometry: Done     \n";
}

static void _logging_res ( void ) {

    size_t y_cnt;
    size_t x_cnt;

    y_cnt = g_slicer_rects.size();
    x_cnt = g_slicer_rects[0].size();

    for (size_t y = 0; y < y_cnt; y++) {
        for (size_t x = 0; x < x_cnt; x++) {

            if (g_slicer_rects[y][x].res.size() == 0) {
                continue;
            }

            _log_index ( g_slicer_rects[y][x] );

        }
    }

    return;
}

static void _load_file ( const char* const file_name ) {

    file_mapper_t   file;
    uint64_t        file_size;
    geo_entry_t     geo_record;

    char            ch = 0;
    bool            eor = false;
    bool            eoc = false;

    std::cerr << "Loading... \r";

    file.Init( file_name );
    file_size = file.GetSize();

    for (g_file_offset = 0; g_file_offset < file_size; g_file_offset++) {

        ch = file[g_file_offset];

        g_geo_parser.load_param(ch, eoc, g_file_offset);

        if (eoc) {
            g_geo_parser.process_map(geo_record, eor);
            if (eor) {
                g_obj_list.push_back(geo_record);
            }
        }

    }

    std::cerr << "Loading: Done.       \n";
}

static void _find_box ( void ) {

    geo_pos_t  next_gps;
    map_pos_t  next_map;

    bool first_entry = true;

    for ( auto record = g_obj_list.cbegin(); record != g_obj_list.cend(); record++ ) {
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

    g_step_ver = scale * 500;
    g_step_hor = scale * 500;
}

static void _create_slicer_rects( void ) {

    scan_rect_t     map_rect;
    v_scan_rect_t   rect_line;
    size_t          y_idx;

    std::cerr << "Map Rects... \r";

    for (int32_t y = g_map_min.y; y <= g_map_max.y; y += g_step_ver) {

        rect_line.clear();
        g_slicer_rects.push_back(rect_line);

        y_idx = g_slicer_rects.size() - 1;

        for (int32_t x = g_map_min.x; x <= g_map_max.x; x += g_step_hor) {

            map_rect.rect.min.map.x = x;
            map_rect.rect.min.map.y = y;
            map_rect.rect.max.map.x = x + g_step_hor;
            map_rect.rect.max.map.y = y + g_step_ver;

            map_rect.rect.min.map_to_geo();
            map_rect.rect.min.reset_angle();

            map_rect.rect.max.map_to_geo();
            map_rect.rect.max.reset_angle();

            g_slicer_rects[y_idx].push_back(map_rect);
                
        }

    }

    std::cerr << "Maps Rects: " << g_slicer_rects.size() << "       \n";

}

int main ( int argc, char* argv[] ) {

    if ( argc != 2 ) {
        return (-1);
    }

    _load_file( argv[1] );       //
    _find_box();                 // 
    _find_scale();               // 
    _create_slicer_rects();      // 
    _create_rects();             // 
    _attach_objects();           // 
    _slicing_rects();            // 
    _logging_res();              // 

    return 0;
}
