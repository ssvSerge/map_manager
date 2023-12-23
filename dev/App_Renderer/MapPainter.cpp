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

class paint_ctx_t {
    public:
        uint32_t        m_img_size;
        uint32_t        m_full_size;
        BITMAPINFO*     m_bmp;
        uint8_t*        m_buf;
        void*           img_ptr;
        HBITMAP         hBitmap;
        uint32_t        cli_w;
        uint32_t        cli_h;
};

class clr_t {

    public:
        clr_t() {
            r = g = b = 0;
        }

        clr_t (uint8_t _r, uint8_t _g, uint8_t _b ) {
            r = _r;
            g = _g;
            b = _b;
        }

    public:
        uint8_t      b;
        uint8_t      g;
        uint8_t      r;
};

#pragma pack(1)
typedef struct tag_screen_pix {
    uint8_t      b;
    uint8_t      g;
    uint8_t      r;
}   screen_pix_t;
#pragma pack()


static paint_ctx_t   g_paint_ctx;

static uint32_t _calc_row_len ( uint32_t width, uint32_t bits_per_pixel ) {

    unsigned n = width;
    unsigned k;

    switch (bits_per_pixel) {
    case  1: k = n;
        n = n >> 3;
        if (k & 7) n++;
        break;

    case  4: k = n;
        n = n >> 1;
        if (k & 3) n++;
        break;

    case  8:
        break;

    case 16: n *= 2;
        break;

    case 24: n *= 3;
        break;

    case 32: n *= 4;
        break;

    case 48: n *= 6;
        break;

    case 64: n *= 8;
        break;

    case 96: n *= 12;
        break;

    case 128: n *= 16;
        break;

    default: n = 0;
        break;
    }
    return ((n + 3) >> 2) << 2;
}

static uint32_t _calc_palette_size ( uint32_t clr_used, uint32_t bits_per_pixel ) {
    int palette_size = 0;
    if (bits_per_pixel <= 8) {
        palette_size = clr_used;
        if (palette_size == 0) {
            palette_size = 1 << bits_per_pixel;
        }
    }
    return palette_size;
}

static uint32_t _calc_palette_size ( BITMAPINFO* bmp) {
    if (bmp == 0) {
        return 0;
    }
    return _calc_palette_size ( bmp->bmiHeader.biClrUsed, bmp->bmiHeader.biBitCount );
}

static uint32_t _calc_full_size ( BITMAPINFO* bmp ) {
    if (bmp == 0) {
        return 0;
    }
    return sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * _calc_palette_size(bmp) + bmp->bmiHeader.biSizeImage;
}

static uint32_t _calc_header_size ( BITMAPINFO* bmp ) {
    if (bmp == 0) {
        return 0;
    }
    return sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * _calc_palette_size(bmp);
}

static uint8_t* _calc_img_ptr ( BITMAPINFO* bmp ) {
    if (bmp == 0) {
        return 0;
    }
    return ((unsigned char*)bmp) + _calc_header_size(bmp);
}

static void _create_dib_section ( HDC dc, uint32_t width, uint32_t height, uint32_t bits_per_pixel, paint_ctx_t& ctx ) {

    uint32_t    line_len   =  0;
    uint32_t    img_size   =  0;
    uint32_t    rgb_size   =  0;
    uint32_t    full_size  =  0;
    BITMAPINFO* bmp     =  nullptr;
    void*       img_ptr =  0;

    line_len   =  _calc_row_len(width, bits_per_pixel);
    img_size   =  line_len * height;
    rgb_size   =  _calc_palette_size(0, bits_per_pixel) * sizeof(RGBQUAD);
    full_size  =  sizeof(BITMAPINFOHEADER) + rgb_size + img_size;

    bmp = (BITMAPINFO*) new unsigned char[full_size];

    memset ( bmp, 0xFF, full_size );

    bmp->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmp->bmiHeader.biWidth         = width;
    bmp->bmiHeader.biHeight        = height;
    bmp->bmiHeader.biPlanes        = 1;
    bmp->bmiHeader.biBitCount      = (unsigned short)bits_per_pixel;
    bmp->bmiHeader.biCompression   = 0;
    bmp->bmiHeader.biSizeImage     = img_size;
    bmp->bmiHeader.biXPelsPerMeter = 0;
    bmp->bmiHeader.biYPelsPerMeter = 0;
    bmp->bmiHeader.biClrUsed       = 0;
    bmp->bmiHeader.biClrImportant  = 0;

    HBITMAP h_bitmap = ::CreateDIBSection ( dc, bmp, DIB_RGB_COLORS, &img_ptr, NULL, 0 );

    if ( img_ptr ) {
        ctx.m_img_size   =  _calc_row_len ( width, bits_per_pixel ) * height;
        ctx.m_full_size  =  _calc_full_size(bmp);
        ctx.m_bmp        =  bmp;
        ctx.m_buf        =  _calc_img_ptr(bmp);
        ctx.hBitmap      =  h_bitmap;
        ctx.img_ptr      =  img_ptr;
        ctx.cli_w        =  width;
        ctx.cli_h        =  height;

    }

    return;
}

