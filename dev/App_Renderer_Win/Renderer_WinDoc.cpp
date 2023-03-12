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


#define OSM_TYPE_PREV     ( (int)(g_xml_ctx[g_xml_ctx_cnt-2]) )
#define OSM_TYPE_CURR     ( (int)(g_xml_ctx[g_xml_ctx_cnt-1]) )
#define MAP_TYPE(p,c)     ( ((int)(p)<<16) + ((int)(c)) )
#define GEO_SCALE         ( 10000000 )


osm_node_t          g_xml_ctx[4];
int                 g_xml_ctx_cnt = 0;
osm_tag_ctx_t       g_xml_tags    = { 0 };
osm_node_info_t     g_node_info   = { 0 };

static void _osm_push(osm_node_t next_node) {

    g_xml_ctx[g_xml_ctx_cnt] = next_node;
    g_xml_ctx_cnt++;
}

static void _process_root_node ( int attr_cnt, const hpx_attr_t* attr_list ) {

    double val;

    for (int i = 0; i < attr_cnt; i++) {

        if (bs_cmp(attr_list[i].name, "id") == 0) {
            g_node_info.id = bs_tol(attr_list[i].value );
        } else
        if (bs_cmp(attr_list[i].name, "lat") == 0) {
            val = bs_tod(attr_list[i].value);
            g_node_info.lat = (osm_lat_t) ( val * GEO_SCALE );
        } else
        if (bs_cmp(attr_list[i].name, "lon") == 0) {
            val = bs_tod(attr_list[i].value);
            g_node_info.lon = (osm_lon_t)( val * GEO_SCALE );
        }

    }
}


static void _process_open ( int attr_cnt, const hpx_attr_t* attr_list ) {

    switch ( MAP_TYPE(OSM_TYPE_PREV, OSM_TYPE_CURR) ) {


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_NODE):
            _process_root_node(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_NODE, XML_NODE_TAG):
            // _process_node_tag(attr_cnt, attr_list);
            break;


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_WAY):
            // _process_root_way(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_WAY, XML_NODE_ND):
            // _process_way_nd(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_WAY, XML_NODE_TAG):
            // _process_way_tag(attr_cnt, attr_list);
            break;


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_REL):
            // _process_root_rel(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_REL, XML_NODE_MEMBER):
            // _process_rel_member(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_REL, XML_NODE_TAG):
            // _process_rel_tag(attr_cnt, attr_list);
            break;

        default:
            assert(false);
    }

}

static void _osm_pop (osm_node_t osm_node ) {

    g_xml_ctx_cnt--;

    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_NODE ) {
        // expand_node_tags();    // 
        // resolve_node_type();   // 
        store_node_info();     // 
        clean_node_info();     // 
    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_WAY ) {
        // ways_expand_tags();
        // ways_resolve_type();
        // ways_store_info();
        // ways_clean_info();
    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_REL ) {
        // expand_rel_tags();
        // resolve_rel_type();
        // store_rel_info();
    }

    g_xml_ctx[g_xml_ctx_cnt] = XML_NODE_UNDEF;
}



void process_item (int xml_type, const bstring_t& name, int attr_cnt, const hpx_attr_t* attr_list ) {

    osm_node_t osm_node = XML_NODE_UNDEF;

    if ( bs_cmp(name, "node") == 0) {
        osm_node = XML_NODE_NODE;
    } else
    if (bs_cmp(name, "tag") == 0) {
        osm_node = XML_NODE_TAG;
    } else
    if (bs_cmp(name, "way") == 0) {
        osm_node = XML_NODE_WAY;
    } else
    if (bs_cmp(name, "relation") == 0) {
        osm_node = XML_NODE_REL;
    } else
    if (bs_cmp(name, "member") == 0) {
        osm_node = XML_NODE_MEMBER;
    } else
    if (bs_cmp(name, "nd") == 0) {
        osm_node = XML_NODE_ND;
    } else
    if (bs_cmp(name, "osm") == 0) {
        osm_node = XML_NODE_SKIP;
    } else
    if (bs_cmp(name, "bounds") == 0) {
        osm_node = XML_NODE_SKIP;
    } else
    if (bs_cmp(name, "xml") == 0) {
        osm_node = XML_NODE_SKIP;
    }

    assert(osm_node != XML_NODE_UNDEF);

    if (osm_node == XML_NODE_SKIP) {
        return;
    }

    switch (xml_type) {

        case HPX_OPEN: // open tag
            _osm_push(osm_node);
            _process_open(attr_cnt, attr_list);
            break;

        case HPX_SINGLE: // open tag + close tag
            _osm_push(osm_node);
            _process_open(attr_cnt, attr_list);
            _osm_pop(osm_node);
            break;

        case HPX_CLOSE:
            _osm_pop(osm_node);
            break;

        case HPX_INSTR:
        case HPX_COMMENT:
            break;

        case HPX_ILL:
        case HPX_LITERAL:
        case HPX_ATT:
            assert(false);
            break;
    }

}


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

	int				fd = 0;
	hpx_ctrl_t*		ctrl = nullptr;
	hpx_tag_t*		xml_obj = nullptr;
	bstring_t		xml_str;
	long			lno;
	int				io_res;

	_sopen_s ( &fd, lpszPathName, _O_BINARY|_O_RDONLY, _SH_DENYWR, _S_IREAD );
	if (fd == -1) {
		return -1;
	}

	ctrl = hpx_init(fd, 100 * 1024 * 1024);
	if (ctrl == NULL) {
		return -1;
	}

	xml_obj = hpx_tm_create(64);
	if (xml_obj == NULL) {
		return -2;
	}

    g_xml_ctx[0] = XML_NODE_ROOT;
    g_xml_ctx_cnt++;
    
    for ( ; ; ) {

		io_res = hpx_get_elem(ctrl, &xml_str, NULL, &lno);
		if (io_res == 0) {
			break;
		}

		io_res = hpx_process_elem(xml_str, xml_obj);
		if (io_res != 0) {
			printf("[%ld] ERROR in element: %.*s\n", lno, xml_str.len, xml_str.buf);
			break;
		}

		process_item ( xml_obj->type, xml_obj->tag, xml_obj->nattr, xml_obj->attr );
	}

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
