#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <string.h>
#include <cassert>

#include "bstring.h"
#include "libhpxml.h"
#include "FileStorage.h"
#include "Lex.h"

#include "..\common\osm_idx.h"
#include "..\common\osm_types.h"


#define OSM_TYPE_PREV     ( (int)(g_xml_ctx[g_xml_ctx_cnt-2]) )
#define OSM_TYPE_CURR     ( (int)(g_xml_ctx[g_xml_ctx_cnt-1]) )
#define MAP_TYPE(p,c)     ( ((int)(p)<<16) + ((int)(c)) )
#define GEO_SCALE         ( 10000000 )

typedef enum tag_osm_node {
    XML_NODE_UNDEF,
    XML_NODE_ROOT,
    XML_NODE_NODE,
    XML_NODE_WAY,
    XML_NODE_REL,
    XML_NODE_TAG,
    XML_NODE_ND,
    XML_NODE_MEMBER,
    XML_NODE_SKIP,
}   osm_node_t;

// XML call stack
static osm_node_t           g_xml_ctx[4];
static int                  g_xml_ctx_cnt = 0;

// XML tags assigned to NODE
static osm_tag_ctx_t        g_xml_tags = { 0 };

static idx_t                g_node_idx;
static osm_node_info_t      g_node_info;
static uint64_t             g_file_node_len;

static osm_way_info_t       g_way_info;
static idx_t                g_way_idx;

static osm_rel_info_t       g_rel_info;
static idx_t                g_rel_idx;

Lex                         g_lexer;
FileStorage                 g_nodes;
FileStorage                 g_ways;
FileStorage                 g_names;

static void _cp_val ( const hpx_attr_t& src, alloc_str_t& dst ) {

    strncpy ( dst, src.value.buf, src.value.len );
    dst[src.value.len] = 0;
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

static void _process_node_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

    int s = g_xml_tags.pos_1;
    g_xml_tags.pos_1++;

    assert (g_xml_tags.pos_1 < OSM_MAX_TAGS_CNT );

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "k") == 0) {
            _cp_val( attr_list[i], g_xml_tags.list[s].k );
        } else
        if (bs_cmp(attr_list[i].name, "v") == 0) {
            _cp_val ( attr_list[i], g_xml_tags.list[s].v );
        }
    }
}

static void _resolve_node_type ( void ) {

}

static void _process_root_way ( int attr_cnt, const hpx_attr_t* attr_list ) {

}

static void _process_way_nd ( int attr_cnt, const hpx_attr_t* attr_list ) {

}

static void _process_way_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

}

static void _resolve_way_type ( void ) {

}

static void _process_root_rel ( int attr_cnt, const hpx_attr_t* attr_list ) {

}

static void _process_rel_member ( int attr_cnt, const hpx_attr_t* attr_list ) {

}

static void _process_rel_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

}

static void _resolve_rel_type ( void ) {

}

static void _osm_push ( osm_node_t next_node ) {
              
    g_xml_ctx[g_xml_ctx_cnt] = next_node;
    g_xml_ctx_cnt++;
}

static void _osm_pop ( osm_node_t osm_node ) {

    g_xml_ctx_cnt--;

    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_NODE ) {
        _resolve_node_type();
        g_node_info.len = sizeof(g_node_info);
        g_nodes.Store( g_node_info.id, &g_node_info, sizeof(g_node_info) );
        _clean_ctx(g_xml_tags);
        memset ( &g_node_info, 0, sizeof(g_node_info) );

    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_WAY ) {
        _resolve_way_type();
        _clean_ctx(g_xml_tags);
        memset ( &g_way_info, 0, sizeof(g_way_info) );

    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_REL ) {
        _resolve_rel_type();
        _clean_ctx(g_xml_tags);
        memset ( &g_rel_info, 0, sizeof(g_rel_info) );

    }

    g_xml_ctx[g_xml_ctx_cnt] = XML_NODE_UNDEF;
}

static void _process_open ( int attr_cnt, const hpx_attr_t* attr_list ) {

    switch (MAP_TYPE(OSM_TYPE_PREV, OSM_TYPE_CURR)) {


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_NODE):
            _process_root_node(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_NODE, XML_NODE_TAG):
            _process_node_tag(attr_cnt, attr_list);
            break;


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_WAY):
            _process_root_way(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_WAY, XML_NODE_ND):
            _process_way_nd(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_WAY, XML_NODE_TAG):
            _process_way_tag(attr_cnt, attr_list);
            break;


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_REL):
            _process_root_rel(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_REL, XML_NODE_MEMBER):
            _process_rel_member(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_REL, XML_NODE_TAG):
            _process_rel_tag(attr_cnt, attr_list);
            break;


        default:
            assert(false);
    }

}

static void _process_item ( int xml_type, const bstring_t& name, int attr_cnt, const hpx_attr_t* attr_list ) {

    osm_node_t osm_node = XML_NODE_UNDEF;

    if (bs_cmp(name, "node") == 0) {
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

int main ( int argc, char* argv[] ) {

    hpx_ctrl_t*     ctrl;
    hpx_tag_t*      xml_obj;
    bstring_t       xml_str;
    long            lno;
    int             fd;
    int             io_res;

    fd = open ( "D:\\OSM\\ohrada.osm", O_RDONLY);

    g_lexer.Load ("tag.config");
    g_nodes.Create ( "node" );
    g_ways.Create  ( "ways" );

    ctrl = hpx_init(fd, 100 * 1024 * 1024);
    if (ctrl == NULL) {
        return -1;
    }

    xml_obj = hpx_tm_create(64);
    if (xml_obj == NULL) {
        return -2;
    }

    g_xml_ctx[0] = XML_NODE_ROOT;
    g_xml_ctx_cnt = 1;

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

        _process_item(xml_obj->type, xml_obj->tag, xml_obj->nattr, xml_obj->attr);
    }

    if ( ! ctrl->eof ) {
        assert ( false );
        return -3;
    }

    g_nodes.Close();
    g_ways.Close();

    hpx_tm_free(xml_obj);
    hpx_free(ctrl);

    close(fd);

    return 0;
}
