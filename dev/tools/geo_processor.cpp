#include <cassert>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <stack>
#include <queue>
#include <algorithm>

#include <geo_processor.h>
#include <geo_projection.h>

//---------------------------------------------------------------------------//
 
#define GEO_RGB(var,in_a,in_r,in_g,in_b)     { var.setR(in_r); var.setG(in_g); var.setB(in_b); }
#define GEO_INSIDE          (0) // 0000
#define GEO_LEFT            (1) // 0001
#define GEO_RIGHT           (2) // 0010
#define GEO_BOTTOM          (4) // 0100
#define GEO_TOP             (8) // 1000

//---------------------------------------------------------------------------//

geo_processor_t::geo_processor_t() {
    close();
}

//---------------------------------------------------------------------------//

void geo_processor_t::set_names ( const char* const idx_file_name, const char* const map_file_name ) {

    m_map_file_name = map_file_name;
    m_idx_file_name = idx_file_name;

    m_map_file.open ( m_map_file_name, std::ios_base::binary );
}

void geo_processor_t::load_idx ( void ) {

    _load_idx ( m_idx_list );
}

void geo_processor_t::process_map ( geo_coord_t center, const double scale, const double angle ) {

    bool angle_update  = false;
    bool map_update    = false;

    center.geo_to_map();
    center.reset_angle();

    if ( m_first_run ) {
        m_first_run   = false;
        angle_update  = true;
        map_update    = true;
    } else
    if (  !_is_scale_valid(scale)  ||  !_is_view_rect_valid(center)  ) {
        angle_update  =  true;
        map_update    =  true;
    } else 
    if ( !_is_map_rect_valid(center, scale) ) {
        angle_update  =  true;
        map_update    =  true;
    } else
    if ( !_is_angle_valid(angle) ) {
        angle_update  =  true;
    } else
    if ( m_markers_changed ) {
        m_markers_changed = false;
        angle_update      = true;
        map_update        = true;
    }

    if ( map_update ) {

        geo_rect_t  view_rect_ext;
        geo_rect_t  cache_rect;

        m_center     =  center;
        m_geo_scale  =  scale;

        _calc_view_rect ( center );
        _prepare_rects  ( scale, view_rect_ext, cache_rect );

        m_view_rect_ext  =  view_rect_ext;
        m_cache_rect     =  cache_rect;

        _filter_idx_by_rect ( m_cache_rect, m_idx_list, m_map_ids );
        _load_map_by_idx ( m_map_ids, m_cache_map );
    }

    if ( angle_update ) {
        map_update = true;
        _set_angle ( angle );
        _rotate_map_by_angle ( center, scale );
        _trim_rotated_map_by_rect ();
    }

    if ( map_update ) {
        _clr_screen ();
        _draw_area  ();
        _draw_building();
        _draw_roads();
        _draw_center(center);
        _draw_markers();
    }

    return;
}

void geo_processor_t::set_pix ( const map_pos_t& pos, const geo_pixel_t& px ) {

    // assert ( pos.x >= 0 );
    // assert ( pos.x < m_view_out.max.map.x );
    // assert ( pos.y >= 0 );
    // assert ( pos.y < m_view_out.max.map.y );

    if ((pos.x >= 0) && (pos.x < m_screen_rect.max.map.x)) {
        if ((pos.y >= 0) && (pos.y < m_screen_rect.max.map.y)) {

            geo_pixel_int_t   tmp;
            _px_conv(px, tmp);

            int32_t offset;

            offset = pos.y * m_screen_rect.max.map.x;
            offset += pos.x;

            m_video_buffer[offset] = tmp;

        }
    }

}

void geo_processor_t::close ( void ) {

    m_view_angle_step   = 0.5; 
    m_scale_step        = 0.01;
    m_geo_scale         = 1;
    m_x_step            = 0;
    m_y_step            = 0;    
    m_view_angle        = 0;  
    m_first_run         = true;

    m_video_buffer.clear();
    m_screen_rect.clear();
    m_view_rect.clear();
    m_view_rect_ext.clear();
    m_cache_rect.clear();
}

void geo_processor_t::get_shifts ( const double x, const double y, double& shift_x, double& shift_y ) {

    double step_x     =  0.0001;
    double dist_x     =  0;
                        
    double step_y     =  0.0001;
    double dist_y     =  0;

    dist_x = gps_distance (  x, y,  x + step_x, y  );
    dist_y = gps_distance (  x, y,  x, y + step_y  );

    shift_x = (step_x / dist_x) * m_geo_scale;
    shift_y = (step_y / dist_y) * m_geo_scale;

    return;
}

void geo_processor_t::video_alloc ( int32_t x, int32_t y ) {

    size_t  alloc_size = 0;

    alloc_size = x * y;

    m_screen_rect.min.map.x = 0;
    m_screen_rect.min.map.y = 0;
    m_screen_rect.min.reset_angle();

    m_screen_rect.max.map.x = x;
    m_screen_rect.max.map.y = y;
    m_screen_rect.max.reset_angle();

    m_video_buffer.clear();
    m_video_buffer.resize( alloc_size );

    for ( size_t i = 0; i < m_video_buffer.size(); i++ ) {
        m_video_buffer[i] = 0;
    }
}

void geo_processor_t::unpack ( uint16_t clr, uint8_t& r, uint8_t& g, uint8_t& b ) {

    uint32_t _clr = clr;

    _clr <<= 3;
    b = static_cast<uint8_t> (_clr & 0xF8);

    _clr >>= 6;
    g = static_cast<uint8_t> (_clr & 0xFC);

    _clr >>= 5;
    r = static_cast<uint8_t> (_clr & 0xF8);

    return;
}

