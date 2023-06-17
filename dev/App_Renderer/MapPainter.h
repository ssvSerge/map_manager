#pragma once

#define WM_MAP_UPDATE (WM_USER + 1)

#include <geo_types.h>
#include <stdint.h>

#include <vector>
#include <cmath>
#include <stack>
#include <list>
#include <set>
#include <algorithm>

#include <geo_types.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

class screen_pt_t {

    public:
        screen_pt_t ( ) {
            x = 0;
            y = 0;
        }

        screen_pt_t ( int32_t _x, int32_t _y ) {
            x = _x;
            y = _y;
        }

    public:
        int32_t     x;
        int32_t     y;
};

class screen_pix_t {
    public:
        uint8_t     r;
        uint8_t     g;
        uint8_t     b;
        uint8_t     a;

    public:
        screen_pix_t() {
            r = g = b = a = 0;
        }

        void clear() { 
            r = 0; 
            g = 0; 
            b = 0; 
            a = 0; 
        }

        const screen_pix_t Shift ( int val ) {

            screen_pix_t ret_val;

            ret_val.a = this->a;

            ret_val.r = _saturate ( this->r, val );
            ret_val.g = _saturate ( this->g, val );
            ret_val.b = _saturate ( this->b, val );

            return ret_val;
        }

        bool operator!= (const screen_pix_t& ref) const {
            auto cmp_res = this->operator==(ref);
            return !cmp_res;
        }

        bool operator== (const screen_pix_t& ref) const {

            if ( a != ref.a ) {
                return false;
            }
            if ( r != ref.r ) {
                return false;
            }
            if ( g != ref.g ) {
                return false;
            }
            if ( b != ref.b ) {
                return false;
            }

            return true;
        }

    private:
        uint8_t _saturate ( uint8_t val, int shift ) {

            int ret_val = val;

            ret_val += shift;

            if (ret_val > 255) {
                ret_val = 255;
            }
            if (ret_val < 0) {
                ret_val = 0;
            }

            return (uint8_t) (ret_val);
        }

};

class screen_line_t {

    public:
        std::vector<screen_pix_t> line;

    public:
        void clear() {
            for (size_t i = 0; i < line.size(); i++) {
                line[i].clear();
            }
        }
};

typedef std::vector<screen_line_t> screen_data_t;

class screen_t {

    private:
        screen_data_t  lines;
        screen_data_t  shadow;
        int32_t        screen_x;
        int32_t        screen_y;

    public:
        screen_t() {
            lines.clear();
            shadow.clear();
            screen_x = 0;
            screen_y = 0;
        }

    public:

        void Init ( int32_t new_x, int32_t new_y ) {

            bool clear_request = false;

            if ( new_y != screen_y ) {
                clear_request = true;
            }
            if ( new_x != screen_x ) {
                clear_request = true;
            }

            screen_y = new_y;
            screen_x = new_x;

            if ( clear_request ) {

                lines.clear();
                lines.resize ( screen_y );

                for (size_t i = 0; i < screen_y; i++) {
                    lines[i].line.resize(screen_x);
                }

                shadow = lines;

            }  else  {
                _clear(lines);
                _clear(shadow);
            }

        }

        void Clear ( void ) {
            _clear( lines );
        }

        void SetPix ( const geo_coord_t pos, const screen_pix_t clr ) {
            _set_pix ( pos, clr, lines );
        }

        void GetPix ( const geo_coord_t pos, screen_pix_t& px ) {
            _get_pix(pos, px, lines );
        }

        void Line ( const geo_coord_t from, const geo_coord_t to, const screen_pix_t color ) {
            _line ( from, to, color, lines );
        }

        void PolyLine ( const v_geo_coord_t& poly_line, const screen_pix_t color ) {
            _poly_line ( poly_line, color, lines );
        }

        void FillSolid ( const screen_pix_t clr ) {
            _fill_solid ( clr, lines );
        }

        void FillRegion ( const geo_coord_t pt, const screen_pix_t border, const screen_pix_t fill ) {
            _fill_rect ( pt, border, fill, lines );
        }

        void FillRegion ( const v_geo_coord_t& poly_line, const screen_pix_t border, const screen_pix_t fill ) {

            screen_pix_t clr;

            clr.a =   0;
            clr.r = 200;
            clr.g = 200;
            clr.b = 200;

            _fill_solid ( clr, lines );
            _fill_region ( poly_line, border, fill, shadow );
            _merge ( shadow, lines );
        }

    private:

        uint8_t _process_alpha ( uint8_t alpha, uint8_t top, uint8_t bk ) {

            uint32_t p1 = top * alpha;
            uint32_t p2 = bk  * (255 - alpha);

            uint32_t ret_val = (p1 + p2) / 255;

            return static_cast<uint8_t> (ret_val);
        }

        void _merge ( const screen_data_t& top, screen_data_t& bk ) {
            
            size_t  x_max;

            for ( size_t y = 0; y < bk.size(); y++ ) {
                x_max = bk[0].line.size();
                for ( size_t x = 0; x < x_max; x++ ) {

                    bk[y].line[x].r = _process_alpha ( top[y].line[x].a, top[y].line[x].r, bk[y].line[x].r );
                    bk[y].line[x].g = _process_alpha ( top[y].line[x].a, top[y].line[x].g, bk[y].line[x].g );
                    bk[y].line[x].b = _process_alpha ( top[y].line[x].a, top[y].line[x].b, bk[y].line[x].b );

                }
            }
        }

        void _clear ( screen_data_t& img ) {
            for (size_t i = 0; i < img.size(); i++) {
                img[i].clear();
            }
        }

        bool _is_black_transparent ( const screen_pix_t& color ) {

            if ( color.a != 0 ) {  return false;  }
            if ( color.r != 0 ) {  return false;  }
            if ( color.g != 0 ) {  return false;  }
            if ( color.b != 0 ) {  return false;  }

            return true;
        }

        void _set_pix ( const geo_coord_t pos, const screen_pix_t px, screen_data_t& img ) {

            int32_t x = static_cast<int32_t> (pos.x + 0.5);
            int32_t y = static_cast<int32_t> (pos.y + 0.5);

            if ( y < img.size() ) {
                if ( x < img[y].line.size() ) {
                    img[y].line[x] = px;
                }
            }
        }

        void _get_pix ( const geo_coord_t pos, screen_pix_t& px, screen_data_t& img ) {

            int32_t x = static_cast<int32_t> (pos.x);
            int32_t y = static_cast<int32_t> (pos.y);

            if (y < img.size()) {
                if (x < img[y].line.size()) {
                    px = img[y].line[x];
                }
            }
        }

