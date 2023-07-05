#ifndef __GEO_PROCESSOR_H__
#define __GEO_PROCESSOR_H__

#include <stdint.h>
#include <string>
#include <fstream>

#include <geo_types.h>

class geo_pixel_t {

    private:
        uint8_t r;
        uint8_t g;
        uint8_t b;

    public:
        geo_pixel_t() {
            r = g = b = 0;
        }

        geo_pixel_t (uint8_t in_r, uint8_t in_g, uint8_t in_b) {
            setR(in_r);
            setG(in_g);
            setB(in_b);
        }

        geo_pixel_t Shift ( int8_t val ) {

            int tmp;

            geo_pixel_t ret_pixel;

            tmp = r + val;
            if ( tmp < 0 )  { tmp =   0; }
            if ( tmp > 255) { tmp = 255; }
            tmp &= ~0x07;
            ret_pixel.r = static_cast<uint8_t>(tmp);

            tmp = g + val;
            if ( tmp < 0 )   { tmp =   0; }
            if ( tmp > 255 ) { tmp = 255; }
            tmp &= ~0x03;
            ret_pixel.g = static_cast<uint8_t>(tmp);

            tmp = g + val;
            if ( tmp < 0 )   { tmp =   0; }
            if ( tmp > 255 ) { tmp = 255; }
            tmp &= ~0x07;
            ret_pixel.b = static_cast<uint8_t>(tmp);

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

        bool operator!= ( const geo_pixel_t& ref ) const {

            bool cmp_res = this->operator== (ref);

            return ! cmp_res;
        }

        void setR ( uint8_t val ) {
            r   =  val;
            r  &= ~0x07;
        }

        void setG ( uint8_t val ) {
            g  =  val;
            g &= ~0x03;
        }

        void setB ( uint8_t val ) {
            b  =  val;
            b &= ~0x07;
        }

        uint8_t getG() const {
            return g;
        }

        uint8_t getR() const {
            return r;
        }

        uint8_t getB() const {
            return b;
        }

};

typedef uint16_t geo_pixel_int_t;

class geo_processor_t {

    public:
        geo_processor_t();

    public:
        void set_names ( const char* const idx_file_name, const char* const map_file_name );
        void alloc_buffer ( uint32_t width, uint32_t height );
        void set_base_params ( const geo_coord_t center, const double scale, const paint_rect_t wnd );
        void set_angle ( const double angle );
        void get_pix ( const paint_coord_t& pos, geo_pixel_t& px ) const;
        void set_pix ( const paint_coord_t& pos, const geo_pixel_t& px );
        void process_wnd ( void );

    public:
        void cache_init ( void );
        void fill_solid ( const geo_pixel_t clr );
        void trim_map   ( void );

    public:
        void _px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const;
        void _px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const;

    private:
        void _load_idx ( v_geo_idx_rec_t& idx_list );
        void _find_base_rect ( const v_geo_idx_rec_t& map_idx, geo_rect_t& map_rect );
        void _set_scales ( const geo_coord_t center, double scale, const paint_rect_t wnd );
        void _filter_rects ( const v_geo_idx_rec_t& rect_list, const geo_rect_t& base_rect, v_uint32_t& out_list );
        bool _is_overlapped ( const geo_rect_t& window, const geo_rect_t& slice ) const;
        void _merge_idx ( const v_geo_idx_rec_t& rect_list, const v_uint32_t& in_list, v_uint32_t& map_entries );
        void _load_map ( const v_uint32_t& map_entries, l_geo_entry_ex_t& map_items );
        void _load_map_entry ( const uint32_t map_entry, geo_entry_ex_t& map_record );
        void _trim_map ( void );
        void _trim_record ( const vv_geo_coord_t& trim_path, const geo_entry_ex_t& in_record, geo_entry_ex_t& out_record );
        void _geo_to_window ( void );
        void _validate_window_rect ( void ) const;
        void _rotate_map ( const double angle );
        void _get_rect ( const v_geo_coord_t& path, geo_rect_t& rect ) const;
        bool _pt_in_poly ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const;
        bool _pt_in_poly ( const v_paint_coord_t& polygon, const paint_coord_t& pt) const;

        void _map_color ( const obj_type_t& obj_type, geo_pixel_t& border_color, geo_pixel_t& fill_color ) const;
        void _line ( const paint_coord_t from, const paint_coord_t to, const geo_pixel_t color );
        void _poly_line ( const v_paint_coord_t& region, const geo_pixel_t color );

        void _fill_poly ( const v_paint_coord_t& region, v_paint_coord_t& coords_list, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr );
        void _fill_poly ( const paint_coord_t& pos, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr );

        bool _pt_in_polygon ( const v_geo_coord_t& polygon, const geo_coord_t& pt ) const;
        bool _pt_in_polygon ( const v_paint_coord_t& polygon, const paint_coord_t& pt ) const;

        void _generate_paint_pos ( const v_paint_coord_t& region, v_paint_coord_t& coords_list ) const;
        bool _is_pt_on_segment ( const paint_coord_t begin, const paint_coord_t end, const paint_coord_t pt ) const;
        double _dist ( const paint_coord_t p1, const paint_coord_t p2 ) const;

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

        l_geo_entry_ex_t        m_map_cache;
        l_geo_entry_ex_t        m_draw_list;
        l_geo_entry_ex_t        m_rotate_list;
        l_paint_entry_ex_t      m_paint_list;

};

#endif