void geo_processor_t::pack ( uint8_t r, uint8_t g, uint8_t b, uint16_t& packed_clr ) {

    packed_clr   = 0;
    packed_clr  |=  (r & 0xf8) <<  8;
    packed_clr  |=  (g & 0xfc) <<  3;
    packed_clr  |=  (b & 0xf8) >>  3;
}

void geo_processor_t::add_marker ( double lon, double lat ) {

    geo_coord_t new_coord;

    new_coord.geo.x = lon;
    new_coord.geo.y = lat;
    new_coord.geo_to_map();
    new_coord.reset_angle();

    m_markers_list.push_back(new_coord);

    m_markers_changed = true;
}

void geo_processor_t::clear_markers ( void ) {
    m_markers_list.clear();
    m_markers_changed = true;
}
 
//---------------------------------------------------------------------------//

void geo_processor_t::_draw_center ( const geo_coord_t& center ) {

    geo_pixel_t paint_clr;

    geo_coord_t left   = center;
    geo_coord_t right  = center;
    geo_coord_t top    = center;
    geo_coord_t bottom = center;

    paint_clr.setR(255);
    paint_clr.setG(0);
    paint_clr.setB(0);

    left.ang.x    -=  4;
    right.ang.x   +=  4;
    bottom.ang.y  -=  4;
    top.ang.y     +=  4;

    _line ( left, right, 1, paint_clr );
    _line ( top, bottom, 1, paint_clr );

    return;
}

void geo_processor_t::_clr_screen ( void ) {

    uint8_t r = 240;
    uint8_t g = 240;
    uint8_t b = 240;

    uint16_t packed_clr = 0;

    pack ( r, g, b, packed_clr );
    unpack ( packed_clr, r, g, b );

    for ( size_t i = 0; i < m_video_buffer.size(); i++ ) {
        m_video_buffer[i] = packed_clr;
    }
}

void geo_processor_t::_calc_view_rect ( const geo_coord_t& center ) {

    uint32_t screen_width    =  m_screen_rect.max.map.x - m_screen_rect.min.map.x;
    uint32_t screen_height   =  m_screen_rect.max.map.y - m_screen_rect.min.map.y;
    uint32_t shift_x_left    =  screen_width  /  2;
    uint32_t shift_x_right   =  screen_width - shift_x_left;
    uint32_t shift_y_bottom  =  screen_height / 10;
    uint32_t shift_y_top     =  screen_height - shift_y_bottom;

    m_view_rect.min.map.x    =  center.map.x  -  shift_x_left;
    m_view_rect.max.map.x    =  center.map.x  +  shift_x_right;
    m_view_rect.min.map.y    =  center.map.y  -  shift_y_bottom;
    m_view_rect.max.map.y    =  center.map.y  +  shift_y_top;

    m_view_rect.min.reset_angle();
    m_view_rect.max.reset_angle();

    return;                                             
}

void geo_processor_t::_prepare_rects ( const double scale, geo_rect_t& view_rect_ext, geo_rect_t& cache_rect ) {

    static const auto sqrt_scale = std::sqrt(2) / 2;
    static const uint32_t zoom_a = 2;
    static const uint32_t zoom_b = 3;

    double screen_width  = m_screen_rect.max.map.x - m_screen_rect.min.map.x;
    double screen_height = m_screen_rect.max.map.y - m_screen_rect.min.map.y;

    screen_width   /=  scale;
    screen_height  /=  scale;

    double side     =  std::max ( screen_width, screen_height );
    uint32_t radius =  static_cast<uint32_t> ( 0.5 + side * sqrt_scale );

    view_rect_ext.min.map.x = m_center.map.x - ( radius * zoom_a );
    view_rect_ext.max.map.x = m_center.map.x + ( radius * zoom_a );
    view_rect_ext.min.map.y = m_center.map.y - ( radius * zoom_a );
    view_rect_ext.max.map.y = m_center.map.y + ( radius * zoom_a );
    view_rect_ext.min.reset_angle();
    view_rect_ext.max.reset_angle();

    cache_rect.min.map.x = m_center.map.x - (radius * zoom_b);
    cache_rect.max.map.x = m_center.map.x + (radius * zoom_b);
    cache_rect.min.map.y = m_center.map.y - (radius * zoom_b);
    cache_rect.max.map.y = m_center.map.y + (radius * zoom_b);
    cache_rect.min.reset_angle();
    cache_rect.max.reset_angle();

    return;
}

bool geo_processor_t::_is_view_rect_valid ( const geo_coord_t& pos ) const {

    if ( pos.map.x <= m_view_rect_ext.min.map.x ) {
        return false;
    }
    if ( pos.map.x >= m_view_rect_ext.max.map.x ) {
        return false;
    }
    if ( pos.map.y <= m_view_rect_ext.min.map.y ) {
        return false;
    }
    if ( pos.map.y >= m_view_rect_ext.max.map.y ) {
        return false;
    }

    return true;
}

bool geo_processor_t::_is_angle_valid ( const double angle ) const {

    double delta = angle - m_view_angle;

    if ( delta < 0 ) {
        delta = -delta;
    }

    if ( m_view_angle_step < delta ) {
        return false;
    }

    return true;
}

bool geo_processor_t::_is_map_rect_valid ( const geo_coord_t& center, double scale ) const {

    double dx = std::abs ( center.map.x - m_center.map.x );
    double dy = std::abs ( center.map.y - m_center.map.y );
    double offset = (dx * dx) + (dy * dy);

    if (scale > 1) {
        scale = 1;
    }

    offset /= scale;

    if (offset > 1) {
        return false;
    }

    return true;    
}

