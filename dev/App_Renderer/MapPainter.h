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
        bool _is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice );
        void _load_idx ( void );
        void _find_rects ( const geo_rect_t& base_rect, vector_uint_t& out_list );
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
};


