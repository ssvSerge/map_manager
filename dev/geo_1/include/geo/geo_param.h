#ifndef __GEOPARAM_H__
#define __GEOPARAM_H__

#include "geo_param.h"
#include "geo_lex.h"

class geo_param_t {

    public:
        geo_param_t() {
            direction = false;
        }

    public:

        void reset ( void ) {
            direction = false;
            param.reset();
            value.reset();
        }

        void store ( char ch, geo_offset_t offset ) {
            if ( !direction ) {
                param.store(ch, offset);
            }
            else {
                value.store(ch, offset);
            }
        }

        void _load_lex ( char ch, bool& eo_cmd, geo_offset_t offset ) {

            eo_cmd = false;

            if (ch == 0x0D) {
                return;
            }
            if (ch == 0x0A) {
                return;
            }

            if (ch == ':') {
                direction = true;
                return;
            }
            if (ch == ';') {
                eo_cmd = true;
                return;
            }

            store(ch, offset);
        }

    public:
        geo_lex_t    param;
        geo_lex_t    value;
        bool         direction;
};

#endif