bool geo_processor_t::_is_scale_valid ( const double scale ) const {

    double delta = scale - m_geo_scale;

    if ( delta < 0 ) {
        delta = -delta;
    }

    if ( m_scale_step < delta ) {
        return false;
    }

    return true;
}

void geo_processor_t::_load_idx ( l_geo_idx_rec_t& idx_list ) {

    geo_parser_t    parser;
    geo_idx_rec_t   geo_idx;
    geo_offset_t    file_offset = 0;
    char            ch;
    bool            eoc;
    bool            eor;

    idx_list.clear();

    m_idx_file.open ( m_idx_file_name, std::ios_base::binary );

    while ( m_idx_file.read(&ch, 1) ) {

        parser.load_param(ch, eoc, file_offset);
        file_offset++;

        if ( !eoc ) {
            continue;
        }

        parser.process_idx ( geo_idx, eor );

        if ( !eor ) {
            continue;
        }

        idx_list.push_back(geo_idx);
    }

    m_idx_file.close();
}

void geo_processor_t::_filter_idx_by_rect ( const geo_rect_t& base_rect, const l_geo_idx_rec_t& rect_list, v_geo_offset_t& out_list ) const {

    std::set<geo_offset_t> nodes_list;
    size_t  insert_cnt  = 0;
    size_t  objects_cnt = 0;

    out_list.clear();

    for ( auto it = rect_list.cbegin(); it != rect_list.cend(); it++ ) {
        if ( ! _is_overlapped(base_rect, POS_TYPE_MAP, it->m_rect) ) {
            continue;
        }
        for ( auto ref = it->m_list_off.cbegin(); ref != it->m_list_off.cend(); ref++ ) {
            insert_cnt++;
            nodes_list.insert ( *ref );
        }
    }

    objects_cnt = nodes_list.size();

    out_list.reserve ( objects_cnt );

    for ( auto it = nodes_list.cbegin(); it != nodes_list.cend(); it++ ) {
        out_list.push_back ( *it );
    }
                                            
    return;
}

void geo_processor_t::_load_map_by_idx ( const v_uint32_t& map_entries, l_geo_entry_t& map_cache ) {

    geo_entry_t             new_entry;
    std::set<geo_offset_t>  map_set;


    for (auto map_it = map_entries.cbegin(); map_it != map_entries.cend(); map_it++ ) {
        map_set.insert ( * map_it );
    }

    for ( auto cache_entry = map_cache.begin(); cache_entry != map_cache.end(); cache_entry++ ) {

        auto find_res = map_set.find(cache_entry->m_data_off );

        if ( find_res == map_set.end() ) {

            cache_entry->m_to_display = false;
            cache_entry->m_miss_cnt++;

        }  else  {

            cache_entry->m_to_display = true;
            cache_entry->m_miss_cnt = 0;

            map_set.erase( find_res );

        }

    }

    for ( auto id = map_set.cbegin(); id != map_set.cend(); id++ ) {

        new_entry.clear();
        _load_map_entry( *id, new_entry );
        new_entry.m_to_display = true;
        new_entry.m_miss_cnt = 0;
        map_cache.push_back(new_entry);

    }

    return;
}

void geo_processor_t::_rotate_map_by_angle ( const geo_coord_t& center, const double scale ) {

    for ( auto it = m_cache_map.begin(); it != m_cache_map.end(); it++ ) {

        if ( !it->m_to_display ) {
            continue;
        }

        for ( auto line_it = it->m_lines.begin(); line_it != it->m_lines.end(); line_it++ ) {
            line_it->m_fill_pos.clear();
            _rotate_geo_line ( center, scale, line_it->m_coords );
        }

    }

}

void geo_processor_t::_trim_rotated_map_by_rect ( void ) {

    geo_entry_t     out_record;

    m_paint_list.clear();
    m_view_rect.reset_angle();

    for (auto it = m_cache_map.cbegin(); it != m_cache_map.cend(); it++) {

        if ( !it->m_to_display ) {
            continue;
        }

        if ( it->m_record_type == OBJID_RECORD_AREA ) {
            _trim_record ( m_view_rect, *it, true, out_record );
        } else
        if ( it->m_record_type == OBJID_RECORD_BUILDING ) {
            _trim_record ( m_view_rect, *it, true, out_record );
        } else
        if ( it->m_record_type == OBJID_RECORD_HIGHWAY ) {
            _trim_record ( m_view_rect, *it, false, out_record );
        }

        if (out_record.m_lines.size() > 0) {
            m_paint_list.push_back(out_record);
        }
    }
}

//---------------------------------------------------------------------------//

void geo_processor_t::_draw_area ( void ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    size_t cnt;

    for ( auto it = m_paint_list.begin(); it != m_paint_list.end(); it++ ) {

        if ( it->m_record_type != OBJID_RECORD_AREA ) {
            continue;
        }

        cnt = it->m_lines.size();

        if (cnt == 1) {
            _process_area ( *it );
        } else {

            for ( size_t i = 0; i < cnt; i++ ) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_OUTER ) {
                    _process_area ( *it );
                }
            }

            for ( size_t i = 0; i < cnt; i++ ) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_INNER ) {
                    _process_area ( *it );
                }
            }

        }
    }

    return;
}

void geo_processor_t::_draw_building ( void ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    size_t cnt;

    for ( auto it = m_paint_list.begin(); it != m_paint_list.end(); it++) {

        if (it->m_record_type != OBJID_RECORD_BUILDING) {
            continue;
        }

        cnt = it->m_lines.size();

        if (cnt == 1) {
            _process_area ( *it );
        } else {

            for (size_t i = 0; i < cnt; i++) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_OUTER ) {
                    _process_area( *it );
                }
            }

            for (size_t i = 0; i < cnt; i++) {
                if ( it->m_lines[i].m_role != OBJID_ROLE_OUTER ) {
                    _process_area ( *it );
                }
            }

        }

    }
}

