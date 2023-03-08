#include <cassert>

#include "osm_tools.h"
#include "..\types\osm_types.h"

static void _key_cmp ( const char* const key, const char* const val, bool& r1, bool& r2 ) {

    size_t i = 0;

    for (i = 0; i < OSM_STR_MAX_LEN - 1; i++) {
        if (key[i] == 0) {
            break;
        }
        if (key[i] != val[i]) {
            return;
        }
    }

    if (val[i] == 0) {
        r1 = true;
    }

    if (val[i] == ':') {
        r2 = true;
    }
}

void test_and_add ( const char* const key ) {

    int  i;
    bool key_defined = false;
    bool ns_defined = false;

    for (i = 0; i < g_xml_tags.cnt; i++) {
        _key_cmp(key, g_xml_tags.list[i].k, key_defined, ns_defined);
    }

    if ( key_defined ) {
        // Key exists.
        return;
    }

    if ( !ns_defined ) {
        // namespese doesn't exists
        return;
    }

    int s = g_xml_tags.cnt;
    g_xml_tags.cnt++;

    assert(g_xml_tags.cnt < OSM_MAX_TAGS_CNT);

    strncpy_s(g_xml_tags.list[s].k, key, OSM_STR_MAX_LEN - 1);
    strncpy_s(g_xml_tags.list[s].v, "auto", OSM_STR_MAX_LEN - 1);
}

