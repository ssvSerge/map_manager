#include <cassert>
#include <fstream>
#include <algorithm>
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

    geo_rect_t  map_rect;

    _find_idx_rect ( m_idx_list, map_rect, m_x_step, m_y_step );
}

void geo_processor_t::process_map ( const geo_rect_t& paint_wnd, const geo_coord_t& center, const double scale, const double angle ) {

    bool rect_update  = false;
    bool angle_update = false;
    bool map_redraw   = false;

    geo_coord_t     map_center;
    geo_rect_t      map_rect;
    geo_rect_t      map_rect_ext;
    geo_rect_t      load_rect;

    map_center = center;
    map_center.reset_angle();
    map_center.geo_to_map();

    _process_rects ( map_center, scale, paint_wnd, map_rect, map_rect_ext );
    _extend_rect   ( map_center, map_rect_ext, load_rect );

    if (  !_is_scale_valid(scale)  ||  !_is_view_rect_valid(map_rect)  ) {
        rect_update  = true;
        angle_update = true;
    } else
    if ( !_is_angle_valid(angle) ) {
        angle_update = true;
    }

    if ( rect_update ) {

        m_center     =  map_center;
        m_wnd        =  paint_wnd;
        m_wnd_map    =  map_rect_ext;
        m_load_rect  =  load_rect;

        _alloc_img_buffer   ( m_wnd );
        _filter_idx_by_rect ( m_load_rect, m_idx_list, m_map_ids );
        _load_map_by_idx    ( m_map_ids, m_map_cache );

        angle_update = true;
        map_redraw   = true;
    }

    if ( angle_update ) {
        _set_angle ( angle );
        _rotate_map_by_angle ();
        _trim_rotated_map_by_rect ();
        map_redraw = true;

    }

    if ( map_redraw ) {
        _draw_area ();
        _draw_building ();
        _draw_roads ();
    }
}

void geo_processor_t::get_pix ( const geo_coord_t& pos, geo_pixel_t& px ) const {

    bool is_error = false;

    px.clear();

    if ( pos.map.x < 0 ) {
        is_error = true;
    } else
    if ( pos.map.y < 0 ) {
        is_error = true;
    } else
    if ( pos.map.x >= m_wnd.max.map.x ) {
        is_error = true;
    } else
    if ( pos.map.y >= m_wnd.max.map.y ) {
        is_error = true;
    }

    if ( !is_error ) {

        int32_t offset;

        assert ( m_wnd.min.map.x == 0 );
        assert ( m_wnd.min.map.y == 0 );

        offset  = pos.map.y * m_wnd.max.map.x;
        offset += pos.map.x;

        geo_pixel_int_t tmp;
        tmp = m_video_buffer[offset];

        _px_conv ( tmp, px );
    }

}

void geo_processor_t::set_pix ( const geo_coord_t& pos, const geo_pixel_t& px ) {

    bool is_error = false;

    if ( pos.map.x < 0 ) {
        is_error = true;
    } else
    if ( pos.map.y < 0 ) {
        is_error = true;
    } else
    if ( pos.map.x >= m_wnd.max.map.x ) {
        is_error = true;
    } else
    if ( pos.map.y >= m_wnd.max.map.y ) {
        is_error = true;
    }

    if ( ! is_error ) {

        assert ( m_wnd.min.map.x == 0 );
        assert ( m_wnd.min.map.y == 0 );

        geo_pixel_int_t   tmp;
        _px_conv(px, tmp);

        int32_t offset;

        offset  = pos.map.y * m_wnd.max.map.x;
        offset += pos.map.x;

        tmp = m_video_buffer[offset];
        m_video_buffer[offset] = tmp;
    }
}

void geo_processor_t::close ( void ) {

    m_view_angle_step   = 0.5; 
    m_scale_step        = 0.1;

    m_x_step     = 0;
    m_y_step     = 0;    
    m_view_angle = 0;  

    m_wnd.clear();
    m_wnd_map.clear();
    m_wnd_map_ext.clear();
    m_video_buffer.clear();
}

//---------------------------------------------------------------------------//

