#pragma once

#define WM_MAP_UPDATE (WM_USER + 1)

#include <geo_types.h>

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
        void _set_pixel ( const int32_t x2, const int32_t y2, const geo_color_t color );
        void _draw_line ( const geo_coord_t p1, const geo_coord_t p2, const geo_color_t color );
        void _draw_poly_line ( const v_geo_coord_t& poly_line, const geo_color_t color );
        void _paint_map ( const list_geo_record_t& draw_list, CPaintDC& dc, CRect& clientRect );

    private:
        void _map_color ( obj_type_t geo_type, geo_color_t& geo_color );
        bool _is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice );
        void _load_idx ( vector_geo_idx_rec_t& idx_list );
        void _find_base_rect( const vector_geo_idx_rec_t& idx_list, geo_rect_t& base_rect );
        void _find_rects ( const vector_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, vector_uint_t& out_list );
        void _merge ( const vector_geo_idx_rec_t& rect_list, const vector_uint_t& idx_list, vector_uint_t& map_entries );
        void _load_map ( const vector_uint_t& map_entries, list_geo_record_t& map_items );
        void _ajust_minus ( const geo_rect_t& base_rect, double scale_x, double scale_y, v_geo_coord_t& path );
        void _ajust_plus ( const geo_rect_t& base_rect, double scale_x, double scale_y, v_geo_coord_t& path );
        void _trim_record ( const geo_rect_t& base_rect, const geo_rect_t& window_rect, const geo_record_t& src_record, geo_record_t& dst_record, bool& trim_res );
        void _trim_map ( const geo_rect_t& base_rect, const geo_rect_t& window_rect, const list_geo_record_t& src_map, list_geo_record_t& dst_map );
        void _geo_to_window (const geo_rect_t& window_rect, double scale_x, double scale_y, list_geo_record_t& list_rec );
        void _test_range ( const list_geo_record_t& list_rec );
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

        double          m_delta_lat;
        double          m_delta_lon;

        CPaintDC*       m_paint_dc;
};


