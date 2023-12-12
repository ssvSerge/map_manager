#include "pch.h"

#include "geo_processor.h"

#include <fstream>
#include <string>
#include <algorithm>

#include "MapPainter.h"

#define NAME_IDX      "C:\\GitHub\\map_manager\\dev\\_bin\\ohrada_idx.txt"
#define NAME_MAP      "C:\\GitHub\\map_manager\\dev\\_bin\\ohrada_map.txt"

#define CURSOR_MOVE   (1)
#define CURSOR_WAIT   (2)
#define CURSOR_AUTO   (3)


IMPLEMENT_DYNAMIC ( CMapPainter, CStatic )

BEGIN_MESSAGE_MAP ( CMapPainter, CStatic )
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_ERASEBKGND()
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

static geo_processor_t    g_geo_processor;

CMapPainter::CMapPainter () {

    m_DragActive     = false;
    m_bMouseTracking = false;
    m_BasePosition.x = 200;
    m_BasePosition.y = 200;
    m_DeltaX         = 0;
    m_DeltaY         = 0;
    m_delta_hor      = 0;
    m_delta_ver      = 0;
    m_paint_dc       = nullptr;

    m_base_lon       = 14.339209;
    m_base_lat       = 50.036852;
    m_shift_lon      = 0;
    m_shift_lat      = 0;
    m_scale		     = 1;
    m_angle          = 0;

    return;
}

CMapPainter::~CMapPainter () {

    g_geo_processor.close();
}

void CMapPainter::OnPaint ( void ) {

    if ( m_DragActive ) {

        m_shift_lon = 0;
        m_shift_lat = 0;

    } else {

        double org_lon   = 0;
        double org_lat   = 0;
        double shift_lon = 0;
        double shift_lat = 0;

        org_lon = m_base_lon;
        org_lat = m_base_lat;

        g_geo_processor.get_shifts ( org_lon, org_lat, shift_lon, shift_lat );

        shift_lon   = m_DeltaX / m_scale;
        shift_lat   = m_DeltaY / m_scale;

        m_shift_lon = shift_lon;
        m_shift_lat = shift_lat;

    }


    #if 0

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

    map_pos_t     pos;
    geo_pixel_t   px;
    COLORREF      outClr = 0;

    for (y = 0; y < m_client_rect.Height(); y++) {
        for (x = 0; x < m_client_rect.Width(); x++) {

            pos.x = ( x );
            pos.y = ( y );

            g_geo_processor.get_pix( pos, px );
            outClr = RGB ( px.getR(), px.getG(), px.getB() );

            dcMem.SetPixel ( x, m_client_rect.Height() - y - 1, outClr );
            // dc.SetPixel ( x, m_client_rect.Height() - y - 2, RGB(0, 0, 0) );
        }
    }

    dc.BitBlt (m_client_rect.left, m_client_rect.top, m_client_rect.Width(), m_client_rect.Height(), &dcMem, 0, 0, SRCCOPY);
    dcMem.SelectObject(pOldBitmap);

    #endif

}

void CMapPainter::OnLButtonDown ( UINT nFlags, CPoint point ) {

    CStatic::OnLButtonDown ( nFlags, point );

    if ( !m_bMouseTracking ) {

        m_bMouseTracking = TRUE;

        m_cursor_type = CURSOR_MOVE;
        ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZE));

        TRACKMOUSEEVENT tme;

        tme.cbSize    = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags   = TME_LEAVE;
        tme.hwndTrack = this->m_hWnd;

        _TrackMouseEvent(&tme);
    }

    m_DragActive = true;
    m_PickPoint = point;

    return;
}

void CMapPainter::OnLButtonUp ( UINT nFlags, CPoint point ) {

    CStatic::OnLButtonUp(nFlags, point);

    if ( m_bMouseTracking ) {

        m_bMouseTracking = FALSE;

        m_cursor_type = CURSOR_AUTO;
        ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));

        TRACKMOUSEEVENT tme;

        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_CANCEL;
        tme.hwndTrack = this->m_hWnd;

        _TrackMouseEvent(&tme);
    }

    if ( m_DragActive ) {

        m_DragActive = false;

        m_base_lon   += m_shift_lon;
        m_shift_lon   = 0;

        m_base_lat   += m_shift_lat;
        m_shift_lat   = 0;

        Invalidate(1);
        UpdateWindow();
        GetParent()->PostMessage(WM_MAP_UPDATE, 0, 0);
    }
}

void CMapPainter::OnMouseMove ( UINT nFlags, CPoint point ) {

    CStatic::OnMouseMove ( nFlags, point );

    if ( m_DragActive ) {

        double step_x  = 0;
        double step_y  = 0;

        m_DeltaX  =  point.x - m_PickPoint.x;
        m_DeltaY  =  point.y - m_PickPoint.y;

        g_geo_processor.get_shifts ( point.x + m_DeltaX, point.y + m_DeltaY, step_x, step_y );

        m_shift_lon = (m_DeltaX / m_scale) * step_x;
        m_shift_lat = (m_DeltaY / m_scale) * step_y;

        SetBaseParams (m_shift_lon, m_shift_lat, m_scale, m_angle );

        Invalidate(1);
        UpdateWindow();
        GetParent()->PostMessage(WM_MAP_UPDATE, 0, 0);

    } else {

        if ( m_shift_lon != 0 ) {
            m_base_lon += m_shift_lon;
            m_shift_lon = 0;
        }

        if ( m_shift_lat != 0 ) {
            m_base_lat += m_shift_lat;
            m_shift_lat = 0;
        }

    }
}

BOOL CMapPainter::OnEraseBkgnd ( CDC* pDC ) {
    return CStatic::OnEraseBkgnd(pDC);
}

void CMapPainter::SetBaseParams ( double lon, double lat, double scale, double angle ) {

    static bool is_valid = false;

    geo_rect_t    wnd;
    CRect         client_rect;
    geo_coord_t   center;
    double        ang = angle;

    AfxGetApp()->DoWaitCursor(1);

    if ( !is_valid ) {
        is_valid = true;
        g_geo_processor.set_names ( NAME_IDX, NAME_MAP );
        g_geo_processor.load_idx();
    }

    GetClientRect ( client_rect );

    wnd.min.map.x = 0;
    wnd.min.map.y = 0;
    wnd.max.map.x = client_rect.Width();
    wnd.max.map.y = client_rect.Height();

    center.set_src ( POS_TYPE_MAP );
    center.geo.x = lon;
    center.geo.y = lat;

    ang = angle;

    g_geo_processor.process_map ( wnd, center, scale, ang );

    AfxGetApp()->DoWaitCursor(-1);

    return;
}

void CMapPainter::GetBaseParams ( double& lon, double& lat, double& scale ) const {

    lon   = m_base_lon + m_shift_lon;
    lat   = m_base_lat + m_shift_lat;
    scale = m_scale;
}

BOOL CMapPainter::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) {

    switch (m_cursor_type) {
        case 1:
            ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_IBEAM));
            return true;

        case 2:
            ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
            return true;

        case 3:
            ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
            return true;

        default:
            return CStatic::OnSetCursor(pWnd, nHitTest, message);

    }

}