void geo_processor_t::_extend_rect ( const geo_coord_t& center, const geo_rect_t& src_rect, geo_rect_t& dst_rect ) const {

    int32_t  shift_hor;
    int32_t  shift_ver;

    shift_hor = src_rect.max.map.x - src_rect.min.map.x;
    shift_ver = src_rect.max.map.y - src_rect.min.map.y;

    dst_rect.min.map.x = center.map.x - shift_hor;
    dst_rect.min.map.y = center.map.y - shift_ver;
    dst_rect.max.map.x = center.map.x + shift_hor;
    dst_rect.max.map.y = center.map.y + shift_ver;
}

void geo_processor_t::_process_rects ( const geo_coord_t& center, const double scale, const geo_rect_t& paint_wnd, geo_rect_t& map_rect, geo_rect_t& map_rect_ext ) const {

    static const auto sqrt_scale = std::sqrt(2) / 2;

    double step_x = 1 / scale;
    double step_y = 1 / scale;

    double shift_x = paint_wnd.max.map.x * step_x / 2;
    double shift_y = paint_wnd.max.map.y * step_y / 2;

    double dist_x = paint_wnd.max.map.x;
    double dist_y = paint_wnd.max.map.y;

    map_rect.min.map.x = static_cast<int32_t> ( center.map.x - shift_x + 0.5 );
    map_rect.min.map.y = static_cast<int32_t> ( center.map.y - shift_y + 0.5 );
    map_rect.max.map.x = static_cast<int32_t> ( center.map.x + shift_x + 0.5 );
    map_rect.max.map.y = static_cast<int32_t> ( center.map.y + shift_y + 0.5 );

    double  radius      = std::sqrt ( dist_x*dist_x + dist_y*dist_y );
    auto    rect_offset = static_cast<int32_t> ((radius * sqrt_scale) + 0.5);

    map_rect_ext.min.map.x = center.map.x - rect_offset;
    map_rect_ext.min.map.y = center.map.y - rect_offset;
    map_rect_ext.max.map.x = center.map.x + rect_offset;
    map_rect_ext.max.map.y = center.map.y + rect_offset;
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

void geo_processor_t::_calc_geo_rect ( const geo_coord_t& center, const geo_rect_t& wnd ) {

    double x_max_id = wnd.max.map.x - 1;
    double y_max_id = wnd.max.map.y - 1;

    double x_shift  = (x_max_id/2) * m_geo_step_x;
    double y_shift  = (y_max_id/2) * m_geo_step_y;

    m_wnd_map.min.geo.x = center.geo.x - x_shift;
    m_wnd_map.min.geo.y = center.geo.y - y_shift;
    m_wnd_map.max.geo.x = center.geo.x + x_shift;
    m_wnd_map.max.geo.y = center.geo.y + y_shift;

    m_wnd_map.min.reset_angle();
    m_wnd_map.min.to_projection();
    m_wnd_map.max.reset_angle();
    m_wnd_map.max.to_projection();

    m_prj_step_x = m_wnd_map.max.map.x - m_wnd_map.min.map.x;
    m_prj_step_y = m_wnd_map.max.map.y - m_wnd_map.min.map.y;

    #if 0

        x_shift = m_wnd_map.max.ang.x - m_wnd_map.min.ang.x;
        y_shift = m_wnd_map.max.ang.y - m_wnd_map.min.ang.y;

        auto x_cnt = static_cast<int32_t> ( x_shift / m_geo_step_x + 0.5 );
        auto y_cnt = static_cast<int32_t> ( y_shift / m_geo_step_y + 0.5 );

        assert ( x_cnt == (wnd.max.map.x-1) );
        assert ( y_cnt == (wnd.max.map.y-1) );

    #endif
}

bool geo_processor_t::_is_view_rect_valid ( const geo_rect_t& view_rect ) const {

    if ( view_rect.min.map.x <= m_wnd_map.min.map.x ) {
        return false;
    }
    if ( view_rect.max.map.x >= m_wnd_map.max.map.x ) {
        return false;
    }

    if ( view_rect.min.map.y <= m_wnd_map.min.map.y ) {
        return false;
    }
    if ( view_rect.max.map.y >= m_wnd_map.max.map.y ) {
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

bool geo_processor_t::_is_scale_valid ( const double scale ) const {

    double delta = scale - m_scale;

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
        if ( _is_overlapped(base_rect, POS_TYPE_MAP, it->m_rect) ) {
            for (auto ref = it->m_list_off.cbegin(); ref != it->m_list_off.cend(); ref++ ) {
                insert_cnt++;
                nodes_list.insert ( *ref );
            }
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

    geo_entry_t     new_entry;
    bool            already_known;

    for ( auto map_entry_it = map_cache.begin(); map_entry_it != map_cache.end(); map_entry_it++ ) {
        map_entry_it->m_to_display = false;
        map_entry_it->m_miss_cnt++;
    }

    for ( auto id = map_entries.cbegin(); id != map_entries.cend(); id++ ) {

        already_known = false;

        for ( auto idx_it = map_cache.begin(); idx_it != map_cache.end(); idx_it++ ) {
            if ( idx_it->m_data_off == *id ) {
                already_known = true;
                idx_it->m_to_display = true;
                idx_it->m_miss_cnt = 0;
                break;
            }
        }

        if ( !already_known ) {
            new_entry.clear();
            _load_map_entry(*id, new_entry);
            new_entry.m_to_display = true;
            new_entry.m_miss_cnt   = 0;
            map_cache.push_back(new_entry);
        }
    }
}

void geo_processor_t::_rotate_map_by_angle ( void ) {

    for ( auto it_geo_area = m_map_cache.begin(); it_geo_area != m_map_cache.end(); it_geo_area++ ) {

        if ( !it_geo_area->m_to_display ) {
            continue;
        }

        for ( auto it_geo_line = it_geo_area->m_lines.begin(); it_geo_line != it_geo_area->m_lines.end(); it_geo_line++ ) {
            it_geo_line->m_fill_pos.clear();
            _rotate_geo_line ( it_geo_line->m_coords );
        }

    }

}

void geo_processor_t::_trim_rotated_map_by_rect ( void ) {

    geo_entry_t     out_record;

    m_paint_list.clear();
    m_wnd_map_ext.reset_angle();

    for (auto it_src = m_map_cache.cbegin(); it_src != m_map_cache.cend(); it_src++) {

        if ( !it_src->m_to_display ) {
            continue;
        }

        if ( it_src->m_record_type == OBJID_RECORD_AREA ) {
            _trim_record ( m_wnd_map_ext, *it_src, true, out_record );
        } else
        if ( it_src->m_record_type == OBJID_RECORD_BUILDING ) {
            _trim_record ( m_wnd_map_ext, *it_src, true, out_record );
        } else
        if ( it_src->m_record_type == OBJID_RECORD_HIGHWAY ) {
            _trim_record ( m_wnd_map_ext, *it_src, false, out_record );
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
            _process_area ( *it, false, false );
        } else {

            for ( size_t i = 0; i < cnt; i++ ) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_OUTER ) {
                    _process_area ( *it, false, false );
                }
            }

            for ( size_t i = 0; i < cnt; i++ ) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_INNER ) {
                    _process_area ( *it, false, false );
                }
            }

        }
    }
}

