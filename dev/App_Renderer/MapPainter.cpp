#include "pch.h"

#include "geo_processor.h"

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

static geo_processor_t    g_geo_processor;

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
    m_delta_hor      = 0;
    m_delta_ver      = 0;
    m_paint_dc       = nullptr;

    geo_coord_t  center;
    paint_rect_t wnd;

    center.y = 50.0368000;
    center.x = 14.3385000;

    wnd.max.x = 925;
    wnd.max.y = 700;

    g_geo_processor.set_names ( 
        "C:\\GitHub\\map_manager\\dev\\_bin\\prague_idx.txt", 
        "C:\\GitHub\\map_manager\\dev\\_bin\\prague_map.txt" 
    ); 

    g_geo_processor.alloc_buffer ( wnd.width(), wnd.height() );
    g_geo_processor.cache_init();
    g_geo_processor.set_base_params ( center, 1.0, wnd );
    g_geo_processor.set_angle(0);
    g_geo_processor.trim_map();
}

CMapPainter::~CMapPainter () {
}

void CMapPainter::OnPaint ( void ) {

    int	x;
    int	y;

    CPaintDC dc ( this );

    m_paint_dc = &dc;

    x = m_BasePosition.x;
    y = m_BasePosition.y;

    if ( m_DragActive ) {
        x += m_DeltaX;
        y += m_DeltaY;
    }

    GetClientRect ( m_client_rect );

    g_geo_processor.process_wnd();

    paint_coord_t   pos;
    geo_pixel_t     px;
    COLORREF        outClr = 0;

    for (y = 0; y < m_client_rect.Height(); y++) {
        for (x = 0; x < m_client_rect.Width(); x++) {

            pos.x = x;
            pos.y = y;

            g_geo_processor.get_pix( pos, px );
            outClr = RGB ( px.getR(), px.getG(), px.getB() );

            dc.SetPixel ( x, y, outClr );
            dc.SetPixel ( x, y + 1, RGB(0, 0, 0) );
        }
    }

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

    double new_hor;
    double new_ver;

    if ( m_DragActive ) {

        m_DragActive = false;

        new_hor = m_lon + m_delta_hor * m_DeltaX;
        new_ver = m_lat + m_delta_ver * m_DeltaY;

        m_BasePosition.x = m_BasePosition.x + m_DeltaX;
        m_BasePosition.y = m_BasePosition.y + m_DeltaY;

        m_lon = new_hor;
        m_lat = new_ver;

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
    m_delta_hor  = delta_hor / shift_hor;
    m_delta_hor /= scale;

    double shift_ver;
    geod.Inverse ( m_lat, m_lon, m_lat + delta_ver, m_lon, shift_ver );
    m_delta_ver  = delta_ver / shift_ver;
    m_delta_ver /= scale;

    CRect client_rect;

    GetClientRect ( &client_rect );

    double delta_width;
    double delta_height;

    delta_width  = client_rect.Width();
    delta_width *= m_delta_hor;

    delta_height  = client_rect.Height();
    delta_height *= m_delta_ver;

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
