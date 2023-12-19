#include "pch.h"

#include "geo_processor.h"

#include <fstream>
#include <string>
#include <algorithm>

#include "MapPainter.h"

#include <platform/agg_platform_support.h>
#include <util/agg_color_conv.h>

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
    ON_WM_SIZE()
END_MESSAGE_MAP()

static geo_processor_t    g_geo_processor;


void CMapPainter::_video_init ( void ) {

}

void CMapPainter::_video_render ( void ) {

    return;
}

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
    m_isVideoActive  = false;
}

CMapPainter::~CMapPainter () {

    g_geo_processor.close();
}

static void convert_pmap ( agg::rendering_buffer* dst, const agg::rendering_buffer* src, agg::pix_format_e format) {

    switch (format) {
        case agg::pix_format_rgb24:
            convert<agg::pixfmt_sbgr24, agg::pixfmt_rgb24>(dst, src);
            break;

        case agg::pix_format_bgr24:
            convert<agg::pixfmt_sbgr24, agg::pixfmt_bgr24>(dst, src);
            break;
    }
}

void CMapPainter::OnPaint ( void ) {

    static bool                     is_init = false;
    static agg::pixel_map           pmap_tmp;
    static agg::rendering_buffer    rbuf_tmp;

    CPaintDC dc(this);
    CRect Rect;

    GetClientRect(&Rect);

    if ( !is_init ) {
        is_init = true;
        int sys_bpp = 24;
        pmap_tmp.create ( Rect.Width(), Rect.Height(), agg::org_e(sys_bpp) );
        rbuf_tmp.attach ( pmap_tmp.buf(), pmap_tmp.width(), pmap_tmp.height(), -pmap_tmp.stride() );
        g_geo_processor.set_screen_params ( Rect.Width(), Rect.Height() );
    }

    g_geo_processor.paint();

    convert_pmap ( &rbuf_tmp, &g_geo_processor.m_rbuf_window, pix_format );

    pmap_tmp.draw(dc.m_hDC);

    CPen pen(PS_SOLID, 0, RGB(0, 0, 0));
    dc.SelectObject(&pen);

    
    int len     = 20;

    int min_x   = Rect.Width()  / 2 - (len / 2);
    int mid_x   = Rect.Width()  / 2;
    int max_x   = Rect.Width()  / 2 + (len / 2);

    int min_y   = Rect.Height() / 2 - (len / 2);
    int mid_y   = Rect.Height() / 2;
    int max_y   = Rect.Height() / 2 + (len / 2);

    dc.MoveTo ( min_x,  mid_y );
    dc.LineTo ( max_x,  mid_y );
    dc.MoveTo ( mid_x,  min_y );
    dc.LineTo ( mid_x,  max_y );
}

void CMapPainter::OnSize ( UINT nType, int cx, int cy ) {

    CStatic::OnSize ( nType, cx, cy );
    return;
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
    (void) (pDC);
    // return CStatic::OnEraseBkgnd(pDC);
    return TRUE;
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
