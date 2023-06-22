#ifndef __GEO_PROCESSOR_H__
#define __GEO_PROCESSOR_H__

#include <stdint.h>
#include <string>
#include <fstream>

#include <geo_types.h>

typedef struct tag_px_ili9341_t {
    uint8_t         r : 5;
    uint8_t         g : 6;
    uint8_t         b : 5;
}   px_ili9341_t;

class geo_pixel_t {
    public:
        uint8_t     r;
        uint8_t     g;
        uint8_t     b;
};

typedef uint16_t geo_pixel_int_t;

class geo_pos_t {
    public:
        geo_pos_t() {
            x = 0;
            y = 0;
        }

    public:
        uint32_t   x;
        uint32_t   y;
};

class geo_processor_t {

    public:
        geo_processor_t();

    public:
        void set_names ( const char* const idx_file_name, const char* const map_file_name );
        void alloc_buffer ( uint32_t width, uint32_t height );
        void set_base_params ( const geo_coord_t center, const double scale, const window_rect_t wnd );
        void set_angle ( const double angle );
        void get_pix ( const geo_pos_t& pos, geo_pixel_t& px ) const;
        void set_pix ( const geo_pos_t& pos, const geo_pixel_t& px );

    public:
        void cache_init ( void );
        void fill_solid ( const geo_pixel_t clr );
        void trim_map   ( void );

    private:
        void _px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const;
        void _px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const;
        void _load_idx ( vector_geo_idx_rec_t& idx_list );
        void _find_base_rect ( const vector_geo_idx_rec_t& map_idx, geo_rect_t& map_rect );
        void _set_scales ( const geo_coord_t center, double scale, const window_rect_t wnd );
        void _filter_rects ( const vector_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, v_uint32_t& out_list );
        bool _is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice );
        void _merge_idx ( const vector_geo_idx_rec_t& rect_list, const v_uint32_t& in_list, v_uint32_t& map_entries );
        void _load_map ( const v_uint32_t& map_entries, l_geo_record_t& map_items );
        void _load_map_entry ( const uint32_t map_entry, geo_record_t& map_record );
        void _trim_map ( void );
        void _trim_record ( const geo_rect_t& window_rect, const geo_record_t& src_record, geo_record_t& dst_record, bool& trim_res );
        void _geo_to_window ( void );
        void _validate_window_rect ( void );
        void _rotate_map ( const double angle );
        void _find_paint_locations ( void );
        bool _pt_in_poly ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const;

    private:
        geo_pixel_int_t*        m_video_buffer;
        uint32_t                m_video_height;
        uint32_t                m_video_width;
        std::string             m_map_file_name;
        std::string             m_idx_file_name;
        std::ifstream           m_map_file;

        geo_coord_t             m_center;
        double                  m_scale;
        geo_rect_t              m_geo_rect;
        double                  m_step_x;
        double                  m_step_y;
        double                  m_angle;

        vector_geo_idx_rec_t    m_map_idx;
        geo_rect_t              m_map_rect;

        l_geo_record_t          m_base_list;
        l_geo_record_t          m_draw_list;

};

#endif