void geo_processor_t::_draw_roads ( void ) {

    geo_pixel_t clr1 (255, 0, 0);
    geo_pixel_t clr2 (255, 0, 0);
    int         road_width_m;

    for ( auto it = m_paint_list.begin(); it != m_paint_list.end(); it++ ) {;

        if ( it->m_record_type != OBJID_RECORD_HIGHWAY ) {
            continue;
        }


        // "motorway",      DRAW_PATH_MOTORWAY               Автомагистрали 
        // "trunk",         DRAW_PATH_TRUNK                  Важные дороги
        // "primary",       DRAW_PATH_PRIMARY                Дороги регионального значения
        // "secondary",     DRAW_PATH_SECONDARY              Дороги областного значения
        // "tertiary",      DRAW_PATH_TERTIARY               Автомобильных дорог местного значения
        // "unclassified",  DRAW_PATH_UNCLIASSIFIED          Автомобильные дороги местного значения
        // "residential",   DRAW_PATH_RESIDENTIAL            Дороги внутри жилых зон.
        // "living_street", DRAW_PATH_STREET                 Улицы такого же класса как residential.
        // "service",       DRAW_PATH_SERVICE                Служебные проезды
        // "road",          DRAW_PATH_ROAD                   Возможно дороги.
        // "pedestrian",    DRAW_PATH_PEDISTRAN              Улицы, выделенных для пешеходов.
        // "track",         DRAW_PATH_TRACK                  Дороги сельскохозяйственного назначения
        // "footway",       DRAW_PATH_FOOTWAY                Пешеходные дорожки, тротуары
        // "steps",         DRAW_PATH_STEPS                  Лестницы, лестничные пролёты.
        // "path",          DRAW_PATH_PATH                   стихийная тропа
        // "bus_guideway",  DRAW_SKIP                        Дороги предназначенные только для автобусов.
        // "escape",        DRAW_SKIP                        Аварийная полоса
        // "raceway",       DRAW_SKIP                        Гоночная трасса
        // "bridleway",     DRAW_SKIP                        Дорожки для верховой езды.
        // "via_ferrata",   DRAW_SKIP                        горные тропы
        // "cycleway",      DRAW_SKIP                        велодорожки
        // "services",      DRAW_SKIP                        СТО, Кафе, ...

        switch ( it->m_default_type ) {

            case OBJID_TYPE_STREET:         // асфальтированные жилый улицы. оставляем.
            case OBJID_TYPE_SERVICE:        // асфальтовая дорога. сотавляем.           
            case OBJID_TYPE_RESIDENTIAL:    // асфальтовая дорога. сотавляем.     
                _map_color ( it->m_default_type, clr1, clr2 );
                road_width_m = 5;
                break;

            // асфальтовая дорожка. оставляем.
            case OBJID_TYPE_TRACK:              
                _map_color ( it->m_default_type, clr1, clr2 );
                road_width_m = 2;
                break;

            case OBJID_TYPE_PATH:           // тропинки. пропускаем
            case OBJID_TYPE_FOOTWAY:        // просто возможность пройти. пропускаем.
            case OBJID_TYPE_ROAD:           // таких нет. пропускаем.
            case OBJID_TYPE_STEPS:          // таких нет. пропускаем.
            default:                        // 
                continue;

        }

        for ( auto road_it = it->m_lines.cbegin(); road_it != it->m_lines.cend(); road_it++ ) {
            _poly_line ( road_it->m_coords, road_width_m, clr2 );
        }

    }
}

//---------------------------------------------------------------------------//

void geo_processor_t::_map_color ( const obj_type_t& obj_type, geo_pixel_t& border_color, geo_pixel_t& fill_color ) const {

    switch ( obj_type ) {

     //----------------------------------------------//
     //                          A    R    G    B    //
     //----------------------------------------------//
        case OBJID_TYPE_FOREST:       
            GEO_RGB ( fill_color,   255, 173, 209, 158 )
            GEO_RGB ( border_color, 255, 163, 199, 148 )
            break;

        case OBJID_TYPE_GRASS:        
            GEO_RGB ( fill_color,   255, 205, 235, 176 )
            GEO_RGB ( border_color, 255, 185, 215, 156 )
            break;

        case OBJID_TYPE_ASPHALT:      
            GEO_RGB ( fill_color,   255, 174, 174, 179 )
            GEO_RGB ( border_color, 255, 201, 201, 202 )
            break;

        case OBJID_TYPE_BUILDING:     
            GEO_RGB ( fill_color,   255, 217, 208, 201 )
            GEO_RGB ( border_color, 255, 195, 154, 100 )
            break;

        case OBJID_TYPE_WATER:        
            GEO_RGB ( fill_color,   255, 100, 100, 200 )
            GEO_RGB ( border_color, 255,   0,   0,   0 )   
            break;

        case OBJID_TYPE_GENERAL:      
            GEO_RGB ( fill_color,   255,  80, 120,  80 )
            GEO_RGB ( border_color, 255, 150, 150, 250 )
            break;

        case OBJID_TYPE_MOUNTAIN:     
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_STONE:        
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_SAND:         
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_UNDEFINED:    
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_FOOTWAY:      
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_ROAD:         
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_SECONDARY:    
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_TRUNK:        
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_MOTORWAY:     
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_PRIMARY:      
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_TERTIARY:     
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_RAILWAY:      
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_RIVER:        
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_BRIDGE:       
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_TYPE_TUNNEL:       
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_RECORD_AREA:       
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_RECORD_BUILDING:   
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;

        case OBJID_RECORD_HIGHWAY:    
        case OBJID_TYPE_STREET:
        case OBJID_TYPE_SERVICE:
        case OBJID_TYPE_RESIDENTIAL:
            GEO_RGB ( fill_color,   255, 174, 174, 179 )
            GEO_RGB ( border_color, 255, 221, 221, 232 )
            break;

        case OBJID_TYPE_TRACK:
            break;

        default:
            GEO_RGB ( fill_color, 255, 255, 255, 255 )
            GEO_RGB ( border_color, 0,   0,   0,   0 )
            break;
    }

}

