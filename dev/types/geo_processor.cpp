#include <cassert>
#include <fstream>
#include <algorithm>
#include <stack>

#include <GeographicLib/Geodesic.hpp>

#include <geo_processor.h>

//---------------------------------------------------------------------------//

#define GEO_RGB(var,in_a,in_r,in_g,in_b)     { var.setR(in_r); var.setG(in_g); var.setB(in_b); }

//---------------------------------------------------------------------------//

static const geo_pixel_t   g_color_none  (18, 18, 18);

//---------------------------------------------------------------------------//

geo_processor_t::geo_processor_t() {
    m_video_buffer = nullptr;
    m_video_height = 0;
    m_video_width  = 0;
    m_scale        = 1;
    m_step_x       = 1;
    m_step_y       = 1;
    m_center.x     = 0;
    m_center.y     = 0;
    m_angle        = 0;
    m_angle_sin    = 0; // sin(m_angle);
    m_angle_cos    = 1; // cos(m_angle);
}

//---------------------------------------------------------------------------//

void geo_processor_t::close ( void ) {

    m_video_height = 0;
    m_video_width = 0;

    delete m_video_buffer;
    m_video_buffer = nullptr;
}

void geo_processor_t::set_names ( const char* const idx_file_name, const char* const map_file_name ) {

    m_map_file_name = map_file_name;
    m_idx_file_name = idx_file_name;

    m_map_file.open ( m_map_file_name, std::ios_base::binary );
}

void geo_processor_t::alloc_buffer ( uint32_t width, uint32_t height ) {

    if (m_video_buffer != nullptr) {
        delete (m_video_buffer);
        m_video_buffer = nullptr;
    }

    m_video_height = height;
    m_video_width = width;

    uint32_t alloc_size = width * height;

    m_video_buffer = new geo_pixel_int_t[alloc_size];

    assert(m_video_buffer != nullptr);

    fill_solid (g_color_none );
}

void geo_processor_t::get_pix ( const paint_coord_t& pos, geo_pixel_t& px ) const {

    int32_t offset;
    geo_pixel_int_t tmp;

    assert(pos.x <= m_video_width);
    assert(pos.y <= m_video_height);

    offset = (pos.y * m_video_width) + pos.x;

    tmp = m_video_buffer[offset];

    _px_conv (tmp, px);
}

void geo_processor_t::set_pix ( const paint_coord_t& pos, const geo_pixel_t& px ) {

    int32_t offset;
    geo_pixel_int_t tmp;

    assert ( pos.x < m_video_width  );
    assert ( pos.y < m_video_height );

    _px_conv ( px, tmp );

    offset = ( pos.y * m_video_width ) + pos.x;
    m_video_buffer[offset] = tmp;
}

void geo_processor_t::fill_solid (const geo_pixel_t clr ) {

    geo_pixel_int_t clr_int;
    uint32_t cnt = m_video_height * m_video_width;

    _px_conv (clr, clr_int);

    for ( uint32_t i = 0; i < cnt; i++ ) {
        m_video_buffer[i] = clr_int;
    }
}

void geo_processor_t::cache_init() {

    m_map_idx.clear();

    _load_idx ( m_map_idx );
    _find_base_rect ( m_map_idx, m_map_rect );
}

void geo_processor_t::set_base_params ( const geo_coord_t center, const double scale, const paint_rect_t wnd ) {

    v_uint32_t  rects_list;
    v_uint32_t  map_entrie;

    _set_scales   ( center, scale, wnd );
    _filter_rects ( m_map_idx, m_geo_rect, rects_list );
    _merge_idx    ( m_map_idx, rects_list, map_entrie );
    _load_map     ( map_entrie, m_map_cache );
}

void geo_processor_t::set_angle ( const double angle ) {

    constexpr double pi_rad_scale = 0.01745329251994329576923690768489;

    m_angle = angle;

    m_angle_sin = sin ( angle * pi_rad_scale );
    m_angle_cos = cos ( angle * pi_rad_scale );
}

void geo_processor_t::trim_map (void) {

    _trim_map ();
    _process_window ();
    _validate_window_rect ();
}

void geo_processor_t::_process_line ( geo_line_t& geo_line, bool mark_up ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    _map_color ( geo_line.m_type,  border_color, fill_color );
    _poly_line ( geo_line.m_paint, border_color );
    _fill_poly ( geo_line.m_paint, geo_line.m_fill, border_color, fill_color, mark_up );
}

