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
    ON_BN_CLICKED(ID_TEST,              &CAppRendererDlg::OnBnClickedTest)
    ON_BN_CLICKED(IDC_ZOOM_IN,          &CAppRendererDlg::OnBnClickedZoomIn)
    ON_BN_CLICKED(IDC_ZOOM_OUT,         &CAppRendererDlg::OnBnClickedZoomOut)
    ON_EN_UPDATE(IDC_EDIT_LON,          &CAppRendererDlg::OnEnUpdateEditLon)
    ON_EN_UPDATE(IDC_EDIT_LAT,          &CAppRendererDlg::OnEnUpdateEditLat)
    ON_EN_UPDATE(IDC_EDIT_SCALE,        &CAppRendererDlg::OnEnUpdateEditScale)
    ON_BN_CLICKED(IDC_CMD_MAP,          &CAppRendererDlg::OnBnClickedCmdMap)
    ON_MESSAGE(WM_MAP_UPDATE,           &CAppRendererDlg::OnMapUpdate)
    ON_BN_CLICKED(IDC_CMD_ZOOM_OUT,     &CAppRendererDlg::OnBnClickedCmdZoomOut)
    ON_BN_CLICKED(IDC_CMD_ZOOM_IN,      &CAppRendererDlg::OnBnClickedCmdZoomIn)
    ON_BN_CLICKED(IDC_CMD_ANGLE_MINUS,  &CAppRendererDlg::OnBnClickedCmdAngleMinus)
    ON_BN_CLICKED(IDC_CMD_ANGLE_PLUS,   &CAppRendererDlg::OnBnClickedCmdAnglePlus)
    ON_MESSAGE(WM_USER_MOVE_ENTER,      &CAppRendererDlg::OnMouseClickDown)
    ON_MESSAGE(WM_USER_MOVE,            &CAppRendererDlg::OnUserMove)
    ON_MESSAGE(WM_USER_MOVE_LEAVE,      &CAppRendererDlg::OnMouseClickUp)
    ON_BN_CLICKED(IDC_CMD_FIND_OBJECT,  &CAppRendererDlg::OnBnClickedCmdFindObject)
END_MESSAGE_MAP()


CAppRendererDlg::CAppRendererDlg ( CWnd* pParent /*=nullptr*/ ) : CDialogEx(IDD_APP_REMDERER_DIALOG, pParent) {

    m_base_scale  = 1;
    m_base_lon    = 0;
    m_base_lat    = 0;
    m_shift_lon   = 0;
    m_shift_lat   = 0;
    m_base_angle  = 0;
    m_mouse_down  = false;
    m_mouse_drag  = true;

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

void CAppRendererDlg::MapRedraw ( void ) {

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

    m_EditLat.GetWindowText ( val );
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

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return 0;
}

void CAppRendererDlg::OnBnClickedCmdZoomOut() {

    CString val;
    double scale = 0;

    m_EditScale.GetWindowText(val);
    scale = atof(val);

    if ( scale > 0.1 ) {
        scale *= 0.99;
    }

    val.Format("%f", scale);
    m_EditScale.SetWindowText(val);

    OnBnClickedCmdMap();
}

void CAppRendererDlg::OnBnClickedCmdZoomIn() {

    CString val;
    double scale = 0;

    m_EditScale.GetWindowText(val);
    scale = atof(val);

    if (scale < 50) {
        scale *= 1.01;
    }

    val.Format("%f", scale);
    m_EditScale.SetWindowText(val);

    OnBnClickedCmdMap();
}

void CAppRendererDlg::OnBnClickedCmdAngleMinus() {

    CString val;
    double angle = 0;

    m_EditAngle.GetWindowText(val);
    angle = atof(val);

    angle += 1;

    if (angle >= 360) {
        angle = 0;
    }

    val.Format("%f", angle);
    m_EditAngle.SetWindowText(val);

    OnBnClickedCmdMap();
}

void CAppRendererDlg::OnBnClickedCmdAnglePlus() {

    CString val;
    double angle = 0;

    m_EditAngle.GetWindowText(val);
    angle = atof(val);

    angle -= 1;

    if (angle < 0) {
        angle = 359;
    }

    val.Format("%f", angle);
    m_EditAngle.SetWindowText(val);

    OnBnClickedCmdMap();
}

LRESULT CAppRendererDlg::OnUserMove ( WPARAM wParam, LPARAM lParam ) {

    (void) (wParam);
    (void) (lParam);

    if ( m_mouse_down ) {

        CString  str_lon;
        CString  str_lat;

        double  curr_lon    = m_base_lon;
        double  curr_lat    = m_base_lat;
        double  step_geo_x  = 0;
        double  step_geo_y  = 0;
        double  shift_geo_x = 0;
        double  shift_geo_y = 0;

        m_mouse_drag = true;

        g_geo_processor.get_shifts ( curr_lat, curr_lon, step_geo_x, step_geo_y );

        shift_geo_x = m_MapRender.m_drag_x * step_geo_x / m_base_scale / 2;
        shift_geo_y = m_MapRender.m_drag_y * step_geo_y / m_base_scale / 2;

        m_shift_lon = m_base_lon + shift_geo_x;
        m_shift_lat = m_base_lat + shift_geo_y;

        str_lon.Format ( "%.7f", m_shift_lon );
        str_lat.Format ( "%.7f", m_shift_lat );
        
        m_EditLon.SetWindowText ( str_lon );
        m_EditLat.SetWindowText ( str_lat );

        m_MapRender.SetBaseParams ( m_shift_lon, m_shift_lat, m_base_scale, m_base_angle );

        m_MapRender.Invalidate();
        m_MapRender.UpdateWindow();

    }

    return 0;
}

LRESULT CAppRendererDlg::OnMouseClickDown ( WPARAM wParam, LPARAM lParam ) {

    CString val;

    (void)(wParam);
    (void)(lParam);

    m_mouse_down = true;
    m_mouse_drag = false;

    m_EditLon.GetWindowText(val);
    m_base_lon = atof(val);

    m_EditLat.GetWindowText(val);
    m_base_lat = atof(val);

    m_EditScale.GetWindowText(val);
    m_base_scale = atof(val);

    m_EditAngle.GetWindowText(val);
    m_base_angle = atof(val);

    return 0;
}

LRESULT CAppRendererDlg::OnMouseClickUp ( WPARAM wParam, LPARAM lParam ) {

    (void)(wParam);
    (void)(lParam);

    if ( !m_mouse_drag ) {
        FindObject();
    }

    m_mouse_down = false;
    m_mouse_drag = false;

    return 0;
}

void CAppRendererDlg::OnBnClickedCmdFindObject() {

    m_MapRender.m_HighlightsList.clear();

    m_MapRender.Invalidate(true);
    m_MapRender.UpdateWindow();

    return;
}

void CAppRendererDlg::FindObject ( void ) {

    CRect      mapDrawRect;
    CPoint     basePoint;
    CPoint     clickPoint;
    uint32_t   w;
    uint32_t   h;

    m_MapRender.GetClientRect(mapDrawRect);

    w = mapDrawRect.Width();
    h = mapDrawRect.Height();

    basePoint.y = h - (h / 10);
    basePoint.x = w /  2;

    m_MapRender.m_HighlightsList.push_back(basePoint);
    m_MapRender.m_HighlightsList.push_back(m_MapRender.m_ClickPos);

    m_MapRender.Invalidate(true);
    m_MapRender.UpdateWindow();

    return;
}
