#include "pch.h"

#include <fstream>
#include <string>

#include "MapPainter.h"
#include "GeoCache.h"

#include <GeographicLib/Geodesic.hpp>

#include <geo/geo_idx.h>

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

static geo_idx_record_t         g_geo_idx;
static geo_indexer_t            g_geo_idx_file;

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

    g_geo_idx_file.set_idx_name ( "C:\\GitHub\\map_manager\\dev\\_bin\\prague_idx.txt" );
    g_geo_idx_file.set_map_name ( "C:\\GitHub\\map_manager\\dev\\_bin\\prague_map.txt" );
    g_geo_idx_file.load_idx_file();
}

CMapPainter::~CMapPainter () {
}

void CMapPainter::OnPaint ( void ) {

    int			x;
    int			y;
    CRect		clientRect;
    CRgn		clientRgn;
    geo_rect_t  geo_rect;

    CPaintDC dc ( this );

    if (m_DragActive) {
        x = m_BasePosition.x + m_DeltaX;
        y = m_BasePosition.y + m_DeltaY;
    } else {
        x = m_BasePosition.x;
        y = m_BasePosition.y;
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

    g_geo_idx_file.cache_region ( geo_rect );

    clientRgn.CreateRectRgnIndirect ( clientRect );

    dc.SelectClipRgn ( &clientRgn );
    dc.FillSolidRect ( clientRect, RGB(180, 180, 180) );
    dc.TextOut ( x, y, TEXT("TextOut Samples") );
    dc.SelectClipRgn ( NULL );
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

    #if 1
    geod.Inverse ( m_lat, m_lon, m_lat, m_lon + m_delta_lon, shift_hor );
    geod.Inverse ( m_lat, m_lon, m_lat + m_delta_lat, m_lon, shift_ver );
    #endif

    CRect			client_rect;
    // geo_rect_t   geo_rect;

    GetClientRect ( &client_rect );

    double delta_width;
    double delta_height;

    delta_width  = client_rect.Width();
    delta_width *= m_delta_lon;
    delta_width *= 0.75;

    delta_height  = client_rect.Height();
    delta_height *= m_delta_lat;
    delta_height *= 0.75;

    // geo_rect.min_lon = m_lon - delta_width;
    // geo_rect.max_lon = m_lon + delta_width;

    // geo_rect.min_lat = m_lat - delta_height;
    // geo_rect.max_lat = m_lat + delta_height;

    // g_geo_idx.CacheRegion(geo_rect);
}

void CMapPainter::GetBaseParams ( double& lon, double& lat, double& scale ) {

    lon   = m_lon;
    lat   = m_lat;
    scale = m_scale;
}
