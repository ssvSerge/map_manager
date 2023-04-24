#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>

#include "geo_lex.h"
#include "geo_obj.h"

#include "MemMapper.h"

#define CNT(x)   ( sizeof(x) / sizeof(x[0]) )

typedef struct tag_lex_ctx {
    const char*     p;
    const char*     v;
    int             n;
}   lex_ctx_t;

lex_t  g_param;
lex_t  g_value;
bool   g_direction = false;

void _reset ( lex_t& lex ) {
    lex.msg[0] = 0;
    lex.pos = 0;
    return;
}

void _reset ( void ) {

    g_direction = false;

    _reset ( g_param );
    _reset ( g_value );
}

void _store ( lex_t& lex, char ch ) {

    lex.msg[lex.pos] = ch;
    lex.pos++;
    lex.msg[lex.pos] = 0;
}

void _load_lex ( char ch, bool& eo_cmd ) {

    eo_cmd  = false;

    if ( ch == 0x0D ) {
        return;
    }
    if ( ch == 0x0A ) {
        return;
    }

    if ( ch == ':' ) {
        g_direction = true;
        return;
    }
    if ( ch == ';' ) {
        eo_cmd = true;
        return;
    }

    if ( !g_direction ) {
        _store ( g_param, ch );
    } else {
        _store ( g_value, ch );
    }

    return;
}

bool _cmp ( const lex_ctx_t* inp, int& val ) {

    if (strcmp ( inp->p, g_param.msg) != 0) {
        return false;
    }

    if ( inp->v[0] != 0 ) {
        if (strcmp(inp->v, g_value.msg) != 0) {
            return false;
        }
    }

    val = inp->n;
    return true;
}

void _commit_lex () {

    const lex_ctx_t l_lex[] = {
        { "RECORD",     "AREA",        0 },
        { "RECORD",     "END",         0 },
        { "RECORD",     "BUILDING",    0 },
        { "RECORD",     "HIGHWAY",     0 },

        { "ROLE",       "OUTER",       0 },
        { "ROLE",       "INNER",       0 },
        { "ROLE",       "COORDS",      0 },
        { "ROLE",       "END",         0 },

        { "TYPE",       "ASPHALT",     0 },
        { "TYPE",       "WATER",       0 },
        { "TYPE",       "FOREST",      0 },
        { "TYPE",       "GRASS",       0 },
        { "TYPE",       "GENERAL",     0 },
        { "TYPE",       "MOUNTAIN",    0 },
        { "TYPE",       "STONE",       0 },
        { "TYPE",       "SAND",        0 },
        { "TYPE",       "UNDEFINED",   0 },
        { "TYPE",       "BUILDING",    0 },
        { "TYPE",       "FOOTWAY",     0 },
        { "TYPE",       "ROAD",        0 },
        { "TYPE",       "SECONDARY",   0 },
        { "TYPE",       "TRUNK",       0 },
        { "TYPE",       "MOTORWAY",    0 },
        { "TYPE",       "PRIMARY",     0 },
        { "TYPE",       "TERTIARY",    0 },
        { "TYPE",       "RAILWAY",     0 },
        { "TYPE",       "RIVER",       0 },
        { "TYPE",       "BRIDGE",      0 },
        { "TYPE",       "TUNNEL",      0 },

        { "SIZE",       "",            0 },
        { "C",          "",            0 },
    };

    int code = -1;

    for (size_t i = 0; i < CNT(l_lex); i++ ) {

        if ( _cmp( &l_lex[i], code) ) {
            break;
        }
    }

    if ( code == -1 ) {
        code = code;
    }

    _reset();
}

void _commit_line () {

}

int main() {

    file_mapper file;
    uint64_t    file_size;
    bool        eo_cmd;
    char        ch;

    file.Init ( "C:\\GitHub\\map_manager\\dev\\_bin\\log.txt" );
    file_size = file.GetSize();

    for ( size_t i = 0; i < file_size; i++ ) {

        ch = file[i];
        _load_lex ( ch, eo_cmd );

        if ( eo_cmd ) {
            _commit_lex ();
        }

    }

    return 0;
}
