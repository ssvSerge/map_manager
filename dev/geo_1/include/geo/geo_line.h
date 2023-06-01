#ifndef __GEO_LINE_H__
#define __GEO_LINE_H__

#include <stdint.h>
#include <list>

#include "geo_types.h"

class geo_line_t {

    public:
        uint32_t            off;

    public:
        obj_id_t            role;
        obj_id_t            m_type;
        uint32_t            area;
        geo_path_t          coords;

    public:
        void clear ( void ) {
            role    = OBJID_UNDEF;
            m_type  = OBJID_UNDEF;
            area    = 0;
            coords.clear();
        }
};

typedef std::list<geo_line_t>   list_geo_lines_t;

#endif