void geo_processor_t::_set_angle ( const double angle ) {

    constexpr double pi_rad_scale = 3.14159265358979323846 / 180;
        
    m_geo_angle     = angle;
    m_geo_angle_sin = sin ( angle * pi_rad_scale );
    m_geo_angle_cos = cos ( angle * pi_rad_scale );

    m_view_angle = angle;
}

void geo_processor_t::_px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const {

    uint16_t val = 0;

    val |= (from.getR() >> 3);

    val <<= 6;
    val |= (from.getG() >> 2);

    val <<= 5;
    val |= (from.getB() >> 3);

    to = val;
}

void geo_processor_t::_px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const {

    uint8_t tmp = 0;

    tmp = static_cast<uint8_t> (from >> 8);
    tmp &= 0xF8;
    to.setR(tmp);

    tmp = static_cast<uint8_t> (from >> 3);
    tmp &= 0xFC;
    to.setG(tmp);

    tmp = static_cast<uint8_t> (from << 3);
    tmp &= 0xF8;
    to.setB(tmp);
}

bool geo_processor_t::_is_overlapped ( const geo_rect_t& window, const pos_type_t pos_type, const geo_rect_t& slice ) const {
    return window.is_overlapped ( pos_type, slice );
}

void geo_processor_t::_load_map_entry ( const uint32_t map_entry_offset, geo_entry_t& map_entry ) {

    geo_parser_t parser;

    char    ch = 0;
    bool    eoc = false;
    bool    eor = false;

    m_map_file.seekg ( map_entry_offset, std::fstream::beg );

    while (m_map_file.read(&ch, 1)) {

        parser.load_param(ch, eoc, 0);
        if (!eoc) {
            continue;
        }

        parser.process_map(map_entry, eor);
        if (!eor) {
            continue;
        }

        break;
    }

    map_entry.m_data_off = map_entry_offset;
}

void geo_processor_t::_rotate_geo_line ( const geo_coord_t& center, const double scale, v_geo_coord_t& coords ) const {

    for ( auto it_coord = coords.begin(); it_coord != coords.end(); it_coord++ ) {
        _rotate_coord ( center, scale, *it_coord );
    }
}

void geo_processor_t::_rotate_coord (const geo_coord_t& center, const double scale, geo_coord_t& coord ) const {

    double x = coord.map.x - center.map.x;
    double y = coord.map.y - center.map.y;

    double next_x = (x * m_geo_angle_cos)  -  (y * m_geo_angle_sin);
    double next_y = (x * m_geo_angle_sin)  +  (y * m_geo_angle_cos);

    next_x *= scale;
    next_y *= scale;

    next_x += center.map.x;
    next_y += center.map.y;

    coord.ang.x = static_cast<int32_t> ( next_x + 0.5 );
    coord.ang.y = static_cast<int32_t> ( next_y + 0.5 );
}

void geo_processor_t::_get_vcode ( const geo_rect_t& rect_path, const geo_coord_t& p2, uint32_t& res ) const {

    uint32_t c = GEO_INSIDE;

    if (p2.ang.x < rect_path.min.ang.x) {
        c |= GEO_LEFT;
    }
    if (p2.ang.x > rect_path.max.ang.x) {
        c |= GEO_RIGHT;
    }
    if (p2.ang.y < rect_path.min.ang.y) {
        c |= GEO_BOTTOM;
    }
    if (p2.ang.y > rect_path.max.ang.y) {
        c |= GEO_TOP;
    }

    res = c;
}

bool geo_processor_t::_cohen_sutherland ( const geo_rect_t& r, geo_coord_t& a, geo_coord_t& b ) const {

    uint32_t code_a, code_b, code;
    geo_coord_t* c;

    _get_vcode ( r, a, code_a );
    _get_vcode ( r, b, code_b );

    while (code_a | code_b) {

        if ( code_a & code_b ) {
            // both points are at the same side. Exit with "no intersection".
            return false;
        }

        if (code_a) {
            code = code_a;
            c = &a;
        } else {
            code = code_b;
            c = &b;
        }

        if ( code & GEO_LEFT ) {
            c->ang.y += (a.ang.y - b.ang.y) * (r.min.ang.x - c->ang.x) / (a.ang.x - b.ang.x);
            c->ang.x  = r.min.ang.x;
        } else 
        if ( code & GEO_RIGHT ) {
            c->ang.y += (a.ang.y - b.ang.y) * (r.max.ang.x - c->ang.x) / (a.ang.x - b.ang.x);
            c->ang.x  = r.max.ang.x;
        } else 
        if ( code & GEO_BOTTOM ) {
            c->ang.x += (a.ang.x - b.ang.x) * (r.min.ang.y - c->ang.y) / (a.ang.y - b.ang.y);
            c->ang.y  = r.min.ang.y;
        } else 
        if ( code & GEO_TOP ) {
            c->ang.x += (a.ang.x - b.ang.x) * (r.max.ang.y - c->ang.y) / (a.ang.y - b.ang.y);
            c->ang.y  = r.max.ang.y;
        }

        if (code == code_a) {
            _get_vcode( r, a, code_a );
        } else {
            _get_vcode( r, b, code_b );
        }
    }

    return true;
}

