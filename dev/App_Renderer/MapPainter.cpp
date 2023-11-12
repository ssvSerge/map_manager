#include "pch.h"

#include "geo_processor.h"

#include <fstream>
#include <string>
#include <algorithm>

#include "MapPainter.h"

// #include <GeographicLib/Geodesic.hpp>
// #include <idx_file.h>

#define NAME_IDX    "C:\\GitHub\\map_manager\\dev\\_bin\\ohrada_idx.txt"
#define NAME_MAP    "C:\\GitHub\\map_manager\\dev\\_bin\\ohrada_map.txt"


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
}

CMapPainter::~CMapPainter () {

    g_geo_processor.close();
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

    CDC dcMem;
    CBitmap bitmap;

    dcMem.CreateCompatibleDC( &dc );
    bitmap.CreateCompatibleBitmap(&dc, m_client_rect.Width(), m_client_rect.Height());
    CBitmap* pOldBitmap = dcMem.SelectObject ( &bitmap );

    geo_coord_t   pos;
    geo_pixel_t   px;
    COLORREF      outClr = 0;

    for (y = 0; y < m_client_rect.Height(); y++) {
        for (x = 0; x < m_client_rect.Width(); x++) {

            pos.map.x = ( x );
            pos.map.y = ( y );

            g_geo_processor.get_pix( pos, px );
            outClr = RGB ( px.getR(), px.getG(), px.getB() );

            dcMem.SetPixel ( x, m_client_rect.Height() - y - 1, outClr );
            // dc.SetPixel ( x, m_client_rect.Height() - y - 2, RGB(0, 0, 0) );
        }
    }

    dc.BitBlt (m_client_rect.left, m_client_rect.top, m_client_rect.Width(), m_client_rect.Height(), &dcMem, 0, 0, SRCCOPY);
    dcMem.SelectObject(pOldBitmap);
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

void CMapPainter::_calc_geo ( double lon, double lat, double scale ) const {

    (void) (lon);
    (void) (lat);
    (void) (scale);
}

void CMapPainter::SetBaseParams ( double lon, double lat, double scale, double angle ) const {

    static bool is_valid = false;

    geo_rect_t    wnd;
    CRect         client_rect;
    geo_coord_t   center;
    double        ang = angle;

    if ( !is_valid ) {
        is_valid = true;
        g_geo_processor.set_names ( NAME_IDX, NAME_MAP );
    }

    (void)(lon);
    (void)(lat);

    GetClientRect ( client_rect );

    wnd.min.map.x = 0;
    wnd.min.map.y = 0;
    wnd.max.map.x = client_rect.Width();
    wnd.max.map.y = client_rect.Height();

    center.set_src ( POS_TYPE_MAP );

    // 50.036852, 14.339209
    center.geo.x = 14.339209;
    center.geo.y = 50.036852;
    ang          = 0;

    g_geo_processor.load_idx ();
    g_geo_processor.process_map ( wnd, center, scale, ang );
}

void CMapPainter::GetBaseParams ( double& lon, double& lat, double& scale ) const {

    lon   = m_lon;
    lat   = m_lat;
    scale = m_scale;
}
