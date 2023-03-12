#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>

#include "..\common\osm_processor.h"

osm_processor_t processor;

void add_node (const osm_obj_info_t& info) {

}

void add_way (const osm_obj_info_t& info) {

}

void add_rel(const osm_obj_info_t& info) {

}

int main() {

    processor.configure ( add_node, add_way, add_rel );
    processor.process_file("D:\\OSM_Extract\\prague_short.osm");

    return 0;
}
