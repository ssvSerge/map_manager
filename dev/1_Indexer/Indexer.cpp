#define PY_SSIZE_T_CLEAN

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <string.h>
#include <cassert>
#include <vector>

#include "..\common\osm_idx.h"
#include "..\common\osm_types.h"

#include "bstring.h"
#include "libhpxml.h"
#include "FileStorage.h"
#include "OsmLex.h"

#define ITEMS_CNT(x)         ( ( sizeof(x) ) / ( sizeof (x[0]) ) )


#define OSM_TYPE_PREV     ( (int)(g_xml_ctx[g_xml_ctx_cnt-2]) )
#define OSM_TYPE_CURR     ( (int)(g_xml_ctx[g_xml_ctx_cnt-1]) )
#define MAP_TYPE(p,c)     ( ((int)(p)<<16) + ((int)(c)) )
#define GEO_SCALE         ( 10000000 )
#define LINK_CLUSTER      ( 1024 * 1024 )

typedef std::vector<link_info_t>  ref_list_t;

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

FileStorage                 g_nodes;
FileStorage                 g_ways;
FileStorage                 g_names;

ref_list_t                  g_ref_list;

extern void _clean_ctx(osm_tag_ctx_t& ctx);

static void _cp_val(const hpx_attr_t& src, alloc_str_t& dst) {

    strncpy(dst, src.value.buf, src.value.len);
    dst[src.value.len] = 0;
}

static bool find_in_skip_list ( const hpx_attr_t* new_item ) {

    const char* skip_list[] = {
        "source",
        "barrier",
        "ref",
        "heading",
        "wikipedia",
        "wikidata",
        "surface",
        "name",
        "phone",
        "sport",
        "old_name",
        "website",
        "note",
        "access",
        "created_by",
        "place",
        "bench",
        "lit",
        "height",
        "historic",
        "min_height",
        "roof:colour",
        "roof:height",
        "roof:shape",
        "roof:material",
        "roof:levels",
        "count",
        "usage",
        "fence_type",
        "material",
        "covered"
    };

    for (size_t i = 0; i < ITEMS_CNT(skip_list); i++) {
        if (strncmp(skip_list[i], new_item->value.buf, new_item->value.len) == 0) {
            return true;
        }
    }

    return false;
}

static void _store_attr ( int attr_cnt, const hpx_attr_t* new_item ) {

    if ( find_in_skip_list(new_item) ) {
        return;
    }

    int s = g_xml_tags.cnt;
    g_xml_tags.cnt++;

    assert (g_xml_tags.cnt < OSM_MAX_TAGS_CNT );

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(new_item[i].name, "k") == 0) {
            _cp_val(new_item[i], g_xml_tags.list[s].k );
        } else
        if (bs_cmp(new_item[i].name, "v") == 0) {
            _cp_val (new_item[i], g_xml_tags.list[s].v );
        }
    }
}

static void _store_ref ( const bstring_t& value, ref_type_t ref_type ) {

    link_info_t next_item;

    next_item.id  = bs_tol(value);
    next_item.ref = ref_type;

    g_ref_list.push_back ( next_item );
}

static void _store_way ( const osm_draw_type_t draw_type ) {

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

    _store_attr (attr_cnt, attr_list );
}

static void _resolve_node_type ( void ) {

}

static void _process_root_way ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "id") == 0) {
            g_node_info.id = bs_tol(attr_list[i].value );
            break;
        }
    }
}

static void _process_way_nd ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "ref") == 0) {
            _store_ref ( attr_list[i].value, REF_NODE );
            break;
        }
    }

}

static void _process_way_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

    _store_attr ( attr_cnt, attr_list );
}

static void _key_cmp(const char* const key, const char* const val, bool& r1, bool& r2) {

    size_t i = 0;

    for ( i = 0; i < OSM_STR_MAX_LEN-1; i++ ) {
        if ( key[i] == 0 ) {
            break;
        }
        if ( key[i] != val[i] ) {
            return;
        }
    }

    if ( val[i] == 0 ) {
        r1 = true;
    }

    if (val[i] == ':') {
        r2 = true;
    }
}

static void _test_and_add ( const char* const key ) {

    int  i;
    bool key_defined = false;
    bool ns_defined  = false;

    for ( i = 0; i < g_xml_tags.cnt; i++ ) {
        _key_cmp ( key, g_xml_tags.list[i].k, key_defined, ns_defined);
    }

    if ( key_defined ) {
        // Key exists.
        return;
    }

    if ( ! ns_defined ) {
        // namespese doesn't exists
        return;
    }

    int s = g_xml_tags.cnt;
    g_xml_tags.cnt++;

    strncpy ( g_xml_tags.list[s].k, key,    OSM_STR_MAX_LEN-1 );
    strncpy ( g_xml_tags.list[s].v, "auto", OSM_STR_MAX_LEN-1 );
}

static void _preprocess_tag_ways() {

    _test_and_add ( "disused"   );
    _test_and_add ( "bridge"    );
    _test_and_add ( "building"  );
    _test_and_add ( "abandoned" );
    _test_and_add ( "area"      );
    _test_and_add ( "proposed"  );
    _test_and_add ( "construction" );
    _test_and_add ( "cycleway" );
}

static void _resolve_way_type ( cfg::OsmCfgParser& cfg ) {

    osm_draw_type_t draw_type;

    _preprocess_tag_ways();
    
    cfg.ParseWay ( g_xml_tags, g_node_info.id, g_ref_list, draw_type );

    // _store_way(draw_type);
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

static void _osm_pop (cfg::OsmCfgParser& cfg, osm_node_t osm_node ) {

    g_xml_ctx_cnt--;

    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_NODE ) {
        _resolve_node_type();
        g_node_info.len = sizeof(g_node_info);
        g_nodes.Store( g_node_info.id, &g_node_info, sizeof(g_node_info) );
        g_ref_list.clear();
        _clean_ctx(g_xml_tags);
        memset ( &g_node_info, 0, sizeof(g_node_info) );

    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_WAY ) {
        _resolve_way_type( cfg );
        g_ref_list.clear();
        _clean_ctx(g_xml_tags);
        memset ( &g_way_info, 0, sizeof(g_way_info) );

    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_REL ) {
        _resolve_rel_type();
        g_ref_list.clear();
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

static void _process_item (cfg::OsmCfgParser& cfg, int xml_type, const bstring_t& name, int attr_cnt, const hpx_attr_t* attr_list ) {

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
            _osm_pop(cfg, osm_node);
            break;

        case HPX_CLOSE:
            _osm_pop(cfg, osm_node);
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

    g_ref_list.reserve ( LINK_CLUSTER );

    cfg::OsmCfgParser           cfg_parser;

    // cfg_parser.LoadConfig("C:\\GitHub\\map_manager\\dev\\1_Indexer\\map.ost");

    fd = open ( "D:\\OSM_Extract\\prague_short.osm", O_RDONLY);

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

        _process_item(cfg_parser, xml_obj->type, xml_obj->tag, xml_obj->nattr, xml_obj->attr);
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