void geo_processor_t::_trim_record_area ( const geo_rect_t& rect_path, const geo_entry_t& in_path, geo_entry_t& out_path ) const {

    v_geo_line_t res;

    for (auto it = in_path.m_lines.cbegin(); it != in_path.m_lines.cend(); it++ ) {
        geo_intersect ( POS_TYPE_ANGLE, true, *it, rect_path, res );
        for (size_t i = 0; i < res.size(); i++) {
            out_path.m_lines.push_back(std::move(res[i]));
        }
    }

    return;
}

void geo_processor_t::_trim_record_path ( const geo_rect_t& rect_path, const geo_entry_t& in_path, geo_entry_t& out_path ) const {

    geo_line_t      next_line;
    uint32_t        curr_pos;
    uint32_t        next_pos;
    bool            find_res;

    for ( auto line_it = in_path.m_lines.cbegin(); line_it != in_path.m_lines.cend(); line_it++ ) {

        if ( line_it->m_coords.size() < 2) {
            continue;
        }

        next_line.m_role = line_it->m_role;
        next_line.m_type = line_it->m_type;
        next_line.m_area = 0;

        next_line.clear();

        _get_vcode ( rect_path, line_it->m_coords[0], curr_pos );

        if ( curr_pos == GEO_INSIDE ) {
            next_line.m_coords.push_back ( line_it->m_coords[0] );
        }

        for ( size_t i = 1; i < line_it->m_coords.size(); i++ ) {

            _get_vcode ( rect_path, line_it->m_coords[i], next_pos );

            if ( (curr_pos == GEO_INSIDE)  &&  (next_pos == GEO_INSIDE) ) {
                next_line.m_coords.push_back( line_it->m_coords[i] );
            } else
            if ( (curr_pos == GEO_INSIDE)  &&  (next_pos != GEO_INSIDE) ) {
                geo_coord_t a = line_it->m_coords[i - 1];
                geo_coord_t b = line_it->m_coords[i - 0];
                find_res = _cohen_sutherland ( rect_path, a, b );
                assert ( find_res == true );
                next_line.m_coords.push_back(b);
                out_path.m_lines.push_back ( std::move(next_line) );
                next_line.clear();
            } else
            if ( (curr_pos != GEO_INSIDE)  &&  (next_pos == GEO_INSIDE) ) {
                assert ( next_line.m_coords.size() == 0 );
                geo_coord_t a = line_it->m_coords[i - 1];
                geo_coord_t b = line_it->m_coords[i - 0];
                find_res = _cohen_sutherland ( rect_path, a, b );
                assert ( find_res == true );
                next_line.m_coords.push_back(a);
                next_line.m_coords.push_back(b);
            } else {
                assert ( next_line.m_coords.size() == 0 );
                geo_coord_t a = line_it->m_coords[i - 1];
                geo_coord_t b = line_it->m_coords[i - 0];
                find_res = _cohen_sutherland(rect_path, a, b);
                if ( find_res ) {
                    next_line.m_coords.push_back(a);
                    next_line.m_coords.push_back(b);
                    out_path.m_lines.push_back ( std::move(next_line) );
                    next_line.clear();
                }
            }

            curr_pos = next_pos;
        }

        if ( next_line.m_coords.size() > 0 ) {
            out_path.m_lines.push_back(std::move(next_line));
        }

    }

    return;
}

void geo_processor_t::_trim_record ( const geo_rect_t& rect_path, const geo_entry_t& in_path, const bool is_area, geo_entry_t& out_path ) const {

    out_path.m_record_type   =  in_path.m_record_type;
    out_path.m_default_type  =  in_path.m_default_type;
    out_path.m_osm_ref       =  in_path.m_osm_ref;
    out_path.m_data_off      =  in_path.m_data_off;
    out_path.m_to_display    =  in_path.m_to_display;
    out_path.m_miss_cnt      =  in_path.m_miss_cnt;

    out_path.m_lines.clear();

    if ( is_area ) {
        _trim_record_area ( rect_path, in_path, out_path );
    } else {
        _trim_record_path ( rect_path, in_path, out_path );
    }

    return;
}

void geo_processor_t::_process_area ( geo_entry_t& geo_line ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    _map_color ( geo_line.m_default_type, border_color, fill_color );

    for ( size_t i = 0; i < geo_line.m_lines.size(); i++ ) {
        _fill_poly ( geo_line.m_lines[i], border_color, fill_color );
    }
}

void geo_processor_t::_poly_line ( const v_geo_coord_t& poly_line, const int width, const geo_pixel_t color ) {

    if (poly_line.size() >= 2) {
        for (size_t i = 1; i < poly_line.size(); i++) {
            _line ( poly_line[i - 1], poly_line[i], width, color );
        }
    }
}

