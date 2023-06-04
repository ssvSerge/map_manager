#include "pch.h"

#ifdef min
    #undef min
#endif
#ifdef max
    #undef max
#endif

#include <fstream>
#include <string>
#include <algorithm>

#include "MapPainter.h"

#include <GeographicLib/Geodesic.hpp>

#include <idx_file.h>

IMPLEMENT_DYNAMIC ( CMapPainter, CStatic )

BEGIN_MESSAGE_MAP ( CMapPainter, CStatic )
    ON_WM_PAINT()
    ON_WM_MOUSEHOVER()
    ON_WM_MOUSELEAVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

static    vector_geo_idx_rec_t   g_idx_list;
static    geo_rect_t             g_base_rect;
static    list_geo_record_t      g_map_list;
static    list_geo_record_t      g_draw_list;


static std::string      g_idx_name;
static std::string      g_map_name;


CMapPainter::CMapPainter () {

    m_DragActive     = false;
    m_bMouseTracking = false;
    m_BasePosition.x = 200;
    m_BasePosition.y = 200;
    m_DeltaX	     = 0;
    m_DeltaY	     = 0;
    m_lon		     = 0;
    m_lat		     = 0;
    m_scale		     = 0;
    m_delta_lat      = 0;
    m_delta_lon      = 0;

    g_idx_name = "C:\\GitHub\\map_manager\\dev\\_bin\\prague_idx.txt";
    g_map_name = "C:\\GitHub\\map_manager\\dev\\_bin\\prague_map.txt";
}

CMapPainter::~CMapPainter () {
}

void CMapPainter::_load_idx ( vector_geo_idx_rec_t& idx_list ) {

    geo_parser_t    parser;
    std::ifstream   idx_file;
    geo_idx_rec_t   geo_idx;
    geo_offset_t    file_offset = 0;
    char            ch;
    bool            eoc;
    bool            eor;

    idx_file.open(g_idx_name, std::ios_base::binary);

    while (idx_file.read(&ch, 1)) {

        parser.load_param(ch, eoc, file_offset);
        file_offset++;

        if (!eoc) {
            continue;
        }

        parser.process_idx ( geo_idx, eor );

        if (!eor) {
            continue;
        }

        idx_list.push_back(geo_idx);
    }
}

bool CMapPainter::_is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice ) {

    if ( window.min_lon > slice.max_lon ) {
        return false;
    }
    if ( window.max_lon < slice.min_lon ) {
        return false;
    }

    if ( window.min_lat > slice.max_lat ) {
        return false;
    }

    if ( window.max_lat < slice.min_lat ) {
        return false;
    }

    return true;
}

void CMapPainter::_find_rects ( const vector_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, vector_uint_t& out_list ) {

    for (uint32_t i = 0; i < rect_list.size(); i++) {
        if ( _is_overlapped(base_rect, rect_list[i].m_rect ) ) {
            out_list.push_back(i);
        }
    }
}

void CMapPainter::_merge ( const vector_geo_idx_rec_t& rect_list, const vector_uint_t& in_list, vector_uint_t& map_entries ) {

    set_offset_t tmp_map;
    size_t sub_idx;

    map_entries.clear();

    for ( size_t i = 0; i < in_list.size(); i++ ) {
        sub_idx = in_list[i];
        for (size_t y = 0; y < rect_list[sub_idx].m_list_off.size(); y++) {
            tmp_map.insert(rect_list[sub_idx].m_list_off[y] );
        }
    }

    for (auto it = tmp_map.cbegin(); it != tmp_map.cend(); it++) {
        map_entries.push_back( *it );
    }

    std::sort ( map_entries.begin(), map_entries.end() );
}

void CMapPainter::_load_map ( const vector_uint_t& map_entries, list_geo_record_t& map_items ) {

    geo_parser_t    parser;
    geo_record_t    map_entry;
    std::ifstream   map_file;
    char            ch;
    bool            eoc;
    bool            eor;

    map_file.open ( g_map_name, std::ios_base::binary );

    for ( auto it = map_entries.cbegin(); it != map_entries.cend(); it++ ) {

        map_file.seekg( *it, map_file.beg );

        while ( map_file.read(&ch, 1) ) {

            parser.load_param ( ch, eoc, 0 );
            if ( !eoc ) {
                continue;
            }

            parser.process_map ( map_entry, eor );

            if ( !eor ) {
                continue;
            }

            map_items.push_back ( map_entry );

            break;
        }

    }

}

