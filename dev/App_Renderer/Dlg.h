#pragma once

#include "MapPainter.h"

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
		void   UpdateText ( double val, CEdit& edit );
		void   UpdateParams ( void );

	public:
		DECLARE_MESSAGE_MAP()
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnPaint();
		afx_msg void OnBnClickedTest();
		afx_msg void OnBnClickedZoomIn();
		afx_msg void OnBnClickedZoomOut();
		afx_msg void OnEnUpdateEditLon();
		afx_msg void OnEnUpdateEditLat();
		afx_msg void OnEnUpdateEditScale();
		afx_msg void OnBnClickedCmdMap();
		afx_msg LRESULT OnMapUpdate(WPARAM wParam, LPARAM lParam);

		CEdit			m_EditLon;
		CEdit			m_EditLat;
		CEdit			m_EditScale;
		CMapPainter		m_MapRender;
};