void geo_processor_t::_draw_building ( void ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    size_t cnt;

    int  start_cnt = 15;
    int  end_cnt = 20;
    int  out_cnt = 0;

    for ( auto it = m_paint_list.begin(); it != m_paint_list.end(); it++) {

        if (it->m_record_type != OBJID_RECORD_BUILDING) {
            continue;
        }

        out_cnt++;

        if (out_cnt < start_cnt || out_cnt > end_cnt) {
            continue;
        }

        cnt = it->m_lines.size();

        if (cnt == 1) {
            _process_area ( *it, true, true);
        }
        else {

            for (size_t i = 0; i < cnt; i++) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_OUTER ) {
                    _process_area( *it, true, true );
                }
            }
            for (size_t i = 0; i < cnt; i++) {
                if ( it->m_lines[i].m_role != OBJID_ROLE_OUTER ) {
                    _process_area ( *it, true, true );
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
            if ( out_cnt >= stop_cnt ) {
                // break;
            }
            _poly_line ( road_it->m_coords, road_width_m, clr2 );
            out_cnt++;
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
            GEO_RGB ( fill_color,   255, 173, 209, 158 );
            GEO_RGB ( border_color, 255, 163, 199, 148 );
            break;

        case OBJID_TYPE_GRASS:        
            GEO_RGB ( fill_color,   255, 205, 235, 176 );
            GEO_RGB ( border_color, 255, 185, 215, 156 );
            break;

        case OBJID_TYPE_ASPHALT:      
            GEO_RGB ( fill_color,   255, 174, 174, 179 );
            GEO_RGB ( border_color, 255, 221, 221, 232 );
            break;

        case OBJID_TYPE_BUILDING:     
            GEO_RGB ( fill_color,   255, 217, 208, 201 );
            GEO_RGB ( border_color, 255, 195, 154, 100 );
            break;

        case OBJID_TYPE_WATER:        
            GEO_RGB ( fill_color,   255, 100, 100, 200 );
            GEO_RGB ( border_color, 255,   0,   0,   0 );   
            break;

        case OBJID_TYPE_GENERAL:      
            GEO_RGB ( fill_color,   255,  80, 120,  80 );
            GEO_RGB ( border_color, 255, 150, 150, 250 );
            break;

        case OBJID_TYPE_MOUNTAIN:     
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_STONE:        
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_SAND:         
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_UNDEFINED:    
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_FOOTWAY:      
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_ROAD:         
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_SECONDARY:    
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_TRUNK:        
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_MOTORWAY:     
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_PRIMARY:      
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_TERTIARY:     
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_RAILWAY:      
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_RIVER:        
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_BRIDGE:       
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_TYPE_TUNNEL:       
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_RECORD_AREA:       
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_RECORD_BUILDING:   
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
            break;

        case OBJID_RECORD_HIGHWAY:    
        case OBJID_TYPE_STREET:
        case OBJID_TYPE_SERVICE:
        case OBJID_TYPE_RESIDENTIAL:
            GEO_RGB ( fill_color,   255, 174, 174, 179 );
            GEO_RGB ( border_color, 255, 221, 221, 232 );
            break;

        case OBJID_TYPE_TRACK:
            break;

        default:
            GEO_RGB ( fill_color, 255, 255, 255, 255 );
            GEO_RGB ( border_color, 0,   0,   0,   0 );
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

    uint32_t alloc_size = m_wnd.max.map.x * m_wnd.max.map.y;

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

void geo_processor_t::_rotate_geo_line ( v_geo_coord_t& coords ) const {

    for (auto it_coord = coords.begin(); it_coord != coords.end(); it_coord++) {
        _rotate_coord (*it_coord);
    }
}

void geo_processor_t::_rotate_coord ( geo_coord_t& coord ) const {

    map_pos_t  pt;
    map_pos_t  center;

    double  translated_x = 0;
    double  translated_y = 0;
    double  rotated_x = 0;
    double  rotated_y = 0;

    pt = coord.map;
    center = m_geo_center.map;

    translated_x = pt.x - center.x;
    translated_y = pt.y - center.y;

    rotated_x   =  translated_x * m_geo_angle_cos - translated_y * m_geo_angle_sin;
    rotated_x  +=  center.x;
                   
    rotated_y   =  translated_x * m_geo_angle_sin + translated_y * m_geo_angle_cos;
    rotated_y  +=  center.y;
                   
    coord.ang.x = static_cast<int32_t> (rotated_x + 0.5);
    coord.ang.y = static_cast<int32_t> (rotated_y + 0.5);
}

void geo_processor_t::_trim_record ( const geo_rect_t& rect_path, const geo_entry_t& in_path, const bool is_area, geo_entry_t& out_path ) const {

    v_geo_line_t    res;

    out_path.m_record_type  = in_path.m_record_type;
    out_path.m_default_type = in_path.m_default_type;
    out_path.m_osm_ref      = in_path.m_osm_ref;
    out_path.m_data_off     = in_path.m_data_off;
    out_path.m_to_display   = in_path.m_to_display;
    out_path.m_miss_cnt     = in_path.m_miss_cnt;
    out_path.m_lines.clear();

    for ( auto it_in_line = in_path.m_lines.cbegin(); it_in_line != in_path.m_lines.cend(); it_in_line++ ) {
        geo_intersect ( POS_TYPE_ANGLE, is_area, *it_in_line, rect_path, res );
        for ( size_t i = 0; i < res.size(); i++ ) {
            out_path.m_lines.push_back( std::move(res[i]) );
        }
    }

    return;
}

void geo_processor_t::_process_area ( geo_entry_t& geo_line, const bool force_clr, const bool mark_up ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    _map_color ( geo_line.m_default_type, border_color, fill_color );

    for ( size_t i = 0; i < geo_line.m_lines.size(); i++ ) {
        _poly_area ( geo_line.m_lines[i], border_color );
        _fill_poly ( geo_line.m_lines[i], border_color, fill_color, force_clr, mark_up );
    }
}

void geo_processor_t::_poly_line ( const v_geo_coord_t& poly_line, const int width, const geo_pixel_t color ) {

    (void)poly_line;
    (void)width;
    (void)color;

    // if (poly_line.size() >= 2) {
    //     for (size_t i = 0; i < poly_line.size() - 1; i++) {
    //         _line ( poly_line[i + 0], poly_line[i + 1], width, color );
    //     }
    // }
}

void geo_processor_t::_poly_area ( const geo_line_t& poly_line, const geo_pixel_t color ) {

    if ( poly_line.m_coords.size() < 3 ) {
        return;
    }

    for ( size_t i = 0; i < poly_line.m_coords.size() - 1; i++ ) {
        _line ( poly_line.m_coords[i + 0], poly_line.m_coords[i + 1], color );
    }
}

void geo_processor_t::_fill_poly ( geo_line_t& poly_line, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr, const bool force_clr, const bool mark_up ) {

    if ( poly_line.m_fill_pos.size() == 0 ) {
        _generate_paint_pos ( poly_line );
    }

    for ( auto paint_pt = poly_line.m_fill_pos.cbegin(); paint_pt != poly_line.m_fill_pos.cend(); paint_pt++ ) {
        _fill_poly ( *paint_pt, bk_clr, fill_clr, force_clr );
    }

    if ( mark_up ) {
        for ( auto paint_pt = poly_line.m_fill_pos.cbegin(); paint_pt != poly_line.m_fill_pos.cend(); paint_pt++ ) {
            set_pix ( *paint_pt, geo_pixel_t(255, 0, 0) );
        }
    }
}

void geo_processor_t::_fill_poly ( const geo_coord_t& pos, const geo_pixel_t br_clr, const geo_pixel_t fill_clr, const bool ignore_bk ) {

    std::queue<geo_coord_t> queue;
    geo_coord_t     p;
    geo_coord_t     next;
    geo_pixel_t     clr;
    int px_cnt = 0;

    queue.push(pos);

    while (  !queue.empty()  ) {

        p = queue.front();
        queue.pop();

        get_pix ( p, clr );

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

        set_pix ( p, fill_clr );
        px_cnt++;

        if ( px_cnt > 4 ) {
            // break;
        }

        if ( p.map.x > (m_wnd.min.map.x) ) {
            next        = p;
            next.map.x -= 1;
            queue.push ( next );
        }

        if ( p.map.x < (m_wnd.max.map.x-1) ) {
            next        = p;
            next.map.x += 1;
            queue.push(next);
        }

        if ( p.map.y > (m_wnd.min.map.y) ) {
            next        = p;
            next.map.y -= 1;
            queue.push(next);
        }

        if ( p.map.y < (m_wnd.max.map.y-1) ) {
            next        = p;
            next.map.y += 1;
            queue.push(next);
        }

    }
}

void geo_processor_t::_line ( const geo_coord_t& from, const geo_coord_t& to, const geo_pixel_t color ) {

    geo_coord_t p1;
    geo_coord_t p2;

    int   error1;
    int   error2;

    p1 = from;             
    p2 = to;

    p1.ang.x -= m_wnd.min.ang.x;
    p1.ang.y -= m_wnd.min.ang.y;

    p2.ang.x -= m_wnd.min.ang.x;
    p2.ang.y -= m_wnd.min.ang.y;

    assert ( p1.ang.x >= 0);
    assert ( p1.ang.x < m_wnd.max.map.x );
    assert ( p1.ang.y >= 0);
    assert ( p1.ang.y < m_wnd.max.map.y );

    const int  deltaX  =  abs ( p2.ang.x - p1.ang.x );
    const int  deltaY  =  abs ( p2.ang.y - p1.ang.y );
    const int  signX   =  ( p1.ang.x  <  p2.ang.x ) ? 1 : -1;
    const int  signY   =  ( p1.ang.y  <  p2.ang.y ) ? 1 : -1;

    error1 = deltaX - deltaY;

    set_pix ( p2, color );

    while ( ( p1.ang.x != p2.ang.x ) || (p1.ang.y != p2.ang.y) ) {

        set_pix ( p1, color );

        error2 = error1 * 2;

        if ( error2 > -deltaY ) {
            error1   -= deltaY;
            p1.ang.x  = ( p1.ang.x + signX );
        }

        if ( error2 < deltaX ) {
            error1   += deltaX;
            p1.ang.y  = ( p1.ang.y + signY );
        }
    }
}

void geo_processor_t::_line ( const geo_coord_t from, const geo_coord_t to, int width, const geo_pixel_t color ) {

    paint_offset_t    pt;
    v_paint_offset_t  offsets_list;
    geo_coord_t       p1;
    geo_coord_t       p2;

    if ( width % 2 ) {
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

    const int deltaX = static_cast<int> (abs(p2.map.x - p1.map.x ));
    const int deltaY = static_cast<int> (abs(p2.map.y - p1.map.y ));
    const int signX = ( p1.map.x < p2.map.x ) ? 1 : -1;
    const int signY = ( p1.map.y < p2.map.y ) ? 1 : -1;

    error1 = deltaX - deltaY;

    _process_pt_list ( p2, offsets_list, color );

    while ((p1.map.x != p2.map.x ) || (p1.map.y != p2.map.y) ) {

        _process_pt_list ( p1, offsets_list, color );

        error2 = error1 * 2;

        if ( error2 > -deltaY ) {
            error1  -= deltaY;
            p1.map.x = ( p1.map.x + signX );
        }

        if ( error2 < deltaX ) {
            error1  += deltaX;
            p1.map.y = (p1.map.y + signY);
        }
    }

}

void geo_processor_t::_process_pt_list ( const geo_coord_t base, const v_paint_offset_t& shift_list, const geo_pixel_t color ) {

    geo_coord_t pt;

    for ( size_t i = 0; i < shift_list.size(); i++ ) {
        
        pt = base;

        pt.map.x = ( pt.map.x + shift_list[i].dx );
        pt.map.y = ( pt.map.y + shift_list[i].dy );

        set_pix ( pt, color );
    }
}

void geo_processor_t::_generate_paint_pos ( geo_line_t& poly_line ) const {

    (void) (poly_line);

    #if 0
    size_t i1, i2, i3;
    paint_coord_t med;

    v_paint_coord_t test;

    coords_list.clear();

    if (region.size() < 4) {
        return;
    }

    for (size_t i = 2; i < region.size(); i++) {

        i1 = i - 2;
        i2 = i - 1;
        i3 = i - 0;

        med.x = (region[i3].x + region[i1].x) / 2;
        med.y = (region[i3].y + region[i1].y) / 2;

        test.push_back(med);

        if (_is_pt_on_segment(region[i1], region[i2], med)) {
            continue;
        }

        if (_is_pt_on_segment(region[i2], region[i3], med)) {
            continue;
        }

        if (_pt_in_poly(region, med)) {
            coords_list.push_back(med);
        }

    }

    #endif
}

#if 0

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

bool geo_processor_t::_is_pt_on_segment ( const paint_coord_t segmentStart, const paint_coord_t segmentEnd, const paint_coord_t point ) const {

    const double p1 = ( point.y       -  segmentStart.y );
    const double p2 = ( segmentEnd.x  -  segmentStart.x );
    const double p3 = ( point.x       -  segmentStart.x );
    const double p4 = ( segmentEnd.y  -  segmentStart.y );

    const double crossProduct = p1 * p2 - p3 * p4;
    if ( crossProduct != 0 ) {
        return false;
    }

    const double dotProduct = p3 * p2 + p1 * p4;
    if ( dotProduct < 0) {
        return false;
    }

    const double squaredLength = p2 * p2 + p4 * p4;
    if ( dotProduct > squaredLength ) {
        return false;
    }

    return true;
}

bool geo_processor_t::_pt_in_poly ( const v_paint_coord_t& polygon, const paint_coord_t& pt ) const {

    size_t   i;
    size_t   j = polygon.size() - 2;
    bool     oddNodes = false;
    double   a, b, c, d;

    for (i = 0; i < polygon.size() - 1; i++) {
        if (polygon[i].y < pt.y && polygon[j].y >= pt.y || polygon[j].y < pt.y && polygon[i].y >= pt.y) {

            a  = pt.y         - polygon[i].y;
            b  = polygon[j].y - polygon[i].y;
            c  = polygon[j].x - polygon[i].x;
            d  = (a) / (b) * (c);
            d += polygon[i].x;

            if ( d < pt.x) {
                oddNodes = !oddNodes;
            }
        }
        j = i;
    }

    return oddNodes;
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