void CMapPainter::_ajust_minus ( const geo_rect_t& base_rect, double scale_x, double scale_y, v_geo_coord_t& path ) {

    (void)(base_rect);
    (void)(scale_x);
    (void)(scale_y);
    (void)(path);

    #if 0

        for ( size_t j = 0; j < path.size(); j++ ) {

            path[j].x -= base_rect.min_lon;
            path[j].x /= scale_x;
            path[j].x += 0.5;
            path[j].x = (int32_t)path[j].x;

            path[j].y -= base_rect.min_lat;
            path[j].y /= scale_y;
            path[j].y += 0.5;
            path[j].y = (int32_t)path[j].y;

        }

    #endif
}

void CMapPainter::_ajust_plus ( const geo_rect_t& base_rect, double scale_x, double scale_y, v_geo_coord_t& path ) {

    (void)(base_rect);
    (void)(scale_x);
    (void)(scale_y);
    (void)(path);

    #if 0

        for (size_t j = 0; j < path.size(); j++) {
            path[j].x += base_rect.min_lon;
            path[j].y += base_rect.min_lat;
        }

    #endif
}

void CMapPainter::_trim_record ( const geo_rect_t& base_rect, const geo_rect_t& window_rect, const geo_record_t& src_record, geo_record_t& dst_record, bool& trim_res ) {

    geo_rect_t          trim_rect;

    geo_coord_t         pt;
    vv_geo_coords_t     trim_in;
    vv_geo_coords_t     trim_out;
    v_geo_coord_t       window_entry;
    vv_geo_coords_t     window_path;

    static int          stop_cnt = 0;
    stop_cnt++;

    (void) (base_rect);

    trim_res = false;

    dst_record.clear();

    trim_rect = window_rect;

    pt.x = trim_rect.min_lon;  pt.y = trim_rect.min_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.max_lon;  pt.y = trim_rect.min_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.max_lon;  pt.y = trim_rect.max_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.min_lon;  pt.y = trim_rect.max_lat;     window_entry.push_back(pt);
    pt.x = trim_rect.min_lon;  pt.y = trim_rect.min_lat;     window_entry.push_back(pt);
    window_path.push_back(window_entry);

    dst_record.m_geo_type   = src_record.m_geo_type;
    dst_record.m_prime_type = src_record.m_prime_type;
    dst_record.m_prime_off  = src_record.m_prime_off;
    dst_record.m_record_id  = src_record.m_record_id;

    trim_in.resize ( 1 );

    for ( size_t i = 0; i < src_record.m_child_lines.size(); i++ ) {

        trim_in[0] = src_record.m_child_lines[i];

        trim_out = Clipper2Lib::Intersect ( trim_in, window_path, Clipper2Lib::FillRule::NonZero, 13 );

        if ( trim_out.size() > 1 ) {
            stop_cnt ++;
        }

        for (size_t j = 0; j < trim_out.size(); j++) {
            
            dst_record.m_child_roles.push_back ( src_record.m_child_roles[i] );
            dst_record.m_child_types.push_back ( src_record.m_child_types[i] );
            dst_record.m_child_areas.push_back ( src_record.m_child_areas[i] );

            trim_out[j].push_back( trim_out[j][0] );

            dst_record.m_child_lines.push_back ( trim_out[j] );
        }

    }

    if ( dst_record.m_child_lines.size() > 0 ) {
        trim_res = true;
    }

    return;
}

void CMapPainter::_trim_map ( const geo_rect_t& base_rect, const geo_rect_t& window_rect, const list_geo_record_t& src_map, list_geo_record_t& dst_map ) {

    geo_record_t out_record;
    bool trim_res;

    dst_map.clear();

    auto src_it = src_map.cbegin();
    while ( src_it != src_map.cend() ) {
        _trim_record ( base_rect, window_rect, *src_it, out_record, trim_res );
        if ( trim_res ) {
            dst_map.push_back(out_record);
        }
        src_it++;
    }

}

