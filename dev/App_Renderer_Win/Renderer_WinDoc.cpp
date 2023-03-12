#include "pch.h"
#include "framework.h"

#include <io.h>
#include <fcntl.h>

#include "..\common\libhpxml.h"

#ifndef SHARED_HANDLERS
	#include "Renderer_Win.h"
#endif

#include "Renderer_WinDoc.h"
#include <propkey.h>

#include "..\types\osm_types.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CRendererWinDoc, CDocument)

BEGIN_MESSAGE_MAP(CRendererWinDoc, CDocument)
END_MESSAGE_MAP()


CRendererWinDoc::CRendererWinDoc() noexcept {
}

CRendererWinDoc::~CRendererWinDoc() {
}

BOOL CRendererWinDoc::OnNewDocument() {

	if ( !CDocument::OnNewDocument() ) {
		return FALSE;
	}

	return TRUE;
}

BOOL CRendererWinDoc::OnOpenDocument ( LPCTSTR lpszPathName ) {

	return true;
}

void CRendererWinDoc::Serialize(CArchive& ar) {
}

#ifdef _DEBUG
void CRendererWinDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRendererWinDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif
