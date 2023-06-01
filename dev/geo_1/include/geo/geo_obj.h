#ifndef __GEO_OBJ_H__
#define __GEO_OBJ_H__

#include <vector>
#include <list>

#include "geo_param.h"
#include "geo_line.h"

#include <clipper2/clipper.h>

typedef struct tag_geo_coord {
    double                  lon;
    double                  lat;
}   geo_coord_t;

typedef std::vector<geo_coord_t> vector_geo_coords_t;

typedef struct tag_geo_object {
    geo_role_t              role;
    geo_entry_t             type;
    size_t                  area;
    vector_geo_coords_t     coords;
}   geo_object_t;

typedef struct tag_lex_ctx {
    const char*             p;
    const char*             v;
    obj_id_t                n;
}   lex_ctx_t;

class geo_rect_t {

    public:
        void clear() {
            min_lon = -333;
            min_lat = -333;
            max_lon = -333;
            max_lat = -333;
        }

        void load(const char* const val) {
            (void)sscanf_s( val, "%lf %lf %lf %lf", &min_lon, &min_lat, &max_lon, &max_lat);
        }

    public:
        double   min_lon;
        double   min_lat;
        double   max_lon;
        double   max_lat;
};

class geo_obj_map_t {

    public:
        geo_offset_t        m_off;
        obj_id_t            m_type;
        list_geo_lines_t    m_lines;
        obj_id_t            m_record;

    public:
        void clear() {
            m_record  = OBJID_UNDEF;
            m_type    = OBJID_UNDEF;
            m_off     = 0;
            m_lines.clear();
        }
};

typedef std::list<geo_obj_map_t> list_geo_objs_t;

class geo_idx_rec_t {

    public:
        geo_rect_t          m_rect;
        vector_geo_off_t    m_list_off;

    public:
        void clear() {
            m_rect.clear();
            m_list_off.clear();
        }
};

typedef std::vector<geo_idx_rec_t> vector_geo_idx_rec_t;

typedef std::vector<obj_id_t>  vector_types;

class geo_draw_t {
    public:
        obj_id_t              m_type;
        vector_types          m_types;
        Clipper2Lib::PathsD   m_paths;
};

typedef std::vector<geo_draw_t>  vector_geo_draw_t;

#endif