        void _line ( const geo_coord_t from, const geo_coord_t to, const screen_pix_t color, screen_data_t& img ) {

            geo_coord_t p1;
            geo_coord_t p2;

            p1.x = (int)(from.x + 0.5);
            p1.y = (int)(from.y + 0.5);
            p2.x = (int)(to.x + 0.5);
            p2.y = (int)(to.y + 0.5);

            const int deltaX = static_cast<int> ( abs(p2.x - p1.x) );
            const int deltaY = static_cast<int> ( abs(p2.y - p1.y) );
            const int signX  = (p1.x < p2.x) ? 1 : -1;
            const int signY  = (p1.y < p2.y) ? 1 : -1;
            int error1 = deltaX - deltaY;

            _set_pix ( p2, color, img );

            while ((p1.x != p2.x) || (p1.y != p2.y)) {

                _set_pix ( p1, color, img );

                int error2 = error1 * 2;

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

        void _poly_line ( const v_geo_coord_t& poly_line, const screen_pix_t color, screen_data_t& img ) {

            if (poly_line.size() < 2) {
                return;
            }

            for (size_t i = 0; i < poly_line.size() - 1; i++) {
                _line ( poly_line[i + 0], poly_line[i + 1], color, img );
            }
        }

        void _fill_solid ( const screen_pix_t clr, screen_data_t& img ) {
            for (size_t i = 0; i < img.size(); i++) {
                for (size_t y = 0; y < img[i].line.size(); y++) {
                    img[i].line[y] = clr;
                }
            }
        }

        void _fill_rect ( const geo_coord_t pt, const screen_pix_t border, const screen_pix_t fill, screen_data_t& img ) {

            std::stack<geo_coord_t> ctx;
            geo_coord_t  my_pt;
            screen_pix_t my_clr;

            ctx.push(pt);

            while ( !ctx.empty() ) {

                my_pt = ctx.top();
                ctx.pop();

                if (my_pt.y < 0) {
                    continue;
                }
                if (my_pt.y >= lines.size()) {
                    continue;
                }

                if (my_pt.x < 0) {
                    continue;
                }
                if (my_pt.x >= lines[0].line.size()) {
                    continue;
                }

                _get_pix ( my_pt, my_clr, img );

                if (my_clr == border) {
                    continue;
                }

                _set_pix ( my_pt, fill, img );

                ctx.push ( geo_coord_t(my_pt.x - 1, my_pt.y) );
                ctx.push ( geo_coord_t(my_pt.x + 1, my_pt.y) );
                ctx.push ( geo_coord_t(my_pt.x, my_pt.y - 1) );
                ctx.push ( geo_coord_t(my_pt.x, my_pt.y + 1) );
            }
        }

        void _find_fill_pos ( const screen_data_t& img, const paint_rect_t scan_rect, const screen_pix_t border, geo_coord_t& fill_pos, bool& is_valid ) {

            int32_t x, y;
            bool is_border_found = false;

            is_valid = false;

            y = static_cast<int32_t> (scan_rect.min.y);
            x = static_cast<int32_t> (scan_rect.min.x);

            while ( x < img[y].line.size() ) {

                if (img[y].line[x - 0] == border) {

                    if ( !is_border_found ) {
                        is_border_found = true;
                    } else {
                        if ( img[y].line[x - 1] != border ) {
                            is_valid = true;
                            fill_pos.y = y;
                            fill_pos.x = x-1;
                            break;
                        }
                    }

                }

                x++;
            }

        }

        void _fill_region ( const v_geo_coord_t& poly_line, const screen_pix_t border, const screen_pix_t fill, screen_data_t& img ) {

            (void) (poly_line);
            (void) (border);
            (void) (fill);
            (void) (img);

            #if 0
            paint_rect_t    draw_rect;
            paint_rect_t    scan_rect;
            geo_coord_t     fill_pos;
            bool            point_valid;
            int             step_y;

            if ( poly_line.size() < 2 ) {
                return;
            }

            _poly_line ( poly_line, border, img );

            if (poly_line.size() < 3) {
                return;
            }

            size_t cnt = poly_line.size() - 1;

            draw_rect.min.x = draw_rect.max.x = poly_line[0].x;
            draw_rect.min.y = draw_rect.max.y = poly_line[0].y;

            for (size_t i = 1; i < cnt; i++) {
                draw_rect.min.x = std::min ( draw_rect.min.x, poly_line[i].x );
                draw_rect.max.x = std::max ( draw_rect.max.x, poly_line[i].x );
                draw_rect.min.y = std::min ( draw_rect.min.y, poly_line[i].y );
                draw_rect.max.y = std::max ( draw_rect.max.y, poly_line[i].y );
            }


            step_y = static_cast<int> ( (draw_rect.max.y - draw_rect.min.y) / 4 );
            if ( step_y < 1 ) {
                step_y = 1;
            }

            scan_rect = draw_rect;
            scan_rect.min.y += 1;

            while ( scan_rect.min.y < scan_rect.max.y) {
             
                _find_fill_pos ( img, scan_rect, border, fill_pos, point_valid );
                 
                if ( point_valid ) {
                    _fill_region ( fill_pos, border, fill, img );
                }
             
                scan_rect.min.y += step_y;
            }

            #endif
        }

        int64_t _map_coord ( const geo_coord_t& coord ) {

            int64_t ret_val = 0;

            ret_val   = static_cast<int32_t> (coord.x);
            ret_val <<= 32;
            ret_val  |= static_cast<int32_t> (coord.y);

            return ret_val;
        }

        geo_coord_t _unmap_coord ( int64_t map_coord ) {

            geo_coord_t ret_val;

            ret_val.y = (int) (map_coord >> 0);
            ret_val.x = (int) (map_coord >> 32);

            return ret_val;
        }

        void _fill_region ( const geo_coord_t pos, const screen_pix_t border, const screen_pix_t fill, screen_data_t& img ) {

            std::list<geo_coord_t> process_queue;

            geo_coord_t     pt_pos;
            screen_pix_t    pt_clr;
            int32_t         x_max;
            int32_t         y_max;

            pt_pos.x = static_cast<int32_t> ( pos.x + 0.5 );
            pt_pos.y = static_cast<int32_t> ( pos.y + 0.5 );

            process_queue.push_back (pt_pos);

            x_max = static_cast<int32_t> (img[0].line.size());
            y_max = static_cast<int32_t> (img.size());

            int cnt = 0;

            while ( ! process_queue.empty() ) {

                pt_pos = process_queue.front();
                process_queue.pop_front();

                if ( ( pt_pos.x < 0 ) || (pt_pos.x >= x_max ) ) {
                    continue;
                }
                if ( ( pt_pos.y < 0 ) || (pt_pos.y >= y_max ) ) {
                    continue;
                }

                _get_pix ( pt_pos, pt_clr, img );

                if ( pt_clr == border ) {
                    continue;
                }

                if ( pt_clr != fill ) {
                    cnt++;
                    if (cnt <= 1) {
                 // if ( 1 ) {
                        _set_pix ( pt_pos, fill, img );
                        process_queue.push_back ( geo_coord_t(pt_pos.x + 1, pt_pos.y) );
                        process_queue.push_back ( geo_coord_t(pt_pos.x - 1, pt_pos.y) );
                        process_queue.push_back ( geo_coord_t(pt_pos.x, pt_pos.y + 1) );
                        process_queue.push_back ( geo_coord_t(pt_pos.x, pt_pos.y - 1) );
                    }
                }

            }
        }
};


class CMapPainter : public CStatic {

    DECLARE_DYNAMIC(CMapPainter)

    public:
        CMapPainter ();
        virtual ~CMapPainter();

    protected:
        DECLARE_MESSAGE_MAP()

    public:
        void  SetBaseParams  ( double lon, double lat, double scale );
        void  GetBaseParams  ( double& lon, double& lat, double& scale );

    public:
        afx_msg void OnPaint       ( void );
        afx_msg void OnMouseHover  ( UINT nFlags, CPoint point);
        afx_msg void OnMouseLeave  ( void );
        afx_msg void OnLButtonDown ( UINT nFlags, CPoint point );
        afx_msg void OnLButtonUp   ( UINT nFlags, CPoint point );
        afx_msg void OnMouseMove   ( UINT nFlags, CPoint point );
        afx_msg BOOL OnEraseBkgnd  ( CDC* pDC );

    private:
        void _paint_map ( const list_geo_record_t& draw_list, CPaintDC& dc, CRect& clientRect );

    private:
        void _flood_fill ( int x, int y, int fill_color, int border_color );
        void _map_color ( obj_type_t geo_type, screen_pix_t& border_color, screen_pix_t& fill_color );
        bool _is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice );
        void _load_idx ( vector_geo_idx_rec_t& idx_list );
        void _find_base_rect( const vector_geo_idx_rec_t& idx_list, geo_rect_t& base_rect );
        void _find_rects ( const vector_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, vector_uint32_t& out_list );
        void _merge ( const vector_geo_idx_rec_t& rect_list, const vector_uint32_t& idx_list, vector_uint32_t& map_entries );
        void _load_map ( const vector_uint32_t& map_entries, list_geo_record_t& map_items );
        void _ajust_minus ( const geo_rect_t& base_rect, double scale_x, double scale_y, v_geo_coord_t& path );
        void _ajust_plus ( const geo_rect_t& base_rect, double scale_x, double scale_y, v_geo_coord_t& path );
        void _trim_record ( const geo_rect_t& window_rect, const geo_record_t& src_record, geo_record_t& dst_record, bool& trim_res );
        void _trim_map ( const geo_rect_t& window_rect, const list_geo_record_t& src_map, list_geo_record_t& dst_map );
        void _geo_to_window (const geo_rect_t& window_rect, double scale_x, double scale_y, list_geo_record_t& list_rec );
        void _test_range ( const list_geo_record_t& list_rec );
        void _process_area ( geo_record_t& record );
        void _process_areas ( list_geo_record_t& geo_records );
        void test ( const geo_rect_t& rect );

    private:
        bool			m_bMouseTracking;
        CPoint			m_PickPoint;
        bool			m_DragActive;
        CPoint			m_BasePosition;
        int				m_DeltaX;
        int				m_DeltaY;

        double          m_lon;
        double          m_lat;
        double          m_scale;

        double          m_delta_hor;
        double          m_delta_ver;

        CPaintDC*       m_paint_dc;
        CRect           m_client_rect;

        screen_t        m_screen;
};