void geo_processor_t::_fill_poly ( geo_line_t& poly_line, const geo_pixel_t border_clr, const geo_pixel_t fill_clr ) {

    bool        are_collinear = true;
    map_pos_t   scan_min;
    map_pos_t   scan_max;
    geo_coord_t pt1;
    geo_coord_t pt2;
    size_t      coords_cnt = poly_line.m_coords.size() - 1;

    if ( coords_cnt < 2 ) {
        return;
    }

    if ( coords_cnt == 2 ) {
        _line ( pt1, pt2, 1, border_clr );
        return;
    }

    for ( size_t i = 2; i <= coords_cnt; i++ ) {
        if ( ! _are_collinear ( poly_line.m_coords[i-2], poly_line.m_coords[i-1], poly_line.m_coords[i-0] ) ) {
            are_collinear = false;
            break;
        }
    }

    if ( are_collinear ) {
        _line ( pt1, pt2, 1, border_clr );
        return;
    }

    scan_min.x  =  scan_max.x  =  poly_line.m_coords[0].ang.x;
    scan_min.y  =  scan_max.y  =  poly_line.m_coords[0].ang.y;

    for ( size_t i = 1; i <= coords_cnt; i++ ) {
        scan_min.x  =  std::min(scan_min.x, poly_line.m_coords[i].ang.x);
        scan_max.x  =  std::max(scan_max.x, poly_line.m_coords[i].ang.x);
        scan_min.y  =  std::min(scan_min.y, poly_line.m_coords[i].ang.y);
        scan_max.y  =  std::max(scan_max.y, poly_line.m_coords[i].ang.y);
    }

    v_segment_t segments_list;
    segment_t   s1;
    segment_t   s2;

    for ( size_t i = 1; i <= coords_cnt; i++ ) {

        if ( poly_line.m_coords[i-1].ang.y == poly_line.m_coords[i-0].ang.y ) {
            continue;
        }

        if ( ! s1.valid ) {
            s1.set_coord ( poly_line.m_coords[i-1].ang, poly_line.m_coords[i-0].ang );
            segments_list.push_back(s1);
            continue;
        }

        s2.set_coord ( poly_line.m_coords[i-1].ang, poly_line.m_coords[i-0].ang );
        s2.dir_prev = s1.dir_my;
        segments_list.back().dir_next = s2.dir_my;

        segments_list.push_back(s2);

        s1 = s2;
    }

    if ( segments_list.size() < 2 ) {
        return;
    }

    segments_list.back().dir_next  = segments_list.front().dir_my;
    segments_list.front().dir_prev = segments_list.back().dir_my;

    map_pos_t  intersection;
    segment_t  _hor;

    for ( int y = scan_min.y; y <= scan_max.y; y++ ) {

        _hor.org.p1.x = scan_min.x;
        _hor.org.p2.x = scan_max.x;
        _hor.org.p1.y = y;
        _hor.org.p2.y = y;

        _clear_intersection ();

        for (size_t id = 0; id < segments_list.size(); id++) {

            if (segments_list[id].srt.p1.y > y) {
                continue;
            }
            if (segments_list[id].srt.p2.y < y) {
                continue;
            }

            if (segments_list[id].org.p1.x == segments_list[id].org.p2.x) {
                intersection.y = y;
                intersection.x = segments_list[id].org.p1.x;
            } else {
                _intersection ( _hor, segments_list[id], intersection );
            }

            intersection_type_t intersection_type = INTERSECTION_TYPE_DOT;

            if ( ( segments_list[id].org.p1 == intersection ) || (intersection == segments_list[id].org.p2) ) {
                if (segments_list[id].dir_my == DIR_UP ) {
                    intersection_type = INTERSECTION_TYPE_UP;
                } else {
                    intersection_type = INTERSECTION_TYPE_DOWN;
                }
            }

            _add_intersection ( scan_min, intersection, intersection_type );

        }

        _commit_intersection ( scan_min, y, fill_clr );
    }

    for ( size_t i = 1; i < poly_line.m_coords.size(); i++ ) {
        _line( poly_line.m_coords[i-1], poly_line.m_coords[i-0], 1, border_clr );
    }

    return;
}

void geo_processor_t::_clear_intersection ( void ) {

    m_intersection_map.clear();
}

void geo_processor_t::_add_intersection ( const map_pos_t& base, const map_pos_t& pos, intersection_type_t type ) {

    size_t x_idx = pos.x - base.x;

    if ( m_intersection_map.size() <= x_idx) {
        m_intersection_map.resize (x_idx + 1 );
    }

    switch (type) {
        case INTERSECTION_TYPE_DOT:
            m_intersection_map[x_idx].cnt_pt++;
            break;
        case INTERSECTION_TYPE_UP:
            m_intersection_map[x_idx].cnt_up++;
            break;
        case INTERSECTION_TYPE_DOWN:
            m_intersection_map[x_idx].cnt_down++;
            break;
    }
}

void geo_processor_t::_intersection ( const segment_t& s1, const segment_t& s2, map_pos_t& result ) {

    assert(s1.org.p1.y == s1.org.p1.y);
    assert(s2.org.p1.y != s2.org.p2.y);

    double dX = s2.org.p1.x - s2.org.p2.x;
    double dY = s2.org.p1.y - s2.org.p2.y;
    double ss1 = dX / dY;
    double ss2 = s1.org.p1.y - s2.org.p1.y;
    double rX = s2.org.p1.x + (ss1 * ss2);

    result.y = s1.org.p1.y;
    result.x = (int)(rX + 0.5);
}

