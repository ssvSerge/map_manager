#include <cassert>
#include <string.h>

#include "osm_resolver.h"
#include "osm_tools.h"

#include "..\types\osm_types.h"

void expand_node_tags(void) {

}

void resolve_node_type(void) {

    g_node_info.type = static_cast<osm_obj_type_t> (0);
}

void store_node_info(void) {

    // g_node_info.len = sizeof ( g_node_info );
    // g_nodes.Store ( g_node_info.id, &g_node_info, sizeof(g_node_info) );
}

void clean_node_info ( void ) {

    assert(g_xml_tags.cnt < OSM_MAX_TAGS_CNT);

    while (g_xml_tags.cnt > 0) {
        g_xml_tags.cnt--;
        g_xml_tags.list[g_xml_tags.cnt].k[0] = 0;
        g_xml_tags.list[g_xml_tags.cnt].v[0] = 0;
    }

    memset ( &g_node_info, 0, sizeof(g_node_info) );
    g_ref_list.clear();
}
