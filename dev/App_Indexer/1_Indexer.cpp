#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>

#include "..\common\ssearch.h"
#include "..\common\libhpxml.h"

#include "xml_parser.h"
#include "osm_resolver.h"


ssearcher     g_skiplist_nodes;
ssearcher     g_skiplist_ways;
ssearcher     g_skiplist_rels;


static void _load_skiplist ( const char* const file_name, ssearcher& bor ) {

    std::ifstream cfg_file;
    std::string   line;
    std::string   key;

    cfg_file.open( file_name );

    while ( std::getline(cfg_file, line) ) {
        if ( ! line.empty() ) {
            bor.add(line, 0);
        }
    }
}

int main() {

    int         fd      = 0;
    hpx_ctrl_t* ctrl    = nullptr;
    hpx_tag_t*  xml_obj = nullptr;
    bstring_t   xml_str;
    long        lno;
    int         io_res;

    _load_skiplist ( "skip.nodes", g_skiplist_nodes );
    _load_skiplist ( "skip.ways",  g_skiplist_ways  );
    _load_skiplist ( "skip.rels",  g_skiplist_rels  );

    _sopen_s ( &fd, "D:\\OSM_Extract\\prague_short.osm", _O_BINARY | _O_RDONLY, _SH_DENYWR, _S_IREAD );
    if ( fd == -1 ) {
        return -1;
    }
    
    ctrl = hpx_init ( fd, 100 * 1024 * 1024 );
    if (ctrl == NULL) {
        return -1;
    }

    xml_obj = hpx_tm_create ( 64 );
    if (xml_obj == NULL) {
        return -2;
    }

    ways_init();

    g_xml_ctx[0] = XML_NODE_ROOT;
    g_xml_ctx_cnt++;

    for ( ; ; ) {

        io_res = hpx_get_elem ( ctrl, &xml_str, NULL, &lno );
        if (io_res == 0) {
            break;
        }

        io_res = hpx_process_elem ( xml_str, xml_obj );
        if (io_res != 0) {
            printf ( "[%ld] ERROR in element: %.*s\n", lno, xml_str.len, xml_str.buf );
            break;
        }

        process_item ( xml_obj->type, xml_obj->tag, xml_obj->nattr, xml_obj->attr );
    }

    if ( ! ctrl->eof ) {
        assert(false);
        return -3;
    }

    hpx_tm_free ( xml_obj );
    hpx_free    ( ctrl );

    _close(fd);

    return 0;
}