void geo_processor_t::_commit_intersection ( const map_pos_t& base, size_t y, const geo_pixel_t& clr ) {

    int  points_cnt   = 0;
    bool pending_up   = false;
    bool pending_down = false;
    bool mark_pixel   = false;

    map_pos_t px_pos;

    for ( size_t x = 0; x < m_intersection_map.size(); x++ ) {

        mark_pixel = false;

        if ( m_intersection_map[x].is_active()) {
            mark_pixel = true;
        }

        points_cnt += m_intersection_map[x].cnt_pt;

        {   // Process cross-points.
            while ( (m_intersection_map[x].cnt_up > 0) && (m_intersection_map[x].cnt_down > 0)) {
                m_intersection_map[x].cnt_up--;
                m_intersection_map[x].cnt_down--;
                points_cnt += 2;
            }
        }

        {   // Process UP/DOWN streams.
            while (m_intersection_map[x].cnt_up >= 2) {
                m_intersection_map[x].cnt_up -= 2;
                points_cnt ++;
            }
            while (m_intersection_map[x].cnt_down >= 2) {
                m_intersection_map[x].cnt_down -= 2;
                points_cnt ++;
            }
        }

        {   // Process tails...
            if (m_intersection_map[x].cnt_up > 0) {
                if (pending_up) {
                    pending_up = false;
                    points_cnt++;
                } else 
                if (pending_down) {
                    pending_down = false;
                    points_cnt  += 2;
                } else {
                    pending_up = true;
                }
            }

            if (m_intersection_map[x].cnt_down > 0) {
                if (pending_down) {
                    pending_down = false;
                    points_cnt++;
                } else 
                if (pending_up) {
                    pending_up  = false;
                    points_cnt += 2;
                } else {
                    pending_down = true;
                }
            }
        }

        px_pos.x = (int32_t)x + base.x - m_view_rect.min.ang.x;
        px_pos.y = (int32_t)y - m_view_rect.min.ang.y;

        if (points_cnt % 2) {
            set_pix ( px_pos, clr );
        } else 
        if ( pending_up || pending_down || mark_pixel ) {
            set_pix ( px_pos, clr );
        }

    }

    assert ( pending_up   == false );
    assert ( pending_down == false );
    assert ( (points_cnt % 2 ) == 0 );

    return;
}

bool geo_processor_t::_are_collinear ( const geo_coord_t& p1, const geo_coord_t& p2, const geo_coord_t& p3 ) const {

    constexpr double min_delta = 0.0001;

    if (p1.ang.x == p2.ang.x) {
        return (p2.ang.x == p3.ang.x);
    }

    if (p1.ang.y == p2.ang.y) {
        return (p2.ang.y == p3.ang.y);
    }

    double vABx = p2.ang.x - p1.ang.x;
    double vCDx = p3.ang.x - p2.ang.x;
    double vABy = p2.ang.y - p1.ang.y;
    double vCDy = p3.ang.y - p2.ang.y;

    double  vABCDx = (vABx / vCDx);
    double  vABCDy = (vABy / vCDy);
    double  my_delta = std::abs(vABCDx - vABCDy);

    return (my_delta < min_delta);
}

void geo_processor_t::_line ( const geo_coord_t& from, const geo_coord_t& to, int width, const geo_pixel_t _color ) {

    int x1 = from.ang.x  - m_view_rect.min.ang.x;
    int y1 = from.ang.y  - m_view_rect.min.ang.y;
    int x2 = to.ang.x    - m_view_rect.min.ang.x;
    int y2 = to.ang.y    - m_view_rect.min.ang.y;

    geo_pixel_t color = _color;

    const int deltaX = std::abs(x2 - x1);
    const int deltaY = std::abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;

    int error = deltaX - deltaY;

    (void)width;

    map_pos_t draw_pos;

    draw_pos.x = x2;
    draw_pos.y = y2;
    set_pix(draw_pos, color);

    while (x1 != x2 || y1 != y2) {

        draw_pos.x = x1;
        draw_pos.y = y1;
        set_pix(draw_pos, color);

        int error2 = error * 2;
        if (error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }

        if (error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }

    return;
}

void geo_processor_t::_draw_markers ( void ) {

    geo_coord_t     from;
    geo_coord_t     to;
    geo_pixel_t     clr;
    int64_t         dx1;
    int64_t         dy1;
    int64_t         dx2;
    int64_t         dy2;
    double          min_dist;
    double          cur_dist;
    bool            first_calc;
    bool            update_pos;

    l_geo_entry_t::const_iterator  best_object_it;
    v_geo_line_t::const_iterator   best_line_it;
    v_geo_coord_t::const_iterator  best_coord_it;

    m_markers_changed = false;

    clr.setR(255);
    clr.setG(0);
    clr.setB(0);

    for (auto marker_it = m_markers_list.cbegin(); marker_it != m_markers_list.cend(); marker_it++) {

        first_calc = true;
        min_dist   = 0;

        for (auto object_it = m_paint_list.cbegin(); object_it != m_paint_list.cend(); object_it++ ) {
            for (auto line_it = object_it->m_lines.cbegin(); line_it != object_it->m_lines.cend(); line_it++) {
                for (auto coord_it = line_it->m_coords.cbegin(); coord_it != line_it->m_coords.cend(); coord_it++) {
                    
                    dx1  =  marker_it->ang.x  -  coord_it->ang.x;
                    dy1  =  marker_it->ang.y  -  coord_it->ang.y;

                    dx2  =  dx1 * dx1;
                    dy2  =  dy1 * dy1;

                    cur_dist = std::sqrt(  dx2  +  dy2  );

                    update_pos = false;

                    if ( first_calc ) {
                        first_calc = false;
                        update_pos = true;
                    } else {
                        if ( min_dist > cur_dist ) {
                            update_pos = true;
                        }
                    }

                    if ( update_pos ) {

                        min_dist = cur_dist;

                        best_object_it  = object_it;
                        best_line_it    = line_it;
                        best_coord_it   = coord_it;

                    }

                }
            }
        }

        from.ang.x = best_coord_it->ang.x - 5;
        from.ang.y = best_coord_it->ang.y;
        to.ang.x   = best_coord_it->ang.x + 5;
        to.ang.y   = best_coord_it->ang.y;
        _line ( from, to, 1, clr );
        
        from.ang.x = best_coord_it->ang.x;
        from.ang.y = best_coord_it->ang.y - 5;
        to.ang.x   = best_coord_it->ang.x;
        to.ang.y   = best_coord_it->ang.y + 5;
        _line ( from, to, 1, clr );


    }

    return;
}

//---------------------------------------------------------------------------//
