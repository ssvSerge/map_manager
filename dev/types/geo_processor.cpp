#include <cassert>
#include <fstream>
#include <algorithm>

#include <GeographicLib/Geodesic.hpp>

#include <geo_processor.h>

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

    memset(m_video_buffer, 0, alloc_size);
}

void geo_processor_t::get_pix ( const geo_pos_t& pos, geo_pixel_t& px ) const {

    int32_t offset;
    geo_pixel_int_t tmp;

    assert(pos.x < m_video_width);
    assert(pos.y < m_video_height);

    offset = (pos.y * m_video_width) + pos.x;
    tmp = m_video_buffer[offset];

    _px_conv(tmp, px);
}

void geo_processor_t::set_pix ( const geo_pos_t& pos, const geo_pixel_t& px ) {

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

void geo_processor_t::set_base_params ( const geo_coord_t center, const double scale, const window_rect_t wnd ) {

    v_uint32_t  rects_list;
    v_uint32_t  map_entrie;

    _set_scales   ( center, scale, wnd );
    _filter_rects ( m_map_idx, m_geo_rect, rects_list );
    _merge_idx    ( m_map_idx, rects_list, map_entrie );
    _load_map     ( map_entrie, m_base_list );
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

//---------------------------------------------------------------------------//

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

void geo_processor_t::_load_idx ( vector_geo_idx_rec_t& idx_list ) {

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

void geo_processor_t::_find_base_rect ( const vector_geo_idx_rec_t& map_idx, geo_rect_t& map_rect ) {

    for ( size_t i = 0; i < map_idx.size(); i++ ) {
    
        if ( i == 0 ) {
            map_rect = map_idx[i].m_rect;
        } else {
            map_rect.min_lat = std::min ( map_rect.min_lat, map_idx[i].m_rect.min_lat );
            map_rect.min_lon = std::min ( map_rect.min_lon, map_idx[i].m_rect.min_lon );
            map_rect.max_lat = std::max ( map_rect.max_lat, map_idx[i].m_rect.max_lat );
            map_rect.max_lon = std::max ( map_rect.max_lon, map_idx[i].m_rect.max_lon );
        }

    }
}

void geo_processor_t::_set_scales ( const geo_coord_t center, double scale, const window_rect_t wnd ) {

    const double geo_delta = 0.0001;

    double  shift_hor = 0;
    double  shift_ver = 0;
    double  geo_hor;
    double  geo_ver;

    m_scale    = scale;
    m_center   = center;

    m_geo_rect.min_lat = m_geo_rect.max_lat = center.y;
    m_geo_rect.min_lon = m_geo_rect.max_lon = center.x;

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

    m_geo_rect.min_lat = center.y - (geo_ver / 2);
    m_geo_rect.max_lat = center.y + (geo_ver / 2);

    m_geo_rect.min_lon = center.x - (geo_hor / 2);
    m_geo_rect.max_lon = center.x + (geo_hor / 2);


#if 1
    geod.Inverse ( 
        m_geo_rect.min_lat, m_geo_rect.min_lon,
        m_geo_rect.min_lat, m_geo_rect.max_lon,
        shift_hor
    );
    shift_hor /= scale;

    geod.Inverse (
        m_geo_rect.min_lat, m_geo_rect.min_lon,
        m_geo_rect.max_lat, m_geo_rect.min_lon,
        shift_ver
    );
    shift_ver /= scale;
#endif

}

void geo_processor_t::_filter_rects ( const vector_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, v_uint32_t& out_list ) {

    for ( uint32_t i = 0; i < rect_list.size(); i++ ) {
        if ( _is_overlapped(base_rect, rect_list[i].m_rect) ) {
            out_list.push_back(i);
        }
    }
}

bool geo_processor_t::_is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice ) {

    if (window.min_lon > slice.max_lon) {
        return false;
    }
    if (window.max_lon < slice.min_lon) {
        return false;
    }

    if (window.min_lat > slice.max_lat) {
        return false;
    }
    if (window.max_lat < slice.min_lat) {
        return false;
    }

    return true;
}

void geo_processor_t::_merge_idx ( const vector_geo_idx_rec_t& rect_list, const v_uint32_t& in_list, v_uint32_t& map_entries ) {

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

    pt.x = trim_rect.min_lon;  pt.y = trim_rect.min_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.max_lon;  pt.y = trim_rect.min_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.max_lon;  pt.y = trim_rect.max_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.min_lon;  pt.y = trim_rect.max_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.min_lon;  pt.y = trim_rect.min_lat;     window_entry.push_back(pt);
    window_path.push_back(window_entry);

    dst_record.m_geo_type = src_record.m_geo_type;
    dst_record.m_prime_type = src_record.m_prime_type;
    dst_record.m_prime_off = src_record.m_prime_off;
    dst_record.m_record_id = src_record.m_record_id;
    dst_record.m_osm_ref = src_record.m_osm_ref;

    trim_in.resize(1);

    for (size_t i = 0; i < src_record.m_child_lines.size(); i++) {

        trim_in[0] = src_record.m_child_lines[i];

        trim_out = Clipper2Lib::Intersect(trim_in, window_path, Clipper2Lib::FillRule::NonZero, 13);

        if (trim_out.size() > 1) {
            stop_cnt++;
        }

        for (size_t j = 0; j < trim_out.size(); j++) {

            dst_record.m_child_roles.push_back(src_record.m_child_roles[i]);
            dst_record.m_child_types.push_back(src_record.m_child_types[i]);
            dst_record.m_child_areas.push_back(src_record.m_child_areas[i]);

            trim_out[j].push_back(trim_out[j][0]);

            dst_record.m_child_lines.push_back(trim_out[j]);
        }

    }

    if (dst_record.m_child_lines.size() > 0) {
        trim_res = true;
    }

    return;
}

void geo_processor_t::_geo_to_window ( void ) {

    geo_coord_t tmp;

    for ( auto it = m_draw_list.begin(); it != m_draw_list.end(); it++ ) {

        it->m_child_wnd_lines.resize ( it->m_child_lines.size() );

        for ( size_t id=0; id < it->m_child_lines.size(); id++ ) {

            it->m_child_wnd_lines[id].resize ( it->m_child_lines[id].size());

            for ( size_t coord = 0; coord < it->m_child_lines[id].size(); coord++ ) {

                tmp = it->m_child_lines[id][coord];

                tmp.x -= m_geo_rect.min_lon;
                tmp.x /= m_step_x;
                tmp.x += 0.5;
                it->m_child_wnd_lines[id][coord].x = static_cast<int32_t>(tmp.x);

                tmp.y -= m_geo_rect.min_lat;
                tmp.y /= m_step_y;
                tmp.y += 0.5;
                it->m_child_wnd_lines[id][coord].y = static_cast<int32_t>(tmp.y);
            }
        }
    }
}

void geo_processor_t::_trim_map ( void ) {

    geo_record_t out_record;
    bool trim_res;

    m_draw_list.clear();

    auto src_it = m_base_list.cbegin();
    while (src_it != m_base_list.cend()) {
        _trim_record(m_geo_rect, *src_it, out_record, trim_res);
        if (trim_res) {
            m_draw_list.push_back(out_record);
        }
        src_it++;
    }
}

void geo_processor_t::_validate_window_rect ( void ) {

    bool    first_entry = true;
    int32_t min_x, min_y, max_x, max_y;

    for (auto it = m_draw_list.cbegin(); it != m_draw_list.cend(); it++) {
        for (auto line = it->m_child_wnd_lines.cbegin(); line != it->m_child_wnd_lines.cend(); line++) {
            for (auto coord = line->cbegin(); coord != line->cend(); coord++) {
                if (first_entry) {
                    first_entry   = false;
                    min_x = max_x = coord->x;
                    min_y = max_y = coord->y;
                } else {
                    min_x = std::min(min_x, coord->x );
                    min_y = std::min(min_y, coord->y );
                    max_x = std::max(max_x, coord->x );
                    max_y = std::max(max_y, coord->y );
                }
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

    for ( auto record = m_base_list.begin(); record != m_base_list.end(); record++ ) {
        for ( auto line = record->m_child_lines.begin(); line != record->m_child_lines.end(); line++ ) {
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

    for (auto it = m_draw_list.begin(); it != m_draw_list.end(); it++) {
        for (size_t id = 0; id < it->m_child_lines.size(); id++) {

        }
    }
    return;
}

bool geo_processor_t::_pt_in_poly( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const {

    return false;
}

//---------------------------------------------------------------------------//

