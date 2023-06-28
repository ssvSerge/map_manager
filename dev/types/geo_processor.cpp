#include <cassert>
#include <fstream>
#include <algorithm>
#include <stack>

#include <GeographicLib/Geodesic.hpp>

#include <geo_processor.h>

//---------------------------------------------------------------------------//

#define GEO_RGB(var,in_a,in_r,in_g,in_b)     { var.r=in_r; var.g=in_g; var.b=in_b; }

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
}

//---------------------------------------------------------------------------//

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

    m_video_buffer = new geo_pixel_int_t [alloc_size];

    assert(m_video_buffer != nullptr);

    memset ( m_video_buffer, 0, alloc_size * sizeof(geo_pixel_int_t) );
}

void geo_processor_t::get_pix ( const paint_pos_t& pos, geo_pixel_t& px ) const {

    int32_t offset;
    geo_pixel_int_t tmp;

    assert(pos.x <= m_video_width);
    assert(pos.y <= m_video_height);

    offset = (pos.y * m_video_width) + pos.x;
    tmp = m_video_buffer[offset];

    _px_conv(tmp, px);
}

void geo_processor_t::set_pix ( const paint_pos_t& pos, const geo_pixel_t& px ) {

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
    _rotate_map(angle);
}

void geo_processor_t::trim_map (void) {

    _trim_map();
    _geo_to_window();
    _find_paint_locations();
    _validate_window_rect();
}