void CMapPainter::_find_base_rect ( const vector_geo_idx_rec_t& idx_list, geo_rect_t& base_rect ) {

    for ( size_t i = 0; i < idx_list.size(); i++ ) {
    
        if ( i == 0 ) {
            base_rect = idx_list[i].m_rect;
        } else {

            base_rect.min_lat = std::min ( base_rect.min_lat, idx_list[i].m_rect.min_lat );
            base_rect.min_lon = std::min ( base_rect.min_lon, idx_list[i].m_rect.min_lon );
            base_rect.max_lat = std::max ( base_rect.max_lat, idx_list[i].m_rect.max_lat );
            base_rect.max_lon = std::max ( base_rect.max_lon, idx_list[i].m_rect.max_lon );

        }
    }
}

void CMapPainter::_geo_to_window ( const geo_rect_t& window_rect, double scale_x, double scale_y, list_geo_record_t& list_rec ) {

    for ( auto it = list_rec.begin(); it != list_rec.end(); it++ ) {
        for ( auto line = it->m_child_lines.begin(); line != it->m_child_lines.end(); line++ ) {
            for ( auto coord = line->begin(); coord != line->end(); coord++ ) {

                coord->x -= window_rect.min_lon;
                coord->x /= scale_x;
                coord->x += 0.5;
                coord->x  = static_cast<int32_t>(coord->x);

                coord->y -= window_rect.min_lat;
                coord->y /= scale_y;
                coord->y += 0.5;
                coord->y  = static_cast<int32_t>(coord->y);

            }
        }
    }
}

void CMapPainter::_test_range ( const list_geo_record_t& list_rec ) {

    bool first_entry = true;
    int32_t   min_x, min_y, max_x, max_y;

    for (auto it = list_rec.cbegin(); it != list_rec.cend(); it++) {
        for (auto line = it->m_child_lines.cbegin(); line != it->m_child_lines.cend(); line++) {
            for (auto coord = line->cbegin(); coord != line->cend(); coord++) {

                if (first_entry) {
                    first_entry = false;
                    min_x = max_x = static_cast<int32_t> (coord->x);
                    min_y = max_y = static_cast<int32_t> (coord->y);
                } else {
                    min_x = std::min ( min_x, static_cast<int32_t>(coord->x) );
                    min_y = std::min ( min_y, static_cast<int32_t>(coord->y) );
                    max_x = std::max ( max_x, static_cast<int32_t>(coord->x) );
                    max_y = std::max ( max_y, static_cast<int32_t>(coord->y) );
                }
            }
        }
    }
}

void CMapPainter::test ( const geo_rect_t& draw_rect ) {

    vector_uint_t  rects_list;
    vector_uint_t  map_entrie;

    _load_idx       ( g_idx_list );
    _find_base_rect ( g_idx_list,   g_base_rect );
    _find_rects     ( g_idx_list,   draw_rect, rects_list );
    _merge          ( g_idx_list,   rects_list, map_entrie );
    _load_map       ( map_entrie,   g_map_list );
    _trim_map       ( g_base_rect,  draw_rect, g_map_list, g_draw_list );
    _geo_to_window  ( draw_rect,    m_delta_lon, m_delta_lon, g_draw_list );
    _test_range     ( g_draw_list );
}

void CMapPainter::_set_pixel ( const int32_t x, const int32_t y, const geo_color_t color ) {

    CPoint pt;

    pt.x = x;
    pt.y = y;

    m_paint_dc->SetPixel ( pt, color );
}

