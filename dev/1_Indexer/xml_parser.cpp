#include <cassert>

#include "..\common\libhpxml.h"
#include "..\common\ssearch.h"

#include "..\types\osm_types.h"

#include "osm_resolver.h"

#define OSM_TYPE_PREV     ( (int)(g_xml_ctx[g_xml_ctx_cnt-2]) )
#define OSM_TYPE_CURR     ( (int)(g_xml_ctx[g_xml_ctx_cnt-1]) )
#define MAP_TYPE(p,c)     ( ((int)(p)<<16) + ((int)(c)) )
#define GEO_SCALE         ( 10000000 )


ref_list_t          g_ref_list;
osm_node_t          g_xml_ctx[4];
int                 g_xml_ctx_cnt =   0;
osm_tag_ctx_t       g_xml_tags    = { 0 };
osm_node_info_t     g_node_info   = { 0 };


static void _clean_ctx ( void ) {

    assert ( g_xml_tags.cnt < OSM_MAX_TAGS_CNT );

    while (g_xml_tags.cnt > 0) {
        g_xml_tags.cnt--;
        g_xml_tags.list[g_xml_tags.cnt].k[0] = 0;
        g_xml_tags.list[g_xml_tags.cnt].v[0] = 0;
    }

    g_ref_list.clear();
}

static void _cp_val ( const hpx_attr_t& src, alloc_str_t& dst ) {

    strncpy_s(dst, src.value.buf, src.value.len);
    dst[src.value.len] = 0;
}

static void _store_attr ( int attr_cnt, const hpx_attr_t* new_item ) {

    int s = g_xml_tags.cnt;
    g_xml_tags.cnt++;

    assert(g_xml_tags.cnt < OSM_MAX_TAGS_CNT);

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(new_item[i].name, "k") == 0) {
            _cp_val(new_item[i], g_xml_tags.list[s].k);
        } else
        if (bs_cmp(new_item[i].name, "v") == 0) {
            _cp_val(new_item[i], g_xml_tags.list[s].v);
        }
    }
}

static void _store_ref ( const bstring_t& value, ref_type_t ref_type ) {

    link_info_t next_item;

    next_item.id = bs_tol(value);
    next_item.ref = ref_type;

    g_ref_list.push_back(next_item);
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

    int  type;
    bool skip;

    skip = g_skiplist_nodes.find ( attr_list->value, type );
    if (skip) {
        return;
    }

    _store_attr ( attr_cnt, attr_list );
}

static void _process_root_way ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "id") == 0) {
            g_node_info.id = bs_tol(attr_list[i].value);
            break;
        }
    }
}

static void _process_way_nd ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "ref") == 0) {
            _store_ref(attr_list[i].value, REF_NODE);
            break;
        }
    }
}

static void _process_way_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

    int  type;
    bool skip;

    skip = g_skiplist_ways.find(attr_list->value, type);
    if (skip) {
        return;
    }

    _store_attr ( attr_cnt, attr_list );
}

static void _process_root_rel(int attr_cnt, const hpx_attr_t* attr_list) {

}

static void _process_rel_member(int attr_cnt, const hpx_attr_t* attr_list) {

}

static void _process_rel_tag(int attr_cnt, const hpx_attr_t* attr_list) {

}

static void _osm_push ( osm_node_t next_node ) {

    g_xml_ctx[g_xml_ctx_cnt] = next_node;
    g_xml_ctx_cnt++;
}

static void _process_open ( int attr_cnt, const hpx_attr_t* attr_list ) {

    switch ( MAP_TYPE(OSM_TYPE_PREV, OSM_TYPE_CURR) ) {


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

static void _osm_pop (osm_node_t osm_node ) {

    g_xml_ctx_cnt--;

    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_NODE ) {
        expand_node_tags();    // 
        resolve_node_type();   // 
        store_node_info();     // 
        clean_node_info();     // 
    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_WAY ) {
        ways_expand_tags();
        ways_resolve_type();
        ways_store_info();
        ways_clean_info();
    } else
    if ( g_xml_ctx[g_xml_ctx_cnt] == XML_NODE_REL ) {
        expand_rel_tags();
        resolve_rel_type();
        store_rel_info();
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

