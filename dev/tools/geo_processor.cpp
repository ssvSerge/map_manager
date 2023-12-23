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

#define LINE_OVERLAP_NONE                               0
#define LINE_OVERLAP_MAJOR                              1
#define LINE_OVERLAP_MINOR                              2


#define LINE_THICKNESS_DRAW_CLOCKWISE                   1
#define LINE_THICKNESS_DRAW_COUNTERCLOCKWISE            2


//---------------------------------------------------------------------------//

static const geo_pixel_t   g_color_none  (242, 239, 233);

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

    // geo_rect_t  map_rect;
    // _find_idx_rect ( m_idx_list, map_rect, m_x_step, m_y_step );
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
        _set_angle(angle);
        _rotate_map_by_angle(center, scale);
        _trim_rotated_map_by_rect();
    }

    // m_geo_angle  =  angle;

    if ( map_update ) {
        _clr_screen();
        // _draw_area();
        _draw_building();
        // _draw_roads();
    }

    return;
}

void geo_processor_t::get_pix ( const map_pos_t& pos, geo_pixel_t& px ) const {

    bool is_error = false;

    px.clear();

    if ( pos.x < 0 ) {
        is_error = true;
    } else
    if ( pos.y < 0 ) {
        is_error = true;
    } else
    if ( pos.x >= m_screen_rect.max.map.x ) {
        is_error = true;
    } else
    if ( pos.y >= m_screen_rect.max.map.y ) {
        is_error = true;
    }

    if ( !is_error ) {

        int32_t offset;

        assert (m_screen_rect.min.map.x == 0 );
        assert (m_screen_rect.min.map.y == 0 );

        offset  = pos.y * m_screen_rect.max.map.x;
        offset += pos.x;

        geo_pixel_int_t tmp;
        tmp = m_video_buffer[offset];

        _px_conv ( tmp, px );
    }

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
    m_scale_step        = 0.1;
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

void geo_processor_t::get_shifts ( const double x, const double y, double& shift_x, double shift_y ) {

    double dist_px_x = 1;
    double step_x    = 0.0001;
    double dist_x    = 0;

    double dist_px_y = 1;
    double step_y    = 0.0001;
    double dist_y    = 0;

    dist_x = gps_distance (  x, y,  x + step_x, y  );
    dist_y = gps_distance (  x, y,  x, y + step_y  );

    shift_x = dist_px_x / dist_x;
    shift_y = dist_px_y / dist_y;

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

//---------------------------------------------------------------------------//

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

void geo_processor_t::_extend_rect ( const geo_coord_t& center, const geo_rect_t& src_rect, geo_rect_t& dst_rect ) const {

    int32_t  shift_hor;
    int32_t  shift_ver;

    shift_hor = src_rect.max.map.x - src_rect.min.map.x;
    shift_ver = src_rect.max.map.y - src_rect.min.map.y;

    dst_rect.min.map.x = center.map.x - shift_hor;
    dst_rect.min.map.y = center.map.y - shift_ver;
    dst_rect.min.reset_angle();

    dst_rect.max.map.x = center.map.x + shift_hor;
    dst_rect.max.map.y = center.map.y + shift_ver;
    dst_rect.max.reset_angle();
}

void geo_processor_t::_find_scale_pixel ( const geo_coord_t& center, const double scale ) {

    double dist_px_x = 1;
    double step_x    = 0.0001;
    double scale_x   = 0;
    double dist_x    = 0;

    double dist_px_y = 1;
    double step_y    = 0.0001;
    double scale_y   = 0;
    double dist_y    = 0;

    dist_px_x /= scale;
    dist_px_y /= scale;

    dist_x  = gps_distance ( center.geo.x, center.geo.y, center.geo.x + step_x, center.geo.y );
    dist_y  = gps_distance ( center.geo.x, center.geo.y, center.geo.x, center.geo.y + step_y );

    scale_x = dist_px_x / dist_x;  // 
    scale_y = dist_px_y / dist_y;  // 

    step_x *= scale_x;
    step_y *= scale_y;

    #if 0
        dist_x = gps_distance ( center.geo.x, center.geo.y, center.geo.x + step_x, center.geo.y );
        dist_y = gps_distance ( center.geo.x, center.geo.y, center.geo.x, center.geo.y + step_y );
    #endif

    m_geo_step_x = step_x;
    m_geo_step_y = step_y;

    return;
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
    if ( pos.map.y <= m_view_rect_ext.min.map.x ) {
        return false;
    }
    if ( pos.map.y >= m_view_rect_ext.max.map.x ) {
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

void geo_processor_t::_get_view_rect ( const geo_rect_t& wnd, const geo_coord_t& center, const double scale, geo_rect_t& view_wnd ) const {

    static const auto sqrt_scale = std::sqrt(2) / 2;

    double x_width  = wnd.width(pos_type_t::POS_TYPE_MAP);
    double y_height = wnd.height(pos_type_t::POS_TYPE_MAP);

    x_width  /= scale;
    y_height /= scale;

    if ( x_width < m_x_step ) {
        x_width = m_x_step;
    }
    if ( y_height < m_y_step ) {
        y_height = m_y_step;
    }

    double  radius = std::sqrt(x_width * x_width + y_height * y_height);
    auto    rect_offset = static_cast<int32_t> ( (radius * sqrt_scale) + 0.5 );

    view_wnd.min.map.x = center.map.x - rect_offset;
    view_wnd.min.map.y = center.map.y - rect_offset;
    view_wnd.max.map.x = center.map.x + rect_offset;
    view_wnd.max.map.y = center.map.y + rect_offset;
}

void geo_processor_t::_extend_view_rect ( const geo_coord_t& center, geo_rect_t& view_rect ) const {

    double x_ext = view_rect.width(pos_type_t::POS_TYPE_MAP);
    double y_ext = view_rect.height(pos_type_t::POS_TYPE_MAP);


    view_rect.min.map.x = static_cast<int32_t> ( 0.5 + center.map.x - x_ext );
    view_rect.min.map.y = static_cast<int32_t> ( 0.5 + center.map.y - y_ext );
    view_rect.max.map.x = static_cast<int32_t> ( 0.5 + center.map.x + x_ext );
    view_rect.max.map.y = static_cast<int32_t> ( 0.5 + center.map.y + y_ext );
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

    for ( auto it_geo_area = m_cache_map.begin(); it_geo_area != m_cache_map.end(); it_geo_area++ ) {

        if ( !it_geo_area->m_to_display ) {
            continue;
        }

        for ( auto it_geo_line = it_geo_area->m_lines.begin(); it_geo_line != it_geo_area->m_lines.end(); it_geo_line++ ) {
            it_geo_line->m_fill_pos.clear();
            _rotate_geo_line ( center, scale, it_geo_line->m_coords );
        }

    }

}

void geo_processor_t::_trim_rotated_map_by_rect ( void ) {

    geo_entry_t     out_record;

    m_paint_list.clear();
    m_view_rect.reset_angle();

    for (auto it_src = m_cache_map.cbegin(); it_src != m_cache_map.cend(); it_src++) {

        if ( !it_src->m_to_display ) {
            continue;
        }

        if ( it_src->m_record_type == OBJID_RECORD_AREA ) {
            _trim_record ( m_view_rect, *it_src, true, out_record );
        } else
        if ( it_src->m_record_type == OBJID_RECORD_BUILDING ) {
            _trim_record ( m_view_rect, *it_src, true, out_record );
        } else
        if ( it_src->m_record_type == OBJID_RECORD_HIGHWAY ) {
            _trim_record ( m_view_rect, *it_src, false, out_record );
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

    static int stop_cnt = 1;
    int out_cnt = 0;

    geo_pixel_t clr1 (255, 0, 0);
    geo_pixel_t clr2 (255, 0, 0);
    int         road_width_m;

    for ( auto it = m_paint_list.begin(); it != m_paint_list.end(); it++ ) {;

        if ( it->m_record_type != OBJID_RECORD_HIGHWAY ) {
            continue;
        }

        switch ( it->m_default_type ) {

            case OBJID_TYPE_STREET:         // асфальтированные жилый улицы. оставляем.
            case OBJID_TYPE_SERVICE:        // асфальтовая дорога. сотавляем.           
            case OBJID_TYPE_RESIDENTIAL:    // асфальтовая дорога. сотавляем.     
                _map_color ( it->m_default_type, clr1, clr2 );
                road_width_m = 3;
                break;

            // асфальтовая дорожка. оставляем.
            case OBJID_TYPE_TRACK:              
                _map_color ( it->m_default_type, clr1, clr2 );
                road_width_m = 1;
                break;

            case OBJID_TYPE_PATH:               // тропинки. пропускаем
            case OBJID_TYPE_FOOTWAY:            // просто возможность пройти. пропускаем.
            case OBJID_TYPE_ROAD:               // таких нет. пропускаем.
            case OBJID_TYPE_STEPS:              // таких нет. пропускаем.
            default:                            // 
                continue;

        }

        road_width_m *= 2;

        for ( auto road_it = it->m_lines.cbegin(); road_it != it->m_lines.cend(); road_it++ ) {
            _poly_line ( road_it->m_coords, road_width_m, clr2 );

            out_cnt++;
            if (out_cnt > stop_cnt) {
                stop_cnt++;
                // return;
            }

        }

    }

    stop_cnt++;
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

    constexpr double pi_rad_scale = 0.01745329251994329576923690768489;

    m_geo_angle     = angle;
    m_geo_angle_sin = sin ( angle * pi_rad_scale );
    m_geo_angle_cos = cos ( angle * pi_rad_scale );

    m_view_angle = angle;
}

void geo_processor_t::_alloc_img_buffer ( const geo_rect_t& geo_rect ) {

    assert ( geo_rect.min.map.x == 0 );
    assert ( geo_rect.min.map.y == 0 );

    size_t buffer_size = geo_rect.max.map.x * geo_rect.max.map.y;

    if ( buffer_size == m_video_buffer.size() ) {
        return;
    }

    m_video_buffer.resize ( buffer_size );
    _fill_solid ( g_color_none );
}

void geo_processor_t::_fill_solid ( const geo_pixel_t clr ) {

    uint32_t alloc_size = m_screen_rect.max.map.x * m_screen_rect.max.map.y;

    geo_pixel_int_t clr_int;
    _px_conv ( clr, clr_int );
    for ( uint32_t i = 0; i < alloc_size; i++ ) {
        m_video_buffer[i] = clr_int;
    }
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

void geo_processor_t::_find_idx_rect ( const l_geo_idx_rec_t& map_idx, geo_rect_t& map_rect, double& x_step, double& y_step ) const {

    geo_pos_t   rect_geo_min;
    geo_pos_t   rect_geo_max;
    map_pos_t   rect_map_min;
    map_pos_t   rect_map_max;

    geo_pos_t   geo_min;
    geo_pos_t   geo_max;
    map_pos_t   map_min;
    map_pos_t   map_max;

    bool       is_first = true;

    for ( auto it = map_idx.cbegin(); it != map_idx.cend(); it++ ) {
        
        geo_min = it->m_rect.min.geo;
        geo_max = it->m_rect.max.geo;

        map_min = it->m_rect.min.map;
        map_max = it->m_rect.max.map;

        if ( is_first ) {

            is_first = false;

            rect_geo_min = geo_min;
            rect_geo_max = geo_max;

            rect_map_min = map_min;
            rect_map_max = map_max;

            x_step = rect_map_max.x - rect_map_min.x;
            y_step = rect_map_max.y - rect_map_min.y;

        } else {

            rect_geo_min.x = std::min ( rect_geo_min.x, geo_min.x );
            rect_geo_min.y = std::min ( rect_geo_min.y, geo_min.y );

            rect_map_min.x = std::min ( rect_map_min.x, map_min.x );
            rect_map_min.y = std::min ( rect_map_min.y, map_min.y );

            rect_geo_max.x = std::max ( rect_geo_max.x, geo_max.x );
            rect_geo_max.y = std::max ( rect_geo_max.y, geo_max.y );

            rect_map_max.x = std::max ( rect_map_max.x, map_max.x );
            rect_map_max.y = std::max ( rect_map_max.y, map_max.y );

        }

    }

    map_rect.min.geo  =  rect_geo_min;
    map_rect.max.geo  =  rect_geo_max;
    map_rect.min.map  =  rect_map_min;
    map_rect.max.map  =  rect_map_max;
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

    for (auto it_coord = coords.begin(); it_coord != coords.end(); it_coord++) {
        _rotate_coord ( center, scale, *it_coord );
    }
}

void geo_processor_t::_rotate_coord (const geo_coord_t& center, const double scale, geo_coord_t& coord ) const {

    double x = coord.map.x - center.map.x;
    double y = coord.map.y - center.map.y;

    x = (x * m_geo_angle_cos)  -  (y * m_geo_angle_sin);
    y = (x * m_geo_angle_sin)  +  (y * m_geo_angle_cos);

    x *= scale;
    y *= scale;

    x += center.map.x;
    y += center.map.y;


    coord.ang.x = static_cast<int32_t> ( x + 0.5 );
    coord.ang.y = static_cast<int32_t> ( y + 0.5 );
}

void geo_processor_t::_trim_record ( const geo_rect_t& rect_path, const geo_entry_t& in_path, const bool is_area, geo_entry_t& out_path ) const {

    v_geo_line_t    res;

    out_path.m_record_type   =  in_path.m_record_type;
    out_path.m_default_type  =  in_path.m_default_type;
    out_path.m_osm_ref       =  in_path.m_osm_ref;
    out_path.m_data_off      =  in_path.m_data_off;
    out_path.m_to_display    =  in_path.m_to_display;
    out_path.m_miss_cnt      =  in_path.m_miss_cnt;

    out_path.m_lines.clear();

    for ( auto it_in_line = in_path.m_lines.cbegin(); it_in_line != in_path.m_lines.cend(); it_in_line++ ) {
        geo_intersect ( POS_TYPE_ANGLE, is_area, *it_in_line, rect_path, res );
        for ( size_t i = 0; i < res.size(); i++ ) {
            out_path.m_lines.push_back( std::move(res[i]) );
        }
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

void geo_processor_t::_poly_area ( const geo_line_t& poly_line, const geo_pixel_t color ) {

    if ( poly_line.m_coords.size() < 3 ) {
        return;
    }

    for ( size_t i = 0; i < poly_line.m_coords.size() - 1; i++ ) {
        _line( poly_line.m_coords[i + 0], poly_line.m_coords[i + 1], 1, color );
    }
}

void geo_processor_t::_pt_geo_to_map ( const map_pos_t& src, map_pos_t& dst ) const {

    dst.x = src.x - m_view_rect.min.ang.x;
    dst.y = src.y - m_view_rect.min.ang.y;
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

void geo_processor_t::_fill_poly ( const geo_coord_t& pos, const geo_pixel_t br_clr, const geo_pixel_t fill_clr, const bool ignore_bk ) {

    (void)(pos);
    (void)(br_clr);
    (void)(fill_clr);
    (void)(ignore_bk);

    assert(false);

    #if 0
    std::queue<geo_coord_t> queue;
    geo_coord_t     p;
    geo_coord_t     next;
    geo_pixel_t     clr;
    int px_cnt = 0;

    queue.push(pos);

    while (  !queue.empty()  ) {

        p = queue.front();
        queue.pop();

        get_pix ( p.ang, clr );

        if ( clr == br_clr ) {
            continue;
        }
        if ( clr == fill_clr ) {
            continue;
        }

        if ( !ignore_bk ) {
            if ( clr != g_color_none ) {
                continue;
            }
        }

        set_pix ( p.ang, fill_clr );
        px_cnt++;

        if ( px_cnt > 4 ) {
            // break;
        }

        if ( p.map.x > (m_view_out.min.map.x) ) {
            next        = p;
            next.map.x -= 1;
            queue.push ( next );
        }

        if ( p.map.x < (m_view_out.max.map.x-1) ) {
            next        = p;
            next.map.x += 1;
            queue.push(next);
        }

        if ( p.map.y > (m_view_out.min.map.y) ) {
            next        = p;
            next.map.y -= 1;
            queue.push(next);
        }

        if ( p.map.y < (m_view_out.max.map.y-1) ) {
            next        = p;
            next.map.y += 1;
            queue.push(next);
        }

    }
    #endif
}

bool geo_processor_t::_pt_in_rect ( const map_pos_t pt, const geo_rect_t& wnd ) const {

    if (pt.x < wnd.min.ang.x) {
        return false;
    }
    if (pt.x > wnd.max.ang.x ) {
        return false;
    }

    if (pt.y < wnd.min.ang.y) {
        return false;
    }
    if (pt.y > wnd.max.ang.y) {
        return false;
    }

    return true;
}

bool geo_processor_t::_is_pt_on_segment ( const geo_coord_t& begin, const geo_coord_t& end, const geo_coord_t& pt ) const {

    const map_pos_t& segmentStart = begin.ang;
    const map_pos_t& segmentEnd   = end.ang;
    const map_pos_t& point        = pt.ang;

    const double p1 = (point.y - segmentStart.y);
    const double p2 = (segmentEnd.x - segmentStart.x);
    const double p3 = (point.x - segmentStart.x);
    const double p4 = (segmentEnd.y - segmentStart.y);

    const double crossProduct = p1 * p2 - p3 * p4;

    if ( crossProduct != 0 ) {
        return false;
    }

    const double dotProduct = p3 * p2 + p1 * p4;
    if (dotProduct < 0) {
        return false;
    }

    const double squaredLength = p2 * p2 + p4 * p4;
    if ( dotProduct > squaredLength ) {
        return false;
    }

    return true;
}

bool geo_processor_t::_pt_in_poly ( const v_geo_coord_t& polygon, const geo_coord_t& point ) const {

    size_t   j = polygon.size() - 2;
    bool     oddNodes = false;
    double   a;
    double   b;
    double   c;
    double   d;

    const map_pos_t& pt = point.ang;

    for (size_t i = 0; i < polygon.size() - 1; i++ ) {

        const map_pos_t& pi = polygon[i].ang;
        const map_pos_t& pj = polygon[j].ang;

        if ( pi.y < pt.y && pj.y >= pt.y || pj.y < pt.y && pi.y >= pt.y ) {

            a = pt.y - pi.y;
            b = pj.y - pi.y;
            c = pj.x - pi.x;
            d = a / b * c;
            d += pi.x;

            if (d < pt.x) {
                oddNodes = !oddNodes;
            }

        }

        j = i;

    }

    return oddNodes;
}

void geo_processor_t::_process_pt_list ( const geo_coord_t& base, const v_paint_offset_t& shift_list, const geo_pixel_t color ) {

    geo_coord_t pt;

    for ( size_t i = 0; i < shift_list.size(); i++ ) {

        pt = base;

        pt.map.x = (pt.map.x + shift_list[i].dx);
        pt.map.y = (pt.map.y + shift_list[i].dy);

        set_pix(pt.ang, color);
    }
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
    color.setR(0);
    color.setG(0);
    color.setB(0);

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


#if 0
void geo_processor_t::_line ( const geo_coord_t& from, const geo_coord_t& to, int width, const geo_pixel_t color ) {

    int     aXStart        = from.ang.x - m_view_geo.min.ang.x;
    int     aYStart        = from.ang.y - m_view_geo.min.ang.y;
    int     aXEnd          = to.ang.x - m_view_geo.min.ang.x;
    int     aYEnd          = to.ang.y - m_view_geo.min.ang.y;
    int     aThickness     = width;
    uint8_t aThicknessMode = 0;

    int i;
    int tDeltaX;
    int tDeltaY;
    int tDeltaXTimes2;
    int tDeltaYTimes2;
    int tError;
    int tStepX;
    int tStepY;

    if (aThickness <= 1) {
        drawLineOverlap ( aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, color );
    }

    tDeltaY = aXEnd - aXStart;
    tDeltaX = aYEnd - aYStart;

    bool tSwap = true;
    if (tDeltaX < 0) {
        tDeltaX = -tDeltaX;
        tStepX  = -1;
        tSwap   = !tSwap;
    } else {
        tStepX = +1;
    }

    if (tDeltaY < 0) {
        tDeltaY = -tDeltaY;
        tStepY  = -1;
        tSwap   = !tSwap;
    } else {
        tStepY = +1;
    }

    tDeltaXTimes2 = tDeltaX << 1;
    tDeltaYTimes2 = tDeltaY << 1;
    bool tOverlap;

    int tDrawStartAdjustCount = aThickness / 2;
    if (aThicknessMode == LINE_THICKNESS_DRAW_COUNTERCLOCKWISE) {
        tDrawStartAdjustCount = aThickness - 1;
    } else 
    if (aThicknessMode == LINE_THICKNESS_DRAW_CLOCKWISE) {
        tDrawStartAdjustCount = 0;
    }

    if (tDeltaX >= tDeltaY) {
        if (tSwap) {
            tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
            tStepY = -tStepY;
        } else {
            tStepX = -tStepX;
        }

        tError = tDeltaYTimes2 - tDeltaX;
        for (i = tDrawStartAdjustCount; i > 0; i--) {
            aXStart -= tStepX;
            aXEnd -= tStepX;
            if (tError >= 0) {
                aYStart -= tStepY;
                aYEnd -= tStepY;
                tError -= tDeltaXTimes2;
            }
            tError += tDeltaYTimes2;
        }

        drawLine ( aXStart, aYStart, aXEnd, aYEnd, color );

        tError = tDeltaYTimes2 - tDeltaX;
        for (i = aThickness; i > 1; i--) {
            aXStart += tStepX;
            aXEnd += tStepX;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                aYStart += tStepY;
                aYEnd += tStepY;
                tError -= tDeltaXTimes2;
                tOverlap = LINE_OVERLAP_MAJOR;
            }
            tError += tDeltaYTimes2;
            drawLineOverlap ( aXStart, aYStart, aXEnd, aYEnd, tOverlap, color );
        }
    } else {

        if (tSwap) {
            tStepX = -tStepX;
        } else {
            tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
            tStepY = -tStepY;
        }

        tError = tDeltaXTimes2 - tDeltaY;
        for (i = tDrawStartAdjustCount; i > 0; i--) {
            aYStart -= tStepY;
            aYEnd -= tStepY;
            if (tError >= 0) {
                aXStart -= tStepX;
                aXEnd -= tStepX;
                tError -= tDeltaYTimes2;
            }
            tError += tDeltaXTimes2;
        }

        drawLine ( aXStart, aYStart, aXEnd, aYEnd, color );

        tError = tDeltaXTimes2 - tDeltaY;

        for (i = aThickness; i > 1; i--) {
            aYStart += tStepY;
            aYEnd += tStepY;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                aXStart += tStepX;
                aXEnd += tStepX;
                tError -= tDeltaYTimes2;
                tOverlap = LINE_OVERLAP_MAJOR;
            }
            tError += tDeltaXTimes2;
            drawLineOverlap ( aXStart, aYStart, aXEnd, aYEnd, tOverlap, color );
        }
    }

}

void geo_processor_t::drawLineOverlap ( unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, uint8_t aOverlap, geo_pixel_t aColor ) {

    int tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;
    map_pos_t draw_pos;

    if ((aXStart == aXEnd) || (aYStart == aYEnd)) {
        fillRect ( aXStart, aYStart, aXEnd, aYEnd, aColor );
        return;
    }

    tDeltaX = aXEnd - aXStart;
    tDeltaY = aYEnd - aYStart;

    if (tDeltaX < 0) {
        tDeltaX = -tDeltaX;
        tStepX = -1;
    } else {
        tStepX = +1;
    }

    if (tDeltaY < 0) {
        tDeltaY = -tDeltaY;
        tStepY = -1;
    } else {
        tStepY = +1;
    }

    tDeltaXTimes2 = tDeltaX << 1;
    tDeltaYTimes2 = tDeltaY << 1;

    draw_pos.x = aXStart;
    draw_pos.y = aYStart;

    set_pix ( draw_pos, aColor );

    if (tDeltaX > tDeltaY) {
        // start value represents a half step in Y direction
        tError = tDeltaYTimes2 - tDeltaX;
        while (aXStart != aXEnd) {
            // step in main direction
            aXStart += tStepX;
            if (tError >= 0) {
                if (aOverlap & LINE_OVERLAP_MAJOR) {
                    draw_pos.x = aXStart;
                    draw_pos.y = aYStart;
                    set_pix(draw_pos, aColor);
                }
                // change Y
                aYStart += tStepY;
                if (aOverlap & LINE_OVERLAP_MINOR) {
                    // draw pixel in minor direction before changing
                    draw_pos.x = aXStart - tStepX;
                    draw_pos.y = aYStart;
                    set_pix ( draw_pos, aColor );
                }
                tError -= tDeltaXTimes2;
            }
            tError += tDeltaYTimes2;
            draw_pos.x = aXStart;
            draw_pos.y = aYStart;
            set_pix ( draw_pos, aColor );
        }
    } else {
        tError = tDeltaXTimes2 - tDeltaY;
        while (aYStart != aYEnd) {
            aYStart += tStepY;
            if (tError >= 0) {
                if (aOverlap & LINE_OVERLAP_MAJOR) {
                    draw_pos.x = aXStart;
                    draw_pos.y = aYStart;
                    set_pix ( draw_pos, aColor );
                }
                aXStart += tStepX;
                if (aOverlap & LINE_OVERLAP_MINOR) {
                    draw_pos.x = aXStart;
                    draw_pos.y = aYStart - tStepY;
                    set_pix(draw_pos, aColor);
                }
                tError -= tDeltaYTimes2;
            }
            tError += tDeltaXTimes2;
            draw_pos.x = aXStart;
            draw_pos.y = aYStart;
            set_pix(draw_pos, aColor);
        }
    }

}

void geo_processor_t::drawLine ( int x1, int y1, int x2, int y2, geo_pixel_t color ) {

    const int deltaX = std::abs(x2 - x1);
    const int deltaY = std::abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;
    int error = deltaX - deltaY;

    map_pos_t draw_pos;

    draw_pos.x = x2;
    draw_pos.y = y2;
    set_pix ( draw_pos, color );

    while (x1 != x2 || y1 != y2) {

        draw_pos.x = x2;
        draw_pos.y = y2;
        set_pix ( draw_pos, color );

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

void geo_processor_t::fillRect ( int aXStart, int aYStart, int aXEnd, int aYEnd, geo_pixel_t aColor ) {

    map_pos_t draw_pos;

    for (int y = aYStart; y < aYEnd; y++) {
        for  ( int x = aXStart; x < aXEnd; x++ ) {
            draw_pos.x = x;
            draw_pos.y = y;
            set_pix ( draw_pos, aColor );
        }
    }
}

#endif

#if 0 

void geo_processor_t::_line_1px(const geo_coord_t& from, const geo_coord_t& to, const geo_pixel_t color) {

    map_pos_t p1;
    map_pos_t p2;

    int   error1;
    int   error2;

    p1 = from.ang;
    p2 = to.ang;

    p1.x -= m_view_geo.min.ang.x;
    p1.y -= m_view_geo.min.ang.y;

    p2.x -= m_view_geo.min.ang.x;
    p2.y -= m_view_geo.min.ang.y;

    assert(p1.x >= 0);
    assert(p1.x < m_view_out.max.map.x);
    assert(p1.y >= 0);
    assert(p1.y < m_view_out.max.map.y);

    const int  deltaX = abs(p2.x - p1.x);
    const int  deltaY = abs(p2.y - p1.y);
    const int  signX = (p1.x < p2.x) ? 1 : -1;
    const int  signY = (p1.y < p2.y) ? 1 : -1;

    error1 = deltaX - deltaY;

    set_pix(p2, color);

    while ((p1.x != p2.x) || (p1.y != p2.y)) {

        set_pix(p1, color);

        error2 = error1 * 2;

        if (error2 > -deltaY) {
            error1 -= deltaY;
            p1.x = (p1.x + signX);
        }

        if (error2 < deltaX) {
            error1 += deltaX;
            p1.y = (p1.y + signY);
        }
    }
}

void geo_processor_t::_line_overlap(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, uint8_t aOverlap, uint16_t aColor) {

    int16_t tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

    if ((aXStart == aXEnd) || (aYStart == aYEnd)) {
        // horizontal or vertical line -> fillRect() is faster than drawLine()
        // fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor);
    }
    else {

        tDeltaX = aXEnd - aXStart;
        tDeltaY = aYEnd - aYStart;

        if (tDeltaX < 0) {
            tDeltaX = -tDeltaX;
            tStepX = -1;
        }
        else {
            tStepX = +1;
        }

        if (tDeltaY < 0) {
            tDeltaY = -tDeltaY;
            tStepY = -1;
        }
        else {
            tStepY = +1;
        }

        tDeltaXTimes2 = tDeltaX << 1;
        tDeltaYTimes2 = tDeltaY << 1;

        // drawPixel(aXStart, aYStart, aColor);

        if (tDeltaX > tDeltaY) {
            tError = tDeltaYTimes2 - tDeltaX;
            while (aXStart != aXEnd) {
                aXStart += tStepX;
                if (tError >= 0) {
                    if (aOverlap & LINE_OVERLAP_MAJOR) {
                        // drawPixel(aXStart, aYStart, aColor);
                    }
                    // change Y
                    aYStart += tStepY;
                    if (aOverlap & LINE_OVERLAP_MINOR) {
                        // drawPixel(aXStart - tStepX, aYStart, aColor);
                    }
                    tError -= tDeltaXTimes2;
                }
                tError += tDeltaYTimes2;
                // drawPixel(aXStart, aYStart, aColor);
            }
        }
        else {

            tError = tDeltaXTimes2 - tDeltaY;
            while (aYStart != aYEnd) {
                aYStart += tStepY;
                if (tError >= 0) {
                    if (aOverlap & LINE_OVERLAP_MAJOR) {
                        // drawPixel(aXStart, aYStart, aColor);
                    }
                    aXStart += tStepX;
                    if (aOverlap & LINE_OVERLAP_MINOR) {
                        // drawPixel(aXStart, aYStart - tStepY, aColor);
                    }
                    tError -= tDeltaYTimes2;
                }
                tError += tDeltaXTimes2;
                // drawPixel (aXStart, aYStart, aColor);
            }
        }
    }
}

void geo_processor_t::_line(const geo_coord_t& from, const geo_coord_t& to, int width, const geo_pixel_t color) {

    if (width == 1) {
        _line_1px(from, to, color);
        return;
    }

#if 0
    paint_offset_t    pt;
    v_paint_offset_t  offsets_list;
    geo_coord_t       p1;
    geo_coord_t       p2;

    if (width % 2) {
        width++;
    }

    int radius = width / 2;

    double x2;
    double y2;
    double r2;

    r2 = radius * radius - 0.5;

    offsets_list.reserve(width * width);

    for (int y = -radius; y <= radius; ++y) {
        y2 = y * y;
        for (int x = -radius; x <= radius; ++x) {
            x2 = x * x;
            if ((x2 + y2) <= r2) {
                pt.dx = x;
                pt.dy = y;
                offsets_list.push_back(pt);
            }
        }
    }

    p1 = from;
    p2 = to;

    int   error1;
    int   error2;

    const int deltaX = static_cast<int> (abs(p2.map.x - p1.map.x));
    const int deltaY = static_cast<int> (abs(p2.map.y - p1.map.y));
    const int signX = (p1.map.x < p2.map.x) ? 1 : -1;
    const int signY = (p1.map.y < p2.map.y) ? 1 : -1;

    error1 = deltaX - deltaY;

    _process_pt_list(p2, offsets_list, color);

    while ((p1.map.x != p2.map.x) || (p1.map.y != p2.map.y)) {

        _process_pt_list(p1, offsets_list, color);

        error2 = error1 * 2;

        if (error2 > -deltaY) {
            error1 -= deltaY;
            p1.map.x = (p1.map.x + signX);
        }

        if (error2 < deltaX) {
            error1 += deltaX;
            p1.map.y = (p1.map.y + signY);
        }
    }

#endif

}

void geo_processor_t::_logged_line(int x1, int y1, const int x2, const int y2, const geo_pixel_t& clr) {

    const int  deltaX = abs(x2 - x1);
    const int  deltaY = abs(y2 - y1);
    const int  signX = x1 < x2 ? 1 : -1;
    const int  signY = y1 < y2 ? 1 : -1;
    map_pos_t  dot_pos;

    int error = deltaX - deltaY;
    bool is_marked = true;

    while ((x1 != x2) || (y1 != y2)) {

        dot_pos.x = x1;
        dot_pos.y = y1;

        set_pix(dot_pos, clr);
        _log_pos(dot_pos, is_marked);

        is_marked = false;

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

    dot_pos.x = x2;
    dot_pos.y = y2;
    set_pix(dot_pos, clr);
    _log_pos(dot_pos, true);
}


void geo_processor_t::_log_pos(const map_pos_t& dot_pos, bool is_marked) {

#if 0
    assert(dot_pos.x >= 0);
    assert(dot_pos.x < m_view_geo.min.ang.x);
    assert(dot_pos.y >= 0);
    assert(dot_pos.y < m_view_geo.min.ang.y);

    int x = dot_pos.x - m_min.x;
    int y = dot_pos.y - m_min.y;

    if (m_matrix.size() <= y) {
        m_matrix.resize(y + 1);
    }

    if (m_matrix[y].size() < (x + 1)) {
        m_matrix[y].resize(x + 1);
    }

    if (is_marked) {
        m_matrix[y][x] += 1;
    }
    else {
        m_matrix[y][x] += 2;
    }
#endif

    return;
}


void geo_processor_t::_generate_paint_pos(geo_line_t& poly_line) const {

    size_t i1;
    size_t i2;
    size_t i3;

    geo_coord_t med;

    const v_geo_coord_t& region = poly_line.m_coords;
    v_geo_coord_t& coords_list = poly_line.m_fill_pos;


    coords_list.clear();

    if (region.size() < 4) {
        return;
    }

    for (size_t i = 2; i < region.size(); i++) {

        i1 = i - 2;
        i2 = i - 1;
        i3 = i - 0;

        // med.ang.x = ( region[i3].ang.x + region[i1].ang.x) / 2;
        // med.ang.y = ( region[i3].ang.y + region[i1].ang.y) / 2;
        // 
        // if ( _is_pt_on_segment(region[i1], region[i2], med) ) {
        //     continue;
        // }
        // if ( _is_pt_on_segment(region[i2], region[i3], med) ) {
        //     continue;
        // }
        // if ( _pt_in_poly(region, med) ) {
        //     coords_list.push_back(med);
        // }

        if (_find_pt_in_poly(region[i1].ang, region[i2].ang, region[i3].ang, med.ang)) {
            coords_list.push_back(med);
        }
    }

}

double geo_processor_t::_calc_dist(const map_pos_t& p1, const map_pos_t& p2, const map_pos_t& p3) const {

    double distance;

    double A = p2.y - p1.y;
    double B = p1.x - p2.x;

    if (B == 0) {
        distance = abs(p3.x - p1.x);
    }
    else {
        double C = p2.x * p1.y - p1.x * p2.y;
        distance = abs(A * p3.x + B * p3.y + C) / sqrt(A * A + B * B);
    }

    return distance;
}

bool geo_processor_t::_find_pt_in_poly(const map_pos_t& p1, const map_pos_t& p2, const map_pos_t& p3, map_pos_t& inside) const {

    double dist;

    inside.x = (p1.x + p3.x) / 2;
    inside.y = (p1.y + p3.y) / 2;

    dist = _calc_dist(p1, p2, inside);
    if (dist < 3) {
        return false;
    }

    dist = _calc_dist(p2, p3, inside);
    if (dist < 3) {
        return false;
    }

    return true;
}

void geo_processor_t::_map_idx ( const geo_rect_t rect, const double& x_step, const double& y_step, l_geo_idx_rec_t& map_idx ) {

    int  y_cnt;
    int  x_cnt;
    int  y;
    int  x;
    double delta;

    delta  = rect.max.y(pos_type_t::POS_TYPE_MAP) - rect.min.y(pos_type_t::POS_TYPE_MAP);
    y_cnt  = static_cast<int> (delta / y_step);

    delta  = rect.max.x(pos_type_t::POS_TYPE_MAP) - rect.min.x(pos_type_t::POS_TYPE_MAP);
    x_cnt  = static_cast<int> (delta / x_step);

    m_idx_map.resize(y_cnt);
    for ( size_t i = 0; i < y_cnt; i++ ) {
        m_idx_map[i].resize(x_cnt);
    }

    auto it = map_idx.begin();
    while ( it != map_idx.end() ) {
        
        delta  = it->m_rect.min.y ( pos_type_t::POS_TYPE_MAP ) - rect.min.y ( pos_type_t::POS_TYPE_MAP );
        y      = static_cast<int> (delta / y_step);

        delta  = it->m_rect.min.x ( pos_type_t::POS_TYPE_MAP ) - rect.min.x ( pos_type_t::POS_TYPE_MAP );
        x      = static_cast<int> (delta / x_step);

        m_idx_map[y][x] = *it;

        map_idx.pop_front();          
        it = map_idx.begin();        
    }

    return;
}

void geo_processor_t::_calc_map_rect ( const geo_coord_t center, const double scale, const paint_rect_t wnd ) {

    map_pos_t map_center;

    int x_shift_1;
    int x_shift_2;
    int y_shift;

    center.get ( POS_TYPE_GPS, map_center );

    x_shift_1 = wnd.width() / 3;
    x_shift_2 = wnd.width() - x_shift_1;

    y_shift = wnd.height() / 2;

    m_map_geo_rect.min.set_x ( POS_TYPE_MAP, map_center.x - x_shift_1 );
    m_map_geo_rect.min.set_y ( POS_TYPE_MAP, map_center.y - y_shift   );

    m_map_geo_rect.max.set_x ( POS_TYPE_MAP, map_center.x + x_shift_2 );
    m_map_geo_rect.max.set_y ( POS_TYPE_MAP, map_center.y + y_shift   );
}



void geo_processor_t::_reset_map ( void ) {

    for ( auto it_geo_area = m_map_cache.begin(); it_geo_area != m_map_cache.end(); it_geo_area++ ) {
        for ( auto it_geo_line = it_geo_area->m_lines.begin(); it_geo_line != it_geo_area->m_lines.end(); it_geo_line++ ) {
            for ( auto it_coord = it_geo_line->m_coords.begin(); it_coord != it_geo_line->m_coords.end(); it_coord++ ) {
                it_coord->reset_angle();
            }
        }
    }

    m_paint_list.clear();;
}

void geo_processor_t::_calc_map_rect ( const geo_coord_t center, double scale, const paint_rect_t wnd ) {

    const double geo_delta = 0.0001;

    double  geo_hor;
    double  geo_ver;
    double  shift_hor;
    double  shift_ver;

    map_pos_t gps_center;
    map_pos_t step_hor;
    map_pos_t step_ver;

    center.get ( gps_center, POS_TYPE_GPS );

    step_hor    = gps_center;
    step_hor.x += geo_delta;

    step_ver    = gps_center;
    step_hor.y += geo_delta;

    GeographicLib::Geodesic geod(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f());

    geod.Inverse ( gps_center.y, gps_center.x, step_hor.y, step_hor.x, shift_hor );
    geod.Inverse ( gps_center.y, gps_center.x, step_ver.y, step_ver.x, shift_ver );

    m_map_step_x = geo_delta / shift_hor / scale;
    m_map_step_y = geo_delta / shift_ver / scale;

    geo_hor = m_map_step_x * ( wnd.width()  - 1 );
    geo_ver = m_map_step_y * ( wnd.height() - 1 );

    map_pos_t  left_bottom;
    map_pos_t  right_top;

    center.get ( left_bottom, pos_type_t::POS_TYPE_GPS );
    center.get ( right_top,   pos_type_t::POS_TYPE_GPS );

    left_bottom.x -= geo_hor / 2;
    left_bottom.y -= geo_ver / 2;

    right_top.x   += geo_hor / 2;
    right_top.y   += geo_ver / 2;

    m_map_geo_rect.min.set ( left_bottom, pos_type_t::POS_TYPE_GPS );
    _pt_to_projection ( left_bottom );
    m_map_geo_rect.min.set ( left_bottom, pos_type_t::POS_TYPE_MAP );

    m_map_geo_rect.max.set ( right_top,   pos_type_t::POS_TYPE_GPS );
    _pt_to_projection ( right_top );
    m_map_geo_rect.min.set ( right_top, pos_type_t::POS_TYPE_MAP );

    

}

void geo_processor_t::_merge_idx ( const v_geo_idx_rec_t& rect_list, const v_uint32_t& in_list, v_uint32_t& map_entries ) {

    set_offset_t tmp_map;
    size_t sub_idx;

    map_entries.clear();

    for ( size_t i = 0; i < in_list.size(); i++ ) {
        sub_idx = in_list[i];
        for (size_t y = 0; y < rect_list[sub_idx].m_list_off.size(); y++) {
            tmp_map.insert(rect_list[sub_idx].m_list_off[y]);
        }
    }

    for ( auto it = tmp_map.cbegin(); it != tmp_map.cend(); it++ ) {
        map_entries.push_back(*it);
    }

    std::sort(map_entries.begin(), map_entries.end());
}



void geo_processor_t::_map_coords ( const v_geo_coord_t& geo_path, v_paint_coord_t& win_path ) {

    paint_coord_t win_coord;

    win_path.clear();
    for ( auto it = geo_path.cbegin(); it != geo_path.cend(); it++ ) {
        _win_coord ( *it, win_coord );
        win_path.push_back ( win_coord );
    }
}

void geo_processor_t::_win_coord ( const geo_coord_t& geo_pos, paint_coord_t& win_pos ) const {

    map_pos_t pos_pt;
    map_pos_t pos_min;

    geo_pos.get ( pos_pt, pos_type_t::POS_TYPE_ANGLE );
    m_map_geo_rect.min.get ( pos_min, pos_type_t::POS_TYPE_ANGLE );

    pos_pt.x -= pos_min.x;
    pos_pt.x /= m_map_step_x;
    pos_pt.x += 0.5;

    pos_pt.y -= pos_min.y;
    pos_pt.y /= m_map_step_y;
    pos_pt.y += 0.5;

    win_pos.x = static_cast<int32_t> (pos_pt.x);
    win_pos.y = static_cast<int32_t> (pos_pt.y);
}

void geo_processor_t::_generate_paint_pos ( const v_paint_coord_t& region, v_paint_coord_t& coords_list ) const {

    size_t i1, i2, i3;
    paint_coord_t med;

    v_paint_coord_t test;

    coords_list.clear();

    if ( region.size() < 4 ) {
        return;
    }

    for (size_t i = 2; i < region.size(); i++) {

        i1 = i - 2;
        i2 = i - 1;
        i3 = i - 0;

        med.x = ( region[i3].x + region[i1].x ) / 2;
        med.y = ( region[i3].y + region[i1].y ) / 2;

        test.push_back(med);

        if ( _is_pt_on_segment(region[i1], region[i2], med) ) {
            continue;
        }

        if ( _is_pt_on_segment(region[i2], region[i3], med) ) {
            continue;
        }

        if ( _pt_in_poly(region, med) ) {
            coords_list.push_back( med );
        }

    }
}

void geo_processor_t::_clip_poly_line ( const v_geo_coord_t& polyline, const geo_rect_t& rect, vv_geo_coord_t& clippedLine ) const {

    pt_code_pos_t pos_prev, pos_new;
    v_geo_coord_t segment;
    geo_coord_t   pt;

    clippedLine.clear();

    if ( polyline.size() < 2 ) {
        return;
    }

    _map_pt_pos ( polyline[0], rect, pos_prev );
    if ( pos_prev == CODE_POS_IN ) {
        segment.push_back( polyline[0] );
    }

    for ( size_t i = 1; i < polyline.size(); i++ ) {
        
        _map_pt_pos ( polyline[i], rect, pos_new );

        if ( pos_new == CODE_POS_IN ) {

            if ( pos_prev != CODE_POS_IN ) {
                if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_prev, rect, pt) ) {
                    segment.push_back ( pt );
                }
            }

            segment.push_back ( polyline[i] );

            pos_prev = pos_new;
            continue;
        }

        if ( pos_prev == CODE_POS_IN ) {

            if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_new, rect, pt) ) {
                segment.push_back(pt);
                if (segment.size() >= 2) {
                    clippedLine.push_back(segment);
                }
                segment.clear();
            }

            pos_prev = pos_new;
            continue;
        }

        if ( pos_prev == pos_new ) {
            continue;
        }

        if ( segment.size() > 0 ) {
            clippedLine.push_back ( segment );
            segment.clear();
            continue;
        }

        if ( pos_prev == pos_new ) {
            continue;
        }

        if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_prev, rect, pt) ) {
            segment.push_back(pt);
            if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_new, rect, pt) ) {
                segment.push_back(pt);
            }
            clippedLine.push_back(segment);
            segment.clear();
        }

        pos_prev = pos_new;

    }

    if ( segment.size() > 0 ) {
        clippedLine.push_back ( segment );
    }

    return;
}


void geo_processor_t::_map_pt_pos ( const geo_coord_t& point, const geo_rect_t& rect, const pos_type_t src, pt_code_pos_t& code ) const {

    if ( point.x(src) < rect.min.x(src) ) {
        code = CODE_POS_LEFT;
    } else
    if ( point.x(src) > rect.max.x(src) ) {
        code = CODE_POS_RIGHT;
    } else
    if ( point.y(src) > rect.max.y(src) ) {
        code = CODE_POS_TOP;
    } else
    if ( point.y(src) < rect.min.y(src) ) {
        code = CODE_POS_BOTTOM;
    } else {
        code = CODE_POS_IN;
    }
}

bool geo_processor_t::_get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const geo_coord_t& p3, const geo_coord_t& p4, const pos_type_t src, geo_coord_t& point ) const {

    assert ( (p3.x(src) == p4.x(src) ) || (p3.y(src) == p4.y(src) ));
    assert ( p3.x(src) <= p4.x(src) );
    assert ( p3.y(src) <= p4.y(src) );

    if ( p3.x(src) == p4.x(src) ) {

        double y = p1.y(src) + ((p2.y(src) - p1.y(src)) * (p3.x(src) - p1.x(src))) / (p2.x(src) - p1.x(src));

        if ( (y > p4.y(src)) || (y < p3.y(src)) ) {
            return false;
        }

        if ( p1.y(src) < p2.y(src) ) {
            if ( (y < p1.y(src)) || (y > p2.y(src)) ) {
                return false;
            }
        } else {
            if ( (y < p2.y(src)) || (y > p1.y(src)) ) {
                return false;
            }
        }

        point.set_x ( src, p3.x(src) );
        point.set_y ( src, y );

    } else {

        double x = p1.x(src) + ((p2.x(src) - p1.x(src)) * (p3.y(src) - p1.y(src))) / (p2.y(src) - p1.y(src) );

        if ( (x > p4.x(src)) || (x < p3.x(src)) ) {
            return false;
        }

        if ( p1.x(src) < p2.x(src) ) {
            if ( (x < p1.x(src)) || (x > p2.x(src)) ) {
                return false;
            }
        } else {
            if ( (x < p2.x(src)) || (x > p1.x(src)) ) {
                return false;
            }
        }

        point.set_x ( src, x );
        point.set_y ( src, p3.y(src) );
    }

    return true;
}

bool geo_processor_t::_get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const pt_code_pos_t border, const geo_rect_t& rect, const pos_type_t src, geo_coord_t& point ) const {

    geo_coord_t p3, p4;
    map_pos_t   pt_min, pt_max;

    rect.min.get ( src, pt_min );
    rect.max.get ( src, pt_max );

    switch (border) {

        case CODE_POS_LEFT:
            p3.set_x ( src, pt_min.x );
            p3.set_y ( src, pt_min.y );
            p4.set_x ( src, pt_min.x );
            p4.set_y ( src, pt_max.y );
            break;

        case CODE_POS_RIGHT:
            p3.set_x ( src, pt_max.x );
            p3.set_y ( src, pt_min.y );
            p4.set_x ( src, pt_max.x );
            p4.set_y ( src, pt_max.y );
            break;

        case CODE_POS_TOP:
            p3.set_x ( src, pt_min.x );
            p3.set_y ( src, pt_max.y );
            p4.set_x ( src, pt_max.x );
            p4.set_y ( src, pt_max.y );
            break;

        case CODE_POS_BOTTOM:
            p3.set_x ( src, pt_min.x );
            p3.set_y ( src, pt_min.y );
            p4.set_x ( src, pt_max.x );
            p4.set_y ( src, pt_min.y );
            break;
    }

    return _get_intersection_pt ( p1, p2, p3, p4, src, point );
}


#endif

//---------------------------------------------------------------------------//
