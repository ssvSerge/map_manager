
// Renderer_WinView.cpp : implementation of the CRendererWinView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Renderer_Win.h"
#endif

#include "Renderer_WinDoc.h"
#include "Renderer_WinView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRendererWinView

IMPLEMENT_DYNCREATE(CRendererWinView, CView)

BEGIN_MESSAGE_MAP(CRendererWinView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CRendererWinView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CRendererWinView construction/destruction

CRendererWinView::CRendererWinView() noexcept
{
	// TODO: add construction code here

}

CRendererWinView::~CRendererWinView()
{
}

BOOL CRendererWinView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CRendererWinView drawing

void CRendererWinView::OnDraw(CDC* /*pDC*/)
{
	CRendererWinDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CRendererWinView printing


void CRendererWinView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CRendererWinView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CRendererWinView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CRendererWinView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CRendererWinView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CRendererWinView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CRendererWinView diagnostics

#ifdef _DEBUG
void CRendererWinView::AssertValid() const
{
	CView::AssertValid();
}

void CRendererWinView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CRendererWinDoc* CRendererWinView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRendererWinDoc)));
	return (CRendererWinDoc*)m_pDocument;
}
#endif //_DEBUG


// CRendererWinView message handlers
