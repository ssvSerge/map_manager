#include "pch.h"
#include "framework.h"
#include "App.h"
#include "Dlg.h"
#include "afxdialogex.h"

#define  GEO_PRECISION          "%.7f"

#ifdef _DEBUG
    #define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CAppRendererDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(ID_TEST,       &CAppRendererDlg::OnBnClickedTest)
    ON_BN_CLICKED(IDC_ZOOM_IN,   &CAppRendererDlg::OnBnClickedZoomIn)
    ON_BN_CLICKED(IDC_ZOOM_OUT,  &CAppRendererDlg::OnBnClickedZoomOut)
    ON_EN_UPDATE(IDC_EDIT_LON,   &CAppRendererDlg::OnEnUpdateEditLon)
    ON_EN_UPDATE(IDC_EDIT_LAT,   &CAppRendererDlg::OnEnUpdateEditLat)
    ON_EN_UPDATE(IDC_EDIT_SCALE, &CAppRendererDlg::OnEnUpdateEditScale)
    ON_BN_CLICKED(IDC_CMD_MAP,   &CAppRendererDlg::OnBnClickedCmdMap)
    ON_MESSAGE(WM_MAP_UPDATE,    &CAppRendererDlg::OnMapUpdate)
END_MESSAGE_MAP()

CAppRendererDlg::CAppRendererDlg ( CWnd* pParent /*=nullptr*/ ) : CDialogEx(IDD_APP_REMDERER_DIALOG, pParent) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAppRendererDlg::DoDataExchange ( CDataExchange* pDX ) {

    CDialogEx::DoDataExchange(pDX);

    DDX_Control ( pDX, IDC_EDIT_LON,    m_EditLon   );
    DDX_Control ( pDX, IDC_EDIT_LAT,    m_EditLat   );
    DDX_Control ( pDX, IDC_EDIT_SCALE,  m_EditScale );
    DDX_Control ( pDX, IDC_EDIT_ANGLE,  m_EditAngle );
    DDX_Control ( pDX, IDC_MAP_PAINTER, m_MapRender );
}

BOOL CAppRendererDlg::OnInitDialog () {

    CDialogEx::OnInitDialog ();

    SetIcon ( m_hIcon, TRUE );
    SetIcon ( m_hIcon, FALSE );

    UpdateText ( 14.3385, m_EditLon   );
    UpdateText ( 50.0368, m_EditLat   );
    UpdateText ( 1.00000, m_EditScale );
    UpdateText ( 0,       m_EditAngle );

    // UpdateParams();

    return TRUE;
}

void CAppRendererDlg::UpdateParams ( void ) {

    CString val;

    m_EditLon.GetWindowText(val);
    double lon = atof ( val.GetBuffer() );

    m_EditLat.GetWindowText(val);
    double lat = atof(val.GetBuffer());

    m_EditScale.GetWindowText(val);
    double scale = atof(val.GetBuffer());

    m_EditAngle.GetWindowText(val);
    double angle = atof(val.GetBuffer());

    m_MapRender.SetBaseParams ( lon, lat, scale, angle );
}

void CAppRendererDlg::UpdateText ( double val, CEdit& edit ) {

    char fmt_text[80];

    sprintf_s ( fmt_text, sizeof(fmt_text), GEO_PRECISION, val );
    edit.SetWindowText(fmt_text);
}

void CAppRendererDlg::OnPaint () {

    if ( IsIconic() ) {

        CPaintDC dc(this);

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        dc.DrawIcon(x, y, m_hIcon);

    } else {
        CDialogEx::OnPaint();
    }
}

HCURSOR CAppRendererDlg::OnQueryDragIcon () {
    return static_cast<HCURSOR>(m_hIcon);
}

void CAppRendererDlg::OnBnClickedTest () {
    CDialogEx::OnCancel();
}

void CAppRendererDlg::OnBnClickedZoomIn () {

    CString str;
    m_EditScale.GetWindowText(str);

    double val = atof(str.GetBuffer());

    val *= 1.003;

    str.Format(GEO_PRECISION, val);
    m_EditScale.SetWindowText(str);
}

void CAppRendererDlg::OnBnClickedZoomOut () {

    CString str;
    m_EditScale.GetWindowText(str);

    double val = atof(str.GetBuffer());

    val /= 1.003;

    str.Format( GEO_PRECISION, val);
    m_EditScale.SetWindowText(str);
}

void CAppRendererDlg::OnEnUpdateEditLon () {
}
                             
void CAppRendererDlg::OnEnUpdateEditLat () {
}

void CAppRendererDlg::OnEnUpdateEditScale () {
}

void CAppRendererDlg::OnBnClickedCmdMap () {

    double lon   = 0;
    double lat   = 0;
    double scale = 0;
    double angle = 0;

    CString val;

    m_EditLon.GetWindowText ( val );
    lon = atof(val);

    m_EditLat.GetWindowText   ( val );
    lat = atof(val);

    m_EditScale.GetWindowText ( val );
    scale = atof(val);

    m_EditAngle.GetWindowText ( val );
    angle = atof(val);

    m_MapRender.SetBaseParams ( lon, lat, scale, angle );

    m_MapRender.Invalidate();
    m_MapRender.UpdateWindow();
}

LRESULT CAppRendererDlg::OnMapUpdate ( WPARAM wParam, LPARAM lParam ) {

    double lon;
    double lat;
    double scale;

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    m_MapRender.GetBaseParams ( lon, lat, scale );

    UpdateText(lon, m_EditLon);
    UpdateText(lat, m_EditLat);

    return 0;
}
