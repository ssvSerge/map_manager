#ifndef __OSM_TOOLS_H__
#define __OSM_TOOLS_H__

#include "..\types\osm_types.h"

extern osm_node_t          g_xml_ctx[4];
extern int                 g_xml_ctx_cnt;
extern osm_tag_ctx_t       g_xml_tags;
extern ref_list_t          g_ref_list;
extern osm_node_info_t     g_node_info;

void test_and_add(const char* const key);

#endif