void geo_processor_t::process_wnd (void) {

    _process_area();
    _process_building();
}

void geo_processor_t::_process_area ( void ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    size_t cnt;

    for ( auto it = m_draw_list.begin(); it != m_draw_list.end(); it++ ) {

        if ( it->m_record_type != OBJID_RECORD_AREA ) {
            continue;
        }

        cnt = it->m_lines.size();

        if ( cnt == 1 ) {
            _process_line( it->m_lines[0] );
        } else {

            for (size_t i = 0; i < cnt; i++) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_OUTER ) {
                    _process_line( it->m_lines[i] );
                }
            }
            for (size_t i = 0; i < cnt; i++) {
                if (it->m_lines[i].m_role != OBJID_ROLE_OUTER) {
                    _process_line(it->m_lines[i]);
                }
            }

        }

    }
}

void geo_processor_t::_process_building ( void ) {

    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    int draw_cnt = 0;
    static int stop_cnt = 0;

    size_t cnt;

    for ( auto it = m_draw_list.begin(); it != m_draw_list.end(); it++ ) {

        if ( it->m_record_type != OBJID_RECORD_BUILDING ) {
            continue;
        }

        cnt = it->m_lines.size();

        if ( cnt == 1 ) {
            if (draw_cnt > stop_cnt) {
                break;
            }
            _process_line ( it->m_lines[0], true );
            draw_cnt++;
        } else {

            for (size_t i = 0; i < cnt; i++) {
                if ( it->m_lines[i].m_role == OBJID_ROLE_OUTER ) {
                    if ( draw_cnt > stop_cnt ) {
                        break;
                    }
                    _process_line ( it->m_lines[i], true );
                    draw_cnt++;
                }
            }
            for (size_t i = 0; i < cnt; i++) {
                if (it->m_lines[i].m_role != OBJID_ROLE_OUTER) {
                    if ( draw_cnt > stop_cnt ) {
                        break;
                    }
                    _process_line ( it->m_lines[i], true );
                    draw_cnt++;
                }
            }


        }

    }

    stop_cnt++;
}

//---------------------------------------------------------------------------//

void geo_processor_t::_poly_line ( const v_paint_coord_t& poly_line, const geo_pixel_t color ) {

    if ( poly_line.size() < 2 ) {
        return;
    }

    for ( size_t i = 0; i < poly_line.size() - 1; i++ ) {
        _line ( poly_line[i + 0], poly_line[i + 1], color );
    }
}

void geo_processor_t::_fill_poly ( const v_paint_coord_t& poly_line, v_paint_coord_t& coords_list, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr, bool mark_up ) {

    if ( coords_list.size() == 0 ) {
        _generate_paint_pos ( poly_line, coords_list );
    }

    for ( auto paint_pt = coords_list.cbegin(); paint_pt != coords_list.cend(); paint_pt++ ) {
        _fill_poly ( *paint_pt, bk_clr, fill_clr );
    }

    if (mark_up) {
        for ( auto paint_pt = coords_list.cbegin(); paint_pt != coords_list.cend(); paint_pt++ ) {
            set_pix ( *paint_pt, geo_pixel_t(255, 0, 0) );
        }
    }
}

void geo_processor_t::_fill_poly ( const paint_coord_t& pos, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr ) {

    std::queue<paint_coord_t> queue;
    paint_coord_t p;
    paint_coord_t next;
    geo_pixel_t clr;
    int px_cnt = 0;

    (void) (bk_clr);

    queue.push(pos);

    while ( queue.size() > 0 ) {

        p = queue.front();
        queue.pop();

        get_pix ( p, clr );

        if ( clr != g_color_none ) {
            continue;
        }

        set_pix ( p, fill_clr );
        px_cnt++;

        if (px_cnt > 4) {
            // break;
        }

        if ( p.x > 0 ) {

            next    = p;
            next.x -= 1;

            queue.push(next);

        }
        if (p.x < m_video_width-1) {

            next    = p;
            next.x += 1;

            queue.push(next);
        }
        if (p.y > 0) {

            next    = p;
            next.y -= 1;

            queue.push(next);
        }
        if (p.y < m_video_height-1) {

            next    = p;
            next.y += 1;

            queue.push(next);
        }
    }
}

