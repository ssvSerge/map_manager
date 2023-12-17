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

class CMapPainter : public CStatic {

    DECLARE_DYNAMIC(CMapPainter)

    public:
        CMapPainter ();
        virtual ~CMapPainter();

    protected:
        DECLARE_MESSAGE_MAP()

    public:
        void  SetBaseParams  ( double lon, double lat, double scale, double angle ) const;
        void  GetBaseParams  ( double& lon, double& lat, double& scale ) const;

    public:
        afx_msg void OnPaint       ( void );
        afx_msg void OnSize        ( UINT nType, int cx, int cy );
        afx_msg void OnMouseHover  ( UINT nFlags, CPoint point);
        afx_msg void OnMouseLeave  ( void );
        afx_msg void OnLButtonDown ( UINT nFlags, CPoint point );
        afx_msg void OnLButtonUp   ( UINT nFlags, CPoint point );
        afx_msg void OnMouseMove   ( UINT nFlags, CPoint point );
        afx_msg BOOL OnEraseBkgnd  ( CDC* pDC );

    private:
        void _calc_geo ( double lon, double lat, double scale ) const;

        void _video_init ( void );
        void _video_render ( void );

    private:
        bool            m_isVideoActive;
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
};


