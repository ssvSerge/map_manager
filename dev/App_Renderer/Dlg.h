#pragma once

#include "MapPainter.h"

#define WM_USER_MOVE_ENTER      (WM_USER + 10)
#define WM_USER_MOVE            (WM_USER + 11)
#define WM_USER_MOVE_LEAVE      (WM_USER + 12)


class CAppRendererDlg : public CDialogEx {

	public:
		CAppRendererDlg(CWnd* pParent = nullptr);

		#ifdef AFX_DESIGN_TIME
			enum { IDD = IDD_APP_REMDERER_DIALOG };
		#endif

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnInitDialog();

	protected:
		HICON m_hIcon;

	private:
		void   UpdateText    ( double val, CEdit& edit );
		void   MapRedraw     ( void );

	public:
		DECLARE_MESSAGE_MAP()
		afx_msg LRESULT OnMapUpdate(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnUserMove(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnUserMoveEnter(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnUserMoveLeave(WPARAM wParam, LPARAM lParam);
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnPaint();
		afx_msg void OnBnClickedTest();
		afx_msg void OnBnClickedZoomIn();
		afx_msg void OnBnClickedZoomOut();
		afx_msg void OnEnUpdateEditLon();
		afx_msg void OnEnUpdateEditLat();
		afx_msg void OnEnUpdateEditScale();
		afx_msg void OnBnClickedCmdMap();
		afx_msg void OnBnClickedCmdZoomOut();
		afx_msg void OnBnClickedCmdZoomIn();
		afx_msg void OnBnClickedCmdAngleMinus();
		afx_msg void OnBnClickedCmdAnglePlus();

		CEdit			m_EditLon;
		CEdit			m_EditLat;
		CEdit			m_EditScale;
		CEdit			m_EditAngle;
		CMapPainter		m_MapRender;
		double          m_base_scale;
		double          m_base_angle;
		double          m_base_lon;
		double          m_base_lat;

		double          m_shift_lon;
		double          m_shift_lat;

		bool            m_drag_active;
		afx_msg void OnBnClickedCmdFindObject();
};