void geo_processor_t::process_wnd ( void ) {

    size_t cnt;
    geo_pixel_t border_color;
    geo_pixel_t fill_color;

    static int draw_cnt = 0;
    int out_cnt = 0;

    for ( auto it = m_draw_list.cbegin(); it != m_draw_list.cend(); it++ ) {

        cnt = it->m_wnd_lines.size();

        for ( size_t i = 0; i < cnt; i++ ) {

            if ( it->m_geo_type != OBJID_RECORD_AREA) {
                continue;
            }

            if ( out_cnt == draw_cnt ) {
                _map_color ( it->m_geo_ctx.m_types[i], border_color, fill_color );
                fill_color.r = 248;  fill_color.g = 0; fill_color.b = 0;
                _poly_line ( it->m_wnd_lines[i], border_color );
                _fill_poly ( it->m_wnd_lines[i], border_color, fill_color );
            }

            out_cnt++;
        }

    }

    draw_cnt++;
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

void geo_processor_t::_fill_poly ( const v_paint_coord_t& poly_line, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr ) {

    size_t        start_pos;
    paint_pos_t   base_pos;
    paint_rect_t  base_rect;

    if (poly_line.size() < 3) {
        return;
    }

    start_pos = 0;
    base_pos  = poly_line[0];
    base_rect.min = base_rect.max = base_pos;

    for ( size_t i = 1; i < poly_line.size()-1; i++ ) {

        if ( poly_line[i].x < base_pos.x ) {
            start_pos = i;
            base_pos  = poly_line[i];
        } else 
        if ( poly_line[i].x == base_pos.x ) {
            if ( poly_line[i].y < base_pos.y ) {
                start_pos = i;
                base_pos  = poly_line[i];
            }
        }

        base_rect.min.x = std::min ( base_rect.min.x, poly_line[i].x );
        base_rect.min.y = std::min ( base_rect.min.y, poly_line[i].y );
        base_rect.max.x = std::max ( base_rect.max.x, poly_line[i].x );
        base_rect.max.y = std::max ( base_rect.max.y, poly_line[i].y );
    }

    paint_pos_t p1, p2, p3; // int

    p1 = poly_line[0];
    p2 = poly_line[1];

    p3.x  = p1.x + (p2.x - p1.x) / 2;
    p3.y  = p1.y + (p2.y - p1.y) / 2;
    p3.y += 2;

    if ( ! _pt_in_poly(poly_line, p3) ) {
        set_pix ( p3, geo_pixel_t(0, 255, 0) );
        return;
    }

    _paint_region ( p3, fill_clr, bk_clr );
}

void geo_processor_t::_px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const {
            
    uint16_t val = 0;

    val  |= (from.r >> 3);
    val <<= 6;

    val  |= (from.g >> 2);
    val <<= 5;

    val |= (from.b >> 3);

    to = val;
}

void geo_processor_t::_px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const {

    uint16_t tmp = from;

    to.b  = tmp & 0x1F;
    tmp >>= 5;

    to.g  = tmp & 0x3F;
    tmp >>= 6;

    to.r = tmp & 0x1F;

    to.r <<= 3;
    to.g <<= 2;
    to.b <<= 3;
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

    for (size_t i = 0; i < in_list.size(); i++) {
        sub_idx = in_list[i];
        for (size_t y = 0; y < rect_list[sub_idx].m_list_off.size(); y++) {
            tmp_map.insert(rect_list[sub_idx].m_list_off[y]);
        }
    }

    for (auto it = tmp_map.cbegin(); it != tmp_map.cend(); it++) {
        map_entries.push_back(*it);
    }

    std::sort(map_entries.begin(), map_entries.end());
}

void geo_processor_t::_load_map ( const v_uint32_t& map_entries, l_geo_record_t& map_items ) {

    geo_record_t map_entry;

    for ( auto id = map_entries.cbegin(); id != map_entries.cend(); id++ ) {

        auto find_pos = std::find ( map_items.cbegin(), map_items.cend(), *id );
        if ( find_pos != map_items.end() ) {
            continue;
        }

        _load_map_entry ( *id, map_entry );
        map_items.push_back ( map_entry );
    }

}

void geo_processor_t::_load_map_entry ( const uint32_t map_entry_offset, geo_record_t& map_entry ) {

    geo_parser_t parser;

    char    ch;
    bool    eoc;
    bool    eor;

    map_entry.entry_offset = map_entry_offset;

    m_map_file.seekg ( map_entry_offset, m_map_file.beg );

    while ( m_map_file.read(&ch, 1) ) {

        parser.load_param(ch, eoc, 0);
        if (!eoc) {
            continue;
        }

        parser.process_map ( map_entry, eor);

        if (!eor) {
            continue;
        }

        break;
    }
}

void geo_processor_t::_trim_record ( const geo_rect_t& window_rect, const geo_record_t& src_record, geo_record_t& dst_record, bool& trim_res ) {

    geo_rect_t          trim_rect;

    geo_coord_t         pt;
    vv_geo_coord_t      trim_in;
    vv_geo_coord_t      trim_out;
    v_geo_coord_t       window_entry;
    vv_geo_coord_t      window_path;

    static int          stop_cnt = 0;
    stop_cnt++;

    trim_res = false;

    dst_record.clear();

    trim_rect = window_rect;

    pt.x = trim_rect.min.x;  pt.y = trim_rect.min.y;     window_entry.push_back(pt);
    pt.x = trim_rect.max.x;  pt.y = trim_rect.min.y;     window_entry.push_back(pt);
    pt.x = trim_rect.max.x;  pt.y = trim_rect.max.y;     window_entry.push_back(pt);
    pt.x = trim_rect.min.x;  pt.y = trim_rect.max.y;     window_entry.push_back(pt);
    pt.x = trim_rect.min.x;  pt.y = trim_rect.min.y;     window_entry.push_back(pt);

    window_path.push_back ( window_entry );

    dst_record.m_geo_type    = src_record.m_geo_type;
    dst_record.m_prime_type  = src_record.m_prime_type;
    dst_record.m_prime_off   = src_record.m_prime_off;
    dst_record.m_record_id   = src_record.m_record_id;
    dst_record.m_osm_ref     = src_record.m_osm_ref;

    trim_in.resize ( 1 );

    for ( size_t i = 0; i < src_record.m_geo_lines.size(); i++ ) {

        trim_in[0] = src_record.m_geo_lines[i];

        trim_out = Clipper2Lib::Intersect ( trim_in, window_path, Clipper2Lib::FillRule::NonZero, 13 );

        if ( trim_out.size() > 1 ) {
            stop_cnt++;
        }

        for (size_t j = 0; j < trim_out.size(); j++) {

            dst_record.m_geo_ctx.m_roles.push_back ( src_record.m_geo_ctx.m_roles[i] );
            dst_record.m_geo_ctx.m_types.push_back ( src_record.m_geo_ctx.m_types[i] );
            dst_record.m_geo_ctx.m_areas.push_back ( src_record.m_geo_ctx.m_areas[i] );

            trim_out[j].push_back ( trim_out[j][0] );

            dst_record.m_geo_lines.push_back ( trim_out[j] );
        }

    }

    if ( dst_record.m_geo_lines.size() > 0 ) {
        trim_res = true;
    }

    return;
}

void geo_processor_t::_geo_to_window ( void ) {

    geo_coord_t tmp;

    for ( auto it = m_draw_list.begin(); it != m_draw_list.end(); it++ ) {

        it->m_wnd_lines.resize ( it->m_geo_lines.size() );

        for ( size_t id=0; id < it->m_geo_lines.size(); id++ ) {

            it->m_wnd_lines[id].resize ( it->m_geo_lines[id].size());

            for ( size_t coord = 0; coord < it->m_geo_lines[id].size(); coord++ ) {

                tmp = it->m_geo_lines[id] [coord];

                tmp.x -= m_geo_rect.min.x;
                tmp.x /= m_step_x;
                tmp.x += 0.5;
                it->m_wnd_lines [id] [coord].x = static_cast<int32_t>(tmp.x);

                tmp.y -= m_geo_rect.min.y;
                tmp.y /= m_step_y;
                tmp.y += 0.5;
                it->m_wnd_lines [id] [coord].y = static_cast<int32_t>(tmp.y);
            }
        }
    }
}

void geo_processor_t::_trim_map ( void ) {

    geo_record_t out_record;
    bool trim_res;

    m_draw_list.clear();

    auto src_it = m_map_cache.cbegin();
    while ( src_it != m_map_cache.cend() ) {

        _trim_record ( m_geo_rect, *src_it, out_record, trim_res );
        if ( trim_res ) {
            m_draw_list.push_back ( out_record );
        }

        src_it++;
    }
}

void geo_processor_t::_validate_window_rect ( void ) const {

    bool first_run = true;
    paint_rect_t rect;

    for (auto it = m_draw_list.cbegin(); it != m_draw_list.cend(); it++) {
        for (auto line = it->m_wnd_lines.cbegin(); line != it->m_wnd_lines.cend(); line++) {

            if ( first_run ) {
                first_run = false;
                rect.min.x = rect.max.x = line->cbegin()->x;
                rect.min.y = rect.max.y = line->cbegin()->y;
            }

            for (auto coord = line->cbegin(); coord != line->cend(); coord++) {
                rect.min.x = std::min ( rect.min.x, coord->x );
                rect.min.y = std::min ( rect.min.y, coord->y );
                rect.max.x = std::max ( rect.max.x, coord->x );
                rect.max.y = std::max ( rect.max.y, coord->y );
            }

        }
    }

}

void geo_processor_t::_rotate_map ( const double angle ) {

    double pi_rad_scale = 0.01745329251994329576923690768489;
    double sin_rotated  = 0;
    double cos_rotated  = 0;
    double translated_x = 0;
    double translated_y = 0;
    double rotated_x    = 0;
    double rotated_y    = 0;

    double delta_angle;

    if ( m_angle == angle ) {
        return;
    }

    delta_angle = angle - m_angle;
    m_angle = angle;

    sin_rotated = sin ( delta_angle * pi_rad_scale );
    cos_rotated = cos ( delta_angle * pi_rad_scale );

    m_angle = angle;

    for ( auto record = m_map_cache.begin(); record != m_map_cache.end(); record++ ) {
        for ( auto line = record->m_geo_lines.begin(); line != record->m_geo_lines.end(); line++ ) {
            for ( auto coord = line->begin(); coord != line->end(); coord++ ) {

                translated_x = coord->x - m_center.x;
                translated_y = coord->y - m_center.y;

                rotated_x = translated_x * cos_rotated - translated_y * sin_rotated;
                rotated_y = translated_x * sin_rotated + translated_y * cos_rotated;

                rotated_x += m_center.x;
                rotated_y += m_center.y;

                coord->x = rotated_x;
                coord->y = rotated_y;

            }
        }
    }
}

void geo_processor_t::_find_paint_locations ( void ) {

    geo_rect_t rect;
    v_geo_coord_t coords_list;

    for ( auto it = m_draw_list.begin(); it != m_draw_list.end(); it++ ) {

        it->m_geo_ctx.m_fill_pt.resize ( it->m_geo_lines.size() );

        for ( size_t id = 0; id < it->m_geo_lines.size(); id++ ) {
            _get_rect ( it->m_geo_lines[id], rect );
            _generate_paint_points ( it->m_geo_lines[id], coords_list );
            _check_paint_points ( it->m_geo_lines[id], coords_list, it->m_geo_ctx.m_fill_pt[id] );
        }
    }

    return;
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

bool geo_processor_t::_pt_in_poly ( const v_paint_coord_t& polygon, const paint_pos_t& pt ) const {

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

void geo_processor_t::_generate_paint_points ( const v_geo_coord_t& polygon, v_geo_coord_t& coords_list ) const {

    geo_coord_t min;
    geo_coord_t max;
    geo_coord_t mid;

    coords_list.clear();

    for ( size_t i = 1; i < polygon.size(); i++ ) {

        min.x = std::min ( polygon[i - 1].x, polygon[i - 0].x );
        max.x = std::max ( polygon[i - 1].x, polygon[i - 0].x );

        min.y = std::min ( polygon[i - 1].y, polygon[i - 0].y );
        max.y = std::max ( polygon[i - 1].y, polygon[i - 0].y );

        mid.x = min.x + (max.x - min.x) / 2;
        mid.y = min.y + (max.y - min.y) / 2;
    }
}

bool geo_processor_t::_check_paint_points ( const v_geo_coord_t& polygon, const v_geo_coord_t& coords_list, geo_coord_t& pt ) const {

    bool pt_in_polygon = false;

    for ( size_t i = 0; i < coords_list.size(); i++ ) {

        pt_in_polygon = _test_position ( polygon, coords_list[i] );

        if ( pt_in_polygon ) {
            pt = coords_list[i];
            break;
        }

    }

    return pt_in_polygon;
}

bool geo_processor_t::_test_position ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const {

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

void geo_processor_t::_map_color ( const obj_type_t& obj_type, geo_pixel_t& border_color, geo_pixel_t& fill_color ) const {

    int8_t shift = 20;

    switch ( obj_type ) {
        //----------------------------------------------//
        //                          A    R    G    B    //
        //----------------------------------------------//
        case OBJID_TYPE_FOREST:       
            GEO_RGB ( border_color, 255,   0,  80,   0 ); 
            fill_color = border_color.Shift( shift );
            break;

        case OBJID_TYPE_GRASS:        
            GEO_RGB ( border_color, 255,   0, 180,   0 ); 
            fill_color = border_color.Shift( shift );
            break;

        case OBJID_TYPE_ASPHALT:      
            GEO_RGB ( border_color, 255,  80,  80,  80 ); 
            fill_color = border_color.Shift( shift );
            break;

        case OBJID_TYPE_BUILDING:     
            GEO_RGB ( border_color, 255,  60, 80,  100 ); 
            fill_color = border_color.Shift( shift );
            break;

        case OBJID_TYPE_WATER:        GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
        case OBJID_TYPE_GENERAL:      GEO_RGB ( border_color, 0, 0, 0, 0 );   break;
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
            GEO_RGB(border_color, 0, 0, 0, 0);
            fill_color = border_color.Shift( shift );
            break;
    }}

void geo_processor_t::_line ( const paint_pos_t from, const paint_pos_t to, const geo_pixel_t color ) {

    paint_pos_t p1;
    paint_pos_t p2;

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

        if (error2 > -deltaY) {
            error1 -= deltaY;
            p1.x += signX;
        }

        if (error2 < deltaX) {
            error1 += deltaX;
            p1.y += signY;
        }
    }
}

void geo_processor_t::_paint_region ( const paint_pos_t pos, const geo_pixel_t fill_clr, const geo_pixel_t bk_clr ) {

    std::stack<paint_pos_t>     paint_list;
    paint_pos_t                 p;
    geo_pixel_t                 px;
    int                         px_cnt = 0;

    p.x = pos.x;
    p.y = pos.y;

    paint_list.push( p );

    while ( paint_list.size() ) {

        p = paint_list.top();
        paint_list.pop();

        get_pix ( p, px );

        if ( px == bk_clr ) {
            continue;
        }
        if ( px == fill_clr ) {
            continue;
        }

        set_pix ( p, fill_clr );

        px_cnt++;
        if ( px_cnt > 5 ) {
            // break;
        }

        paint_list.push ( paint_pos_t(p.x + 1, p.y) );
        paint_list.push ( paint_pos_t(p.x - 1, p.y) );
        paint_list.push ( paint_pos_t(p.x, p.y + 1) );
        paint_list.push ( paint_pos_t(p.x, p.y - 1) );
    }
}

//---------------------------------------------------------------------------//