void CMapPainter::_draw_line ( const geo_coord_t p1, const geo_coord_t p2, const geo_color_t color ) {

    int32_t x1 = static_cast<int32_t> (p1.x); 
    int32_t y1 = static_cast<int32_t> (p1.y); 
    int32_t x2 = static_cast<int32_t> (p2.x); 
    int32_t y2 = static_cast<int32_t> (p2.y);

    const int deltaX = abs(x2 - x1);
    const int deltaY = abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;
    int error = deltaX - deltaY;

    _set_pixel ( x2, y2, color );

    while (x1 != x2 || y1 != y2) {

        _set_pixel ( x1, y1, color );

        int error2 = error * 2;

        if (error2 > -deltaY) {
            error -= deltaY;
            x1    += signX;
        }

        if ( error2 < deltaX ) {
            error += deltaX;
            y1 += signY;
        }
    }
}

void CMapPainter::_draw_poly_line ( const v_geo_coord_t& poly_line, const geo_color_t color ) {

    if (poly_line.size() < 2) {
        return;
    }

    geo_coord_t p1, p2;

    auto it = poly_line.cbegin();

    p1 = *it;

    it++;
    while ( it != poly_line.cend() ) {

        p2 =  p1;
        p1 = *it;

        _draw_line ( p2, p1, color );

        it++;
    }

}

void CMapPainter::_map_color ( obj_type_t geo_type, geo_color_t& geo_color ) {

    switch (geo_type) {
        case OBJID_TYPE_FOREST:       geo_color = GEO_RGB(   0, 100,   0 ); break;
        case OBJID_TYPE_GRASS:        geo_color = GEO_RGB(   0, 200,   0 ); break;
        case OBJID_TYPE_ASPHALT:      geo_color = GEO_RGB( 100, 100, 100 ); break;
        case OBJID_TYPE_BUILDING:     geo_color = GEO_RGB(  80, 100, 120 ); break;

        case OBJID_TYPE_WATER:        geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_GENERAL:      geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_MOUNTAIN:     geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_STONE:        geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_SAND:         geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_UNDEFINED:    geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_FOOTWAY:      geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_ROAD:         geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_SECONDARY:    geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_TRUNK:        geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_MOTORWAY:     geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_PRIMARY:      geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_TERTIARY:     geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_RAILWAY:      geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_RIVER:        geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_BRIDGE:       geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_TYPE_TUNNEL:       geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_RECORD_AREA:       geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_RECORD_BUILDING:   geo_color = GEO_RGB(0,0,0);   break;
        case OBJID_RECORD_HIGHWAY:    geo_color = GEO_RGB(0,0,0);   break;

        default:
            geo_color = GEO_RGB(0, 0, 200);   
            break;
    }
}

void CMapPainter::_paint_map ( const list_geo_record_t& draw_list, CPaintDC& dc, CRect& clientRect ) {

    geo_color_t color;

    (void)(dc);
    (void)(clientRect);

    color = 0;

    for ( auto geo_record = draw_list.cbegin(); geo_record != draw_list.cend(); geo_record++ ) {
        for ( size_t i = 0; i < geo_record->m_child_lines.size(); i++ ) {
            if ( geo_record->m_geo_type == OBJID_RECORD_AREA ) {
                _map_color       ( geo_record->m_child_types[i], color );
                _draw_poly_line  ( geo_record->m_child_lines[i], color );
            }
        }
    }    
}

void CMapPainter::OnPaint ( void ) {

    int	x;
    int	y;

    CRect       clientRect;
    CRgn        clientRgn;
    geo_rect_t  geo_rect;

    CPaintDC dc ( this );

    m_paint_dc = &dc;

    x = m_BasePosition.x;
    y = m_BasePosition.y;

    if (m_DragActive) {
        x += m_DeltaX;
        y += m_DeltaY;
    }

    GetClientRect ( clientRect );

    double geo_cache_width  = clientRect.Width();
    double geo_cache_height = clientRect.Height();

    geo_cache_width  *= ( m_delta_lon / m_scale );
    geo_cache_height *= ( m_delta_lat / m_scale );

    geo_rect.min_lon = m_lon;
    geo_rect.min_lat = m_lat;
    geo_rect.max_lon = m_lon + geo_cache_width;
    geo_rect.max_lat = m_lat + geo_cache_height;

    test ( geo_rect );

    // clientRgn.CreateRectRgnIndirect ( clientRect );
    // dc.SelectClipRgn ( &clientRgn );
    dc.FillSolidRect ( clientRect, RGB(180, 180, 180) );
    _paint_map ( g_draw_list, dc, clientRect );

    // dc.TextOut ( x, y, TEXT("TextOut Samples") );
    // dc.SelectClipRgn ( NULL );
}

