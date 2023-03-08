#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__

#include "..\common\libhpxml.h"
#include "..\types\osm_types.h"

extern osm_node_t   g_xml_ctx[4];
extern int          g_xml_ctx_cnt;

void process_item(int xml_type, const bstring_t& name, int attr_cnt, const hpx_attr_t* attr_list);

#endif

