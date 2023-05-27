#ifndef __GEOLEX_H__
#define __GEOLEX_H__

#include "geo_types.h"

#define MSG_LEN     (128)

class geo_lex_t {

    public:
        geo_lex_t() {
            msg[0] = 0;
            pos    = 0;
            off    = 0;
        }

    public:

        void reset ( void ) {
            msg[0] = 0;
            pos    = 0;
        }

        void store ( char ch, geo_offset_t offset ) {

            if ( pos == 0 ) {
                off = offset;
            }

            msg[pos] = ch;
            pos++;
            msg[pos] = 0;
        }

    public:
        char            msg[MSG_LEN];
        int             pos;
        geo_offset_t    off;
};

#endif
