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

    public:
        geo_pixel_t Shift ( int8_t val ) {

            int tmp;

            geo_pixel_t ret_pixel;

            tmp = r + val;
            if ( tmp < 0 )  { tmp =   0; }
            if ( tmp > 255) { tmp = 255; }
            ret_pixel.r = static_cast<uint8_t> (tmp);

            tmp = g + val;
            if ( tmp < 0 )   { tmp =   0; }
            if ( tmp > 255 ) { tmp = 255; }
            ret_pixel.g = static_cast<uint8_t> (tmp);

            tmp = g + val;
            if ( tmp < 0 )   { tmp =   0; }
            if ( tmp > 255 ) { tmp = 255; }
            ret_pixel.b = static_cast<uint8_t> (tmp);

            return ret_pixel;
        }

        bool operator== ( const geo_pixel_t& ref ) const {
            
            if ( this->r != ref.r ) {
                return false;
            }
            if ( this->g != ref.g ) {
                return false;
            }
            if ( this->b != ref.b ) {
                return false;
            }

            return true;
        }

        bool operator!= (const geo_pixel_t& ref) const {

            bool cmp_res = this->operator== (ref);

            return ! cmp_res;
        }
};

typedef uint16_t geo_pixel_int_t;

class geo_pos1_t {
    public:
        geo_pos1_t ( int32_t in_x, int32_t in_y ) {
            x = in_x;
            y = in_y;
        }

        geo_pos1_t() {
            x = 0;
            y = 0;
        }

    public:
        int32_t   x;
        int32_t   y;
};

class geo_processor_t {

    public:
        geo_processor_t();

    public:
        void set_names ( const char* const idx_file_name, const char* const map_file_name );
        void alloc_buffer ( uint32_t width, uint32_t height );
        void set_base_params ( const geo_coord_t center, const double scale, const paint_rect_t wnd );
        void set_angle ( const double angle );
        void get_pix ( const paint_pos_t& pos, geo_pixel_t& px ) const;
        void set_pix ( const paint_pos_t& pos, const geo_pixel_t& px );
        void process_wnd ( void );

    public:
        void cache_init ( void );
        void fill_solid ( const geo_pixel_t clr );
        void trim_map   ( void );

    private:
        void _px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const;
        void _px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const;
        void _load_idx ( v_geo_idx_rec_t& idx_list );
        void _find_base_rect ( const v_geo_idx_rec_t& map_idx, geo_rect_t& map_rect );
        void _set_scales ( const geo_coord_t center, double scale, const paint_rect_t wnd );
        void _filter_rects ( const v_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, v_uint32_t& out_list );
        bool _is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice ) const;
        void _merge_idx ( const v_geo_idx_rec_t& rect_list, const v_uint32_t& in_list, v_uint32_t& map_entries );
        void _load_map ( const v_uint32_t& map_entries, l_geo_record_t& map_items );
        void _load_map_entry ( const uint32_t map_entry, geo_record_t& map_record );
        void _trim_map ( void );
        void _trim_record ( const geo_rect_t& window_rect, const geo_record_t& src_record, geo_record_t& dst_record, bool& trim_res );
        void _geo_to_window ( void );
        void _validate_window_rect ( void ) const;
        void _rotate_map ( const double angle );
        void _find_paint_locations ( void );
        void _get_rect ( const v_geo_coord_t& path, geo_rect_t& rect ) const;
        bool _pt_in_poly ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const;
        bool _pt_in_poly ( const v_paint_coord_t& polygon, const paint_pos_t& pt) const;
        void _generate_paint_points ( const v_geo_coord_t& polygon, v_geo_coord_t& coords_list ) const;
        bool _check_paint_points ( const v_geo_coord_t& polygon, const v_geo_coord_t& coords_list, geo_coord_t& pt ) const;
        bool _test_position ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const;
        void _map_color ( const obj_type_t& obj_type, geo_pixel_t& border_color, geo_pixel_t& fill_color ) const;
        void _line ( const paint_pos_t from, const paint_pos_t to, const geo_pixel_t color );
        void _poly_line ( const v_paint_coord_t& region, const geo_pixel_t color );
        void _fill_poly ( const v_paint_coord_t& region, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr );
        void _paint_region ( const paint_pos_t pos, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr );

    private:
        std::string             m_map_file_name;
        std::string             m_idx_file_name;
        std::ifstream           m_map_file;

        geo_pixel_int_t*        m_video_buffer;
        int32_t                 m_video_height;
        int32_t                 m_video_width;

        geo_coord_t             m_center;
        double                  m_scale;
        geo_rect_t              m_geo_rect;

        double                  m_step_x;
        double                  m_step_y;
        double                  m_angle;

        v_geo_idx_rec_t         m_map_idx;
        geo_rect_t              m_map_rect;

        l_geo_record_t          m_map_cache;
        l_geo_record_t          m_draw_list;

};

#endif
