#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>

#include "..\common\osm_processor.h"

// ssearcher     g_skiplist_nodes;
// ssearcher     g_skiplist_ways;
// ssearcher     g_skiplist_rels;

int main() {

    // int         fd      = 0;
    // hpx_ctrl_t* ctrl    = nullptr;
    // hpx_tag_t*  xml_obj = nullptr;
    // bstring_t   xml_str;
    // long        lno;
    // int         io_res;

    osm_processor_t processor;

    processor.process_file("D:\\OSM_Extract\\prague_short.osm");


    return 0;
}