void CMapPainter::OnMouseMove ( UINT nFlags, CPoint point ) {

    CStatic::OnMouseMove ( nFlags, point );

    if ( m_DragActive ) {
        m_DeltaX  =  point.x - m_PickPoint.x;
        m_DeltaY  =  point.y - m_PickPoint.y;
        Invalidate(1);
        UpdateWindow();
    }
}

void CMapPainter::OnMouseHover ( UINT nFlags, CPoint point ) {
    CStatic::OnMouseHover(nFlags, point);
}

void CMapPainter::OnMouseLeave ( void ) {

    TRACKMOUSEEVENT tme;

    CStatic::OnMouseLeave();

    if ( m_bMouseTracking ) {

        m_bMouseTracking = FALSE;

        tme.cbSize    = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags   = TME_CANCEL;
        tme.hwndTrack = this->m_hWnd;

        _TrackMouseEvent(&tme);
    }
}

void CMapPainter::OnLButtonDown ( UINT nFlags, CPoint point ) {

    TRACKMOUSEEVENT tme;

    CStatic::OnLButtonDown ( nFlags, point );

    m_PickPoint = point;
    m_DragActive = true;

    if ( !m_bMouseTracking ) {

        m_bMouseTracking = TRUE;

        tme.cbSize    = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags   = TME_LEAVE;
        tme.hwndTrack = this->m_hWnd;

        _TrackMouseEvent(&tme);
    }
}

void CMapPainter::OnLButtonUp ( UINT nFlags, CPoint point ) {

    (void)(nFlags);
    (void)(point);

    double new_lon;
    double new_lat;

    if ( m_DragActive ) {

        m_DragActive = false;

        new_lon = m_lon + m_delta_lon * m_DeltaX;
        new_lat = m_lat + m_delta_lat * m_DeltaY;

        m_BasePosition.x = m_BasePosition.x + m_DeltaX;
        m_BasePosition.y = m_BasePosition.y + m_DeltaY;

        m_lon = new_lon;
        m_lat = new_lat;

        Invalidate(1);
        UpdateWindow();

        GetParent()->PostMessage(WM_MAP_UPDATE, 0, 0);
    }
}

BOOL CMapPainter::OnEraseBkgnd ( CDC* pDC ) {
    return CStatic::OnEraseBkgnd(pDC);
}

void CMapPainter::SetBaseParams ( double lon, double lat, double scale ) {

    const double delta_hor = 0.0001;
    const double delta_ver = 0.0001;

    m_lon   = lon;
    m_lat   = lat;
    m_scale = scale;

    GeographicLib::Geodesic geod(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f());

    double shift_hor;
    geod.Inverse ( m_lat, m_lon, m_lat, m_lon + delta_hor, shift_hor );
    m_delta_lon  = delta_hor / shift_hor;
    m_delta_lon /= scale;

    double shift_ver;
    geod.Inverse ( m_lat, m_lon, m_lat + delta_ver, m_lon, shift_ver );
    m_delta_lat  = delta_ver / shift_ver;
    m_delta_lat /= scale;

    CRect			client_rect;

    GetClientRect ( &client_rect );

    double delta_width;
    double delta_height;

    delta_width  = client_rect.Width();
    delta_width *= m_delta_lon;

    delta_height  = client_rect.Height();
    delta_height *= m_delta_lat;

    #if 1
        geod.Inverse ( m_lat, m_lon, m_lat,  m_lon + delta_width, shift_hor );
        geod.Inverse ( m_lat, m_lon, m_lat + delta_height, m_lon, shift_ver );
    #endif
}

void CMapPainter::GetBaseParams ( double& lon, double& lat, double& scale ) {

    lon   = m_lon;
    lat   = m_lat;
    scale = m_scale;
}
