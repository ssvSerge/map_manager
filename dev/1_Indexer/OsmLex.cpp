#include <fstream>
#include <sstream>

#include "OsmLex.h"

#include "..\common\osm_idx.h"
#include "..\common\osm_types.h"

namespace cfg {

osm_tokens_t split ( const std::string src ) {

    osm_tokens_t        res;
    std::string         token;
    std::istringstream  iss(src);

    while ( iss ) {
        iss >> token;
        if ( token.length() > 0 ) {
            res.push_back(token);
            token = "";
        }
    }

    return res;
}

bool OsmCfgParser::LoadConfig ( const char* const name ) {

    // std::ifstream   file ( name, std::ios::binary | std::ios::ate );
    // std::streamsize size = file.tellg();
    // file.seekg(0, std::ios::beg);
    // 
    // m_cfg_offset = 0;
    // m_cfg_text.resize ( size + 1 );
    // file.read ( m_cfg_text.data(), size );
    // 
    // for ( ; ; ) {
    // 
    //     if ( ! lex (  ) ) {
    //         break;
    //     }
    // }
    // 
    return true;
}

}