static void _put_pixel ( int32_t x, int32_t y, clr_t& clr ) {

    if (x >= g_paint_ctx.m_bmp->bmiHeader.biWidth) {
        return;
    }
    if (y >= g_paint_ctx.m_bmp->bmiHeader.biHeight) {
        return;
    }

    y = g_paint_ctx.m_bmp->bmiHeader.biHeight - 1 - y;

    int32_t line_len  = _calc_row_len ( g_paint_ctx.m_bmp->bmiHeader.biWidth, g_paint_ctx.m_bmp->bmiHeader.biBitCount);
    int32_t offset    = line_len * y;
    screen_pix_t* map = (screen_pix_t*)(g_paint_ctx.m_buf + offset);

    map[x].r  =  clr.r;
    map[x].g  =  clr.g;
    map[x].b  =  clr.b;
}

static void _draw ( HDC dc ) {

    unsigned bmp_width   =  g_paint_ctx.m_bmp->bmiHeader.biWidth;
    unsigned bmp_height  =  g_paint_ctx.m_bmp->bmiHeader.biHeight;
    unsigned dvc_width   =  g_paint_ctx.m_bmp->bmiHeader.biWidth;
    unsigned dvc_height  =  g_paint_ctx.m_bmp->bmiHeader.biHeight;

    dvc_width  = bmp_width;
    dvc_height = bmp_height;

    ::SetStretchBltMode ( dc, COLORONCOLOR );
    ::SetDIBitsToDevice ( dc, 0, 0, dvc_width, dvc_height, 0, 0, 0, bmp_height, g_paint_ctx.m_buf, g_paint_ctx.m_bmp, DIB_RGB_COLORS );
}

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
    m_cursor_type    = 0;

    return;
}

CMapPainter::~CMapPainter () {

    g_geo_processor.close();
}

void CMapPainter::OnPaint ( void ) {

    static bool is_init = false;

    CRect clientRect;
    CPaintDC dc(this);

    GetClientRect ( &clientRect );

    if ( ! is_init ) {
        is_init = true;
        _create_dib_section (dc.m_hDC, clientRect.Width(), clientRect.Height(), 24, g_paint_ctx );
        g_geo_processor.video_alloc ( clientRect.Width(), clientRect.Height() );
    }

    int32_t  dst_line_len  =  _calc_row_len(g_paint_ctx.m_bmp->bmiHeader.biWidth, g_paint_ctx.m_bmp->bmiHeader.biBitCount);
    int32_t  max_offset    =  clientRect.Width() * clientRect.Height();

    uint16_t*        src_ptr  =  nullptr;
    screen_pix_t*    dst_ptr  =  nullptr;
    map_pos_t        pos;
    geo_pixel_t      px;

    if ( max_offset == g_geo_processor.m_video_buffer.size() ) {

        for ( int32_t y = 0; y < clientRect.Height(); y++ ) {

            dst_ptr = (screen_pix_t*) ( g_paint_ctx.m_buf + dst_line_len*y );
            src_ptr = (uint16_t*)     ( &g_geo_processor.m_video_buffer [ y * clientRect.Width() ] );

            for (int32_t x = 0; x < clientRect.Width(); x++) {
                g_geo_processor.unpack ( src_ptr[x], dst_ptr[x].r, dst_ptr[x].g, dst_ptr[x].b );
            }
        }
    }

    _draw ( dc.m_hDC );

    return;
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

    g_geo_processor.process_map ( center, scale, ang );

    AfxGetApp()->DoWaitCursor(-1);

    return;
}

void CMapPainter::GetBaseParams ( double& lon, double& lat, double& scale ) const {

    lon   = m_base_lon + m_shift_lon;
    lat   = m_base_lat + m_shift_lat;
    scale = m_scale;
}

BOOL CMapPainter::OnSetCursor ( CWnd* pWnd, UINT nHitTest, UINT message ) {

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
