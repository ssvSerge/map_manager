
typedef std::vector<uint32_t>    video_buff_t;
video_buff_t            m_video_buffer;


void geo_processor_t::_clear_intersection ( void ) {

    m_intersection_map.clear();
}

void geo_processor_t::_add_intersection (const map_pos_t& base, const map_pos_t& pos, intersection_type_t type ) {

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

void geo_processor_t::_commit_intersection (const map_pos_t& base, size_t y, const geo_pixel_t& clr ) {

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

        px_pos.x = (int32_t)x + base.x - m_view_geo.min.ang.x;
        px_pos.y = (int32_t)y - m_view_geo.min.ang.y;

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

void geo_processor_t::_fill_poly_1 ( geo_line_t& poly_line, const geo_pixel_t border_clr, const geo_pixel_t fill_clr ) {

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

void geo_processor_t::_fill_poly_2 ( const geo_coord_t& pos, const geo_pixel_t br_clr, const geo_pixel_t fill_clr, const bool ignore_bk ) {


    std::queue<geo_coord_t> queue;
    geo_coord_t     p;
    geo_coord_t     next;
    geo_pixel_t     clr;

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
}

