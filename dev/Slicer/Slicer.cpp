#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>

#include "MemMapper.h"

#define    MSG_LEN     (128)

typedef struct tag_lex {
    char   msg[MSG_LEN];
    int    pos;
}   lex_t;

typedef struct tag_coord {
    double lon;
    double lat;
}   coord_t;

typedef enum tag_lex_param {
    PARAM_RECORD,
    PARAM_TYPE,
    PARAM_ROLE,
    PARAM_SIZE,
    PARAM_POINTS,
    PARAM_COORD,
    PARAM_LAST_ID
}   lex_param_t;

typedef enum tag_role {
    ROLE_OUTER,
    ROLE_INNER
}   role_t;

typedef enum tag_draw_type {
    DRAW_TYPE_ASPHALT,
}   draw_type_t;

typedef struct tag_param_map {
    const char* const    text;
    lex_param_t          id;
}   param_map;

typedef std::vector<coord_t> vector_coords_t;

lex_t  g_param;
lex_t  g_value;
bool   g_direction = false;

void _process_lex_record() {

}

void _process_lex_type() {

}

void _process_lex_role() {

}

void _process_lex_size() {

}

void _process_lex_points() {

}

void _commit () {

    static const param_map map[] = {
        { "RECORD",    PARAM_RECORD  },
        { "TYPE",      PARAM_TYPE    },
        { "ROLE",      PARAM_ROLE    },
        { "SIZE",      PARAM_SIZE    },
        { "POINTS",    PARAM_POINTS  },
        { "C",         PARAM_COORD  }
    };

    lex_param_t param = PARAM_LAST_ID;

    int cnt = sizeof(map) / sizeof(map[0]);

    for (int i = 0; i < cnt; i++) {
        if (strcmp(map[i].text, g_param.msg) == 0) {
            param = map[i].id;
            break;
        }
    }

    if ( param == PARAM_LAST_ID ) {
        assert(false);
    }

    switch (param) {
        case PARAM_RECORD:
            _process_lex_record();
            break;
        case PARAM_TYPE:
            _process_lex_type();
            break;
        case PARAM_ROLE:
            _process_lex_role();
            break;
        case PARAM_SIZE:
            _process_lex_size();
            break;
        case PARAM_POINTS:
            _process_lex_size();
            break;
    }

    return;
}

void _reset ( lex_t& lex ) {
    lex.msg[0] = 0;
    lex.pos    = 0;
    return;
}

void _reset ( void ) {

    g_direction = false;

    _reset(g_param);
    _reset(g_value);
}

void _store ( lex_t& lex, char ch ) {

    lex.msg [ lex.pos ] = ch;
    lex.pos++;
    lex.msg [ lex.pos ] = 0;
}

void _store ( char ch ) {

    if ( ch == ':' ) {
        g_direction = true;
        return;
    }
    if ( ch == ';' ) {
        _commit();
        _reset();
        return;
    }

    if ( !g_direction ) {
        _store ( g_param, ch );
    } else {
        _store ( g_value, ch );
    }
}

int main() {

    file_mapper file;
    uint64_t    file_size;

    file.Init ( "C:\\GitHub\\map_manager\\dev\\_bin\\x64_Debug\\log.txt" );
    file_size = file.GetSize();

    for ( size_t i = 0; i < file_size; i++ ) {

        if ( (file[i] == 0x0d) || (file[i] == 0x0a) ) {
            _reset(g_param);
            _reset(g_value);
            continue;
        }

        _store ( file[i] );
    }

    return 0;
}