void geo_processor_t::_px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const {
            
    uint16_t val = 0;

    val  |= (from.getR() >> 3);

    val <<= 6;
    val  |= (from.getG() >> 2);

    val <<= 5;
    val |= (from.getB() >> 3);

    to = val;
}

void geo_processor_t::_px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const {

    uint8_t tmp = 0;

    tmp   = static_cast<uint8_t> (from >> 8);
    tmp  &= 0xF8;
    to.setR ( tmp );

    tmp  = static_cast<uint8_t> (from >> 3);
    tmp &= 0xFC;
    to.setG(tmp);

    tmp  = static_cast<uint8_t> (from << 3);
    tmp &= 0xF8;
    to.setB(tmp);
}

void geo_processor_t::_load_idx ( v_geo_idx_rec_t& idx_list ) {

    geo_parser_t    parser;
    std::ifstream   idx_file;
    geo_idx_rec_t   geo_idx;
    geo_offset_t    file_offset = 0;
    char            ch;
    bool            eoc;
    bool            eor;

    idx_file.open ( m_idx_file_name, std::ios_base::binary );

    while ( idx_file.read(&ch, 1) ) {

        parser.load_param(ch, eoc, file_offset);
        file_offset++;

        if (!eoc) {
            continue;
        }

        parser.process_idx(geo_idx, eor);

        if (!eor) {
            continue;
        }

        idx_list.push_back(geo_idx);
    }
}

void geo_processor_t::_find_base_rect ( const v_geo_idx_rec_t& map_idx, geo_rect_t& map_rect ) {

    for ( size_t i = 0; i < map_idx.size(); i++ ) {
    
        if ( i == 0 ) {
            map_rect = map_idx[i].m_rect;
        } else {
            map_rect.min.x = std::min ( map_rect.min.x, map_idx[i].m_rect.min.x );
            map_rect.min.y = std::min ( map_rect.min.y, map_idx[i].m_rect.min.y );
            map_rect.max.x = std::max ( map_rect.max.x, map_idx[i].m_rect.max.x );
            map_rect.max.y = std::max ( map_rect.max.y, map_idx[i].m_rect.max.y );
        }

    }
}

void geo_processor_t::_set_scales ( const geo_coord_t center, double scale, const paint_rect_t wnd ) {

    const double geo_delta = 0.0001;

    double  shift_hor = 0;
    double  shift_ver = 0;
    double  geo_hor;
    double  geo_ver;

    m_scale    = scale;
    m_center   = center;

    m_geo_rect.min.y = m_geo_rect.max.y = center.y;
    m_geo_rect.min.x = m_geo_rect.max.x = center.x;

    GeographicLib::Geodesic geod(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f());

    geod.Inverse ( 
        center.y, center.x, 
        center.y, center.x + geo_delta, 
        shift_hor 
    );

    geod.Inverse ( 
        center.y, center.x, 
        center.y + geo_delta, center.x, 
        shift_ver 
    );

    m_step_x  = geo_delta / shift_hor;
    m_step_y  = geo_delta / shift_ver;

    m_step_x /= scale;
    m_step_y /= scale;

    geo_hor = m_step_x * (wnd.width()  - 1);
    geo_ver = m_step_y * (wnd.height() - 1);

    m_geo_rect.min.y = center.y - (geo_ver / 2);
    m_geo_rect.max.y = center.y + (geo_ver / 2);

    m_geo_rect.min.x = center.x - (geo_hor / 2);
    m_geo_rect.max.x = center.x + (geo_hor / 2);


#if 1
    geod.Inverse ( 
        m_geo_rect.min.y, m_geo_rect.min.x,
        m_geo_rect.min.y, m_geo_rect.max.x,
        shift_hor
    );
    shift_hor /= scale;

    geod.Inverse (
        m_geo_rect.min.y, m_geo_rect.min.x,
        m_geo_rect.max.y, m_geo_rect.min.x,
        shift_ver
    );
    shift_ver /= scale;
#endif

}

void geo_processor_t::_filter_rects ( const v_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, v_uint32_t& out_list ) {

    for ( uint32_t i = 0; i < rect_list.size(); i++ ) {
        if ( _is_overlapped(base_rect, rect_list[i].m_rect) ) {
            out_list.push_back(i);
        }
    }
}

