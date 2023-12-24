#pragma once

#define WM_MAP_UPDATE (WM_USER + 1)

#include <stdint.h>

#include <vector>
#include <cmath>
#include <stack>
#include <list>
#include <set>
#include <algorithm>

#include <geo_types.h>
#include <geo_processor.h>


extern geo_processor_t    g_geo_processor;


class CMapPainter : public CStatic {

    DECLARE_DYNAMIC(CMapPainter)

    public:
        CMapPainter ();
        virtual ~CMapPainter();

    protected:
        DECLARE_MESSAGE_MAP()

    public:
        void  SetBaseParams  ( double lon, double lat, double scale, double angle );

    public:
        afx_msg void OnPaint       ( void );
        afx_msg void OnLButtonDown ( UINT nFlags, CPoint point );
        afx_msg void OnLButtonUp   ( UINT nFlags, CPoint point );
        afx_msg void OnMouseMove   ( UINT nFlags, CPoint point );
        afx_msg BOOL OnSetCursor   ( CWnd* pWnd, UINT nHitTest, UINT message);
        afx_msg BOOL OnEraseBkgnd  ( CDC* pDC );

    public:
        int32_t         m_drag_x;
        int32_t         m_drag_y;

    private:
        bool			m_bMouseTracking;
        bool			m_DragActive;
        CPoint			m_BasePosition;
        int				m_DeltaX;
        int				m_DeltaY;

        double          m_scale;
        double          m_angle;

        double          m_delta_hor;
        double          m_delta_ver;

        CPaintDC*       m_paint_dc;
        CRect           m_client_rect;
        CPoint          m_DragBasePos;

        int             m_cursor_type;

};