bool geo_processor_t::_is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice ) const {

    if ( window.min.x > slice.max.x ) {
        return false;
    }
    if ( window.max.x < slice.min.x ) {
        return false;
    }

    if ( window.min.y > slice.max.y ) {
        return false;
    }
    if ( window.max.y < slice.min.y ) {
        return false;
    }

    return true;
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

void geo_processor_t::_load_map ( const v_uint32_t& map_entries, l_geo_entry_t& map_items ) {

    geo_entry_t new_entry;

    for ( auto id = map_entries.cbegin(); id != map_entries.cend(); id++ ) {

        new_entry.clear();

        _load_map_entry ( *id, new_entry );
        map_items.push_back ( new_entry );
    }
}

void geo_processor_t::_load_map_entry ( const uint32_t map_entry_offset, geo_entry_t& map_entry ) {

    geo_parser_t parser;

    char    ch     = 0;
    bool    eoc    = false;
    bool    eor    = false;

    m_map_file.seekg ( map_entry_offset, m_map_file.beg );

    while ( m_map_file.read(&ch, 1) ) {

        parser.load_param(ch, eoc, 0);
        if (!eoc) {
            continue;
        }

        parser.process_map ( map_entry, eor );

        if ( !eor ) {
            continue;
        }

        break;
    }

    map_entry.m_data_off = map_entry_offset;
}

void geo_processor_t::_win_coord ( const geo_coord_t& geo_pos, paint_coord_t& win_pos ) const {

    geo_coord_t geo_coord = geo_pos;

    geo_coord.x -= m_geo_rect.min.x;
    geo_coord.x /= m_step_x;
    geo_coord.x += 0.5;

    geo_coord.y -= m_geo_rect.min.y;
    geo_coord.y /= m_step_y;
    geo_coord.y += 0.5;

    win_pos.x = static_cast<int32_t> (geo_coord.x);
    win_pos.y = static_cast<int32_t> (geo_coord.y);
}

void geo_processor_t::_rotate_coord ( geo_coord_t& coord ) const {

    double translated_x = 0;
    double translated_y = 0;
    double rotated_x    = 0;
    double rotated_y    = 0;

    translated_x = coord.x - m_center.x;
    translated_y = coord.y - m_center.y;

    rotated_x = translated_x * m_angle_cos - translated_y * m_angle_sin;
    rotated_y = translated_x * m_angle_sin + translated_y * m_angle_cos;

    rotated_x += m_center.x;
    rotated_y += m_center.y;

    coord.x = rotated_x;
    coord.y = rotated_y;
}

void geo_processor_t::_process_window ( void ) {

    geo_coord_t     src_coord;
    paint_coord_t   dst_coord;

    for ( auto it_geo_area = m_draw_list.begin(); it_geo_area != m_draw_list.end(); it_geo_area++ ) {
        for ( auto it_geo_line = it_geo_area->m_lines.begin(); it_geo_line != it_geo_area->m_lines.end(); it_geo_line++ ) {

            it_geo_line->m_angle.clear ();
            it_geo_line->m_paint.clear ();
            it_geo_line->m_fill.clear ();

            for ( auto it_coord = it_geo_line->m_coords.cbegin(); it_coord != it_geo_line->m_coords.cend(); it_coord++ ) {

                src_coord = *it_coord;

                _rotate_coord ( src_coord );
                _win_coord ( src_coord, dst_coord );

                it_geo_line->m_paint.push_back( dst_coord );
            }

        }
    }

    return;
}

void geo_processor_t::_trim_record ( const vv_geo_coord_t& rect_path, const geo_entry_t& geo_path, geo_entry_t& out_record ) {

    vv_geo_coord_t trim_in;
    vv_geo_coord_t trim_out;
    geo_line_t     out_path;

    static int stop_cnt = 0;

    out_record.clear();

    out_record.m_record_type    = geo_path.m_record_type;
    out_record.m_default_type   = geo_path.m_default_type;
    out_record.m_osm_ref        = geo_path.m_osm_ref;
    out_record.m_data_off       = geo_path.m_data_off;

    trim_in.resize (1);

    for ( auto it_in_line = geo_path.m_lines.cbegin(); it_in_line != geo_path.m_lines.cend(); it_in_line++ ) {

        out_path.m_area = it_in_line->m_area;
        out_path.m_role = it_in_line->m_role;
        out_path.m_type = it_in_line->m_type;

        trim_in[0] = it_in_line->m_coords;

        trim_out = Clipper2Lib::Intersect ( rect_path, trim_in, Clipper2Lib::FillRule::NonZero, 13 );

        if ( trim_out.size() > 1 ) {
            stop_cnt++;
        }

        for ( auto it_out_line = trim_out.cbegin(); it_out_line != trim_out.cend(); it_out_line++ ) {
            out_path.m_coords = *it_out_line;
            out_path.m_coords.push_back ( out_path.m_coords.front() );
            out_record.m_lines.push_back ( out_path );
        }

    }

    return;
}

void geo_processor_t::_trim_map ( void ) {

    vv_geo_coord_t  trim_path;
    geo_entry_t     out_record;

    {   geo_coord_t     pt;
        geo_rect_t      trim_rect;
        v_geo_coord_t   trim_entry;
        trim_rect = m_geo_rect;
        pt.x = trim_rect.min.x;  pt.y = trim_rect.min.y;     trim_entry.push_back(pt);
        pt.x = trim_rect.max.x;  pt.y = trim_rect.min.y;     trim_entry.push_back(pt);
        pt.x = trim_rect.max.x;  pt.y = trim_rect.max.y;     trim_entry.push_back(pt);
        pt.x = trim_rect.min.x;  pt.y = trim_rect.max.y;     trim_entry.push_back(pt);
        trim_path.push_back(trim_entry);
    }

    m_draw_list.clear();

    for ( auto it_src = m_map_cache.cbegin(); it_src != m_map_cache.cend(); it_src++ ) {
        _trim_record ( trim_path, *it_src, out_record);
        if ( out_record.m_lines.size() > 0 ) {
            m_draw_list.push_back(out_record);
        }
    }
}

void geo_processor_t::_validate_window_rect ( void ) const {

    bool first_run = true;
    paint_rect_t rect;

    for (auto it = m_draw_list.cbegin(); it != m_draw_list.cend(); it++) {
        for (auto record = it->m_lines.cbegin(); record != it->m_lines.cend(); record++) {

            for ( auto line = 0; line < record->m_paint.size(); line++ ) {

                if ( first_run ) {
                    first_run = false;
                    rect.min.x = rect.max.x = record->m_paint[line].x;
                    rect.min.y = rect.max.y = record->m_paint[line].y;
                } else {
                    rect.min.x = std::min ( rect.min.x, record->m_paint[line].x );
                    rect.min.y = std::min ( rect.min.y, record->m_paint[line].y );
                    rect.max.x = std::max ( rect.max.x, record->m_paint[line].x );
                    rect.max.y = std::max ( rect.max.y, record->m_paint[line].y );
                }

            }
        }
    }

    (void)(rect);
}

void geo_processor_t::_get_rect ( const v_geo_coord_t& path, geo_rect_t& rect ) const {

    rect.clear();

    if ( path.size() == 0 ) {
        return;
    }

    rect.min.x = rect.max.x = path[0].x;
    rect.min.y = rect.max.y = path[0].y;

    for ( size_t i = 1; i < path.size(); i++ ) {
        rect.min.x = std::min ( rect.min.x, path[i].x );
        rect.min.y = std::min ( rect.min.y, path[i].y );
        rect.max.x = std::max ( rect.max.x, path[i].x );
        rect.max.y = std::max ( rect.max.y, path[i].y );
    }
}

bool geo_processor_t::_pt_in_poly ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const {

    size_t   i;
    size_t   j = polygon.size() - 2;
    bool     oddNodes = false;

    for ( i = 0; i < polygon.size()-1; i++ ) {
        if ( polygon[i].y < pt.y && polygon[j].y >= pt.y || polygon[j].y < pt.y && polygon[i].y >= pt.y ) {
            if ( polygon[i].x + (pt.y - polygon[i].y) / (polygon[j].y - polygon[i].y) * (polygon[j].x - polygon[i].x) < pt.x) {
                oddNodes = !oddNodes;
            }
        }
        j = i;
    }

    return oddNodes;
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

bool geo_processor_t::_pt_in_polygon ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const {

    size_t   i;
    size_t   j = polygon.size() - 2;

    bool oddNodes = false;

    for ( i = 0; i < polygon.size()-1; i++) {
        if ( polygon[i].y < pt.y && polygon[j].y >= pt.y || polygon[j].y < pt.y && polygon[i].y >= pt.y ) {
            if ( polygon[i].x + (pt.y - polygon[i].y) / (polygon[j].y - polygon[i].y) * (polygon[j].x - polygon[i].x) < pt.x) {
                oddNodes = !oddNodes;
            }
        }
        j = i;
    }

    return oddNodes;
}

bool geo_processor_t::_pt_in_polygon ( const v_paint_coord_t& polygon, const paint_coord_t& pt ) const {

    size_t   i;
    size_t   j = polygon.size() - 2;

    double   p1, p2, p3, p4;

    bool oddNodes = false;

    for (i = 0; i < polygon.size() - 1; i++) {
        if (polygon[i].y < pt.y && polygon[j].y >= pt.y || polygon[j].y < pt.y && polygon[i].y >= pt.y) {

            p1 = (pt.y - polygon[i].y);
            p2 = (polygon[j].y - polygon[i].y);
            p3 = (polygon[j].x - polygon[i].x);

            p4  = p1 / p2 * p3;
            p4 += polygon[i].x;

            if ( p4 < pt.x ) {
                oddNodes = !oddNodes;
            }
        }

        j = i;
    }

    return oddNodes;
}

void geo_processor_t::_map_color ( const obj_type_t& obj_type, geo_pixel_t& border_color, geo_pixel_t& fill_color ) const {

    int8_t shift = 20;

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
            GEO_RGB ( fill_color,   255, 115,  74,  10 );
            GEO_RGB ( border_color, 255,  95,  54,   0 );
            break;

        case OBJID_TYPE_WATER:        
            GEO_RGB ( fill_color,   255, 100, 100, 200 );
            GEO_RGB ( border_color, 255,   0,   0,   0 );   
            break;

        case OBJID_TYPE_GENERAL:      
            GEO_RGB ( fill_color,   255,  80, 120,  80 );
            GEO_RGB ( border_color, 255, 150, 150, 250 );
            break;

        case OBJID_TYPE_MOUNTAIN:     GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_STONE:        GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_SAND:         GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_UNDEFINED:    GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_FOOTWAY:      GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_ROAD:         GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_SECONDARY:    GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_TRUNK:        GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_MOTORWAY:     GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_PRIMARY:      GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_TERTIARY:     GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_RAILWAY:      GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_RIVER:        GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_BRIDGE:       GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_TUNNEL:       GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_RECORD_AREA:       GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_RECORD_BUILDING:   GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_RECORD_HIGHWAY:    GEO_RGB ( border_color, 0, 0, 0, 0 );   break;

        default:
            GEO_RGB ( border_color, 0, 0, 0, 0 );
            fill_color = border_color.Shift( shift );
            break;
    }

}

void geo_processor_t::_line ( const paint_coord_t from, const paint_coord_t to, const geo_pixel_t color ) {

    paint_coord_t p1;
    paint_coord_t p2;

    p1.x = (int) ( from.x + 0.5 );
    p1.y = (int) ( from.y + 0.5 );
    p2.x = (int) ( to.x   + 0.5 );
    p2.y = (int) ( to.y   + 0.5 );

    int   error1;
    int   error2;

    const int deltaX = static_cast<int> (abs(p2.x - p1.x));
    const int deltaY = static_cast<int> (abs(p2.y - p1.y));
    const int signX = (p1.x < p2.x) ? 1 : -1;
    const int signY = (p1.y < p2.y) ? 1 : -1;

    error1 = deltaX - deltaY;

    set_pix ( p2, color );

    while ( (p1.x != p2.x) || (p1.y != p2.y) ) {

        set_pix ( p1, color );

        error2 = error1 * 2;

        if ( error2 > -deltaY ) {
            error1 -= deltaY;
            p1.x   += signX;
        }

        if ( error2 < deltaX ) {
            error1 += deltaX;
            p1.y   += signY;
        }
    }
}

double geo_processor_t::_dist ( const paint_coord_t p1, const paint_coord_t p2 ) const {

    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;

    return std::sqrt(dx * dx + dy * dy);
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

//---------------------------------------------------------------------------//

