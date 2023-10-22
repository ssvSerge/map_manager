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

        void clear() {
            r = g = b = 0;
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

typedef enum tag_pt_code_pos {
    CODE_POS_IN,
    CODE_POS_LEFT,
    CODE_POS_RIGHT,
    CODE_POS_TOP,
    CODE_POS_BOTTOM
}   pt_code_pos_t;

class paint_offset_t {
    public:
        paint_offset_t() {
            dx = 0;
            dy = 0;
        }

    public:
        int         dx;
        int         dy;
};

typedef std::vector<paint_offset_t>    v_paint_offset_t;

class geo_processor_t {

    public:
        geo_processor_t();

    public:
        void close ( void );
        void set_names ( const char* const idx_file_name, const char* const map_file_name );
        void get_pix ( const paint_coord_t& pos, geo_pixel_t& px ) const;
        void set_pix ( const paint_coord_t& pos, const geo_pixel_t& px );
        void load_idx ( void );
        void process_map ( const paint_rect_t wnd, const geo_coord_t center, const double scale, const double angle );
        void geo_intersect ( const v_geo_coord_t& polyline, const geo_rect_t& in_rect, const pos_type_t coord_type, bool is_area, vv_geo_coord_t& clippedLine ) const;

    private:
        void _load_idx ( l_geo_idx_rec_t& idx_list );
        void _find_idx_rect ( const l_geo_idx_rec_t& map_idx, geo_rect_t& map_rect, int& x_step, int& y_step );
        void _px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const;
        void _alloc_img_buffer ( const geo_rect_t& geo_rect );
        void _fill_solid ( const geo_pixel_t clr );
        void _load_ids ( const geo_rect_t& base_rect, const l_geo_idx_rec_t& rect_list, v_geo_offset_t& out_list );
        void _load_map_by_idx ( const v_uint32_t& map_entries, l_geo_entry_t& map_cache );
        void _set_angle ( const double angle );
        void _apply_angle ( void );
        void _trim_map_by_rect ( void );
        void _draw_area ( void );
        void _draw_building ( void );
        void _draw_roads ( void );
        bool _is_overlapped ( const geo_rect_t& window, const pos_type_t pos_type, const geo_rect_t& slice ) const;
        void _load_map_entry ( const uint32_t map_entry, geo_entry_t& map_record );
        void _rotate_geo_line ( v_geo_coord_t& coords );
        void _rotate_coord ( geo_coord_t& coord ) const;
        void _trim_record ( const geo_rect_t& rect_path, const geo_entry_t& geo_path, const bool is_area, paint_entry_t& out_record );
        void _process_area ( paint_line_t& geo_line, const bool force_clr, const bool mark_up );
        void _map_color ( const obj_type_t& obj_type, geo_pixel_t& border_color, geo_pixel_t& fill_color ) const;
        void _poly_area ( const v_paint_coord_t& region, const geo_pixel_t color );
        void _poly_line ( const v_paint_coord_t& line, const int width, const geo_pixel_t color );
        void _fill_poly ( const v_paint_coord_t& region, v_paint_coord_t& coords_list, const geo_pixel_t bk_clr, const geo_pixel_t fill_clr, const bool force_clr, const bool mark_up );
        void _line ( const paint_coord_t from, const paint_coord_t to, const geo_pixel_t color );
        void _line ( const paint_coord_t from, const paint_coord_t to, int width, const geo_pixel_t color );
        void _process_pt_list ( const paint_coord_t base, const v_paint_offset_t& shift_list, const geo_pixel_t color );
        // void _map_idx ( const geo_rect_t rect,  const double& x_step, const double& y_step, l_geo_idx_rec_t& map_idx );
        // void _px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const;
        // void _calc_map_rect ( const geo_coord_t center, const double scale, const paint_rect_t wnd );
        // void _merge_idx ( const v_geo_idx_rec_t& rect_list, const v_uint32_t& in_list, v_uint32_t& map_entries );
        // void _reset_map ( void );
        // void _map_coords ( const v_geo_coord_t& geo_path, v_paint_coord_t& win_path );
        // void _win_coord ( const geo_coord_t& geo_pos, paint_coord_t& win_pos ) const;
        // void _fill_poly ( const paint_coord_t& pos, const geo_pixel_t br_clr, const geo_pixel_t fill_clr, const bool ignore_bk );
        // void _generate_paint_pos ( const v_paint_coord_t& region, v_paint_coord_t& coords_list ) const;
        // bool _is_pt_on_segment ( const paint_coord_t begin, const paint_coord_t end, const paint_coord_t pt ) const;
        // bool _pt_in_poly ( const v_paint_coord_t& polygon, const paint_coord_t& pt) const;
        // void _map_pt_pos ( const geo_coord_t& point, const geo_rect_t& square, const pos_type_t src, pt_code_pos_t& code ) const;
        // bool _get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const geo_coord_t& p3, const geo_coord_t& p4, const pos_type_t src, geo_coord_t& point) const;
        // bool _get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const pt_code_pos_t border, const geo_rect_t& rect, const pos_type_t src, geo_coord_t& point ) const;
        // void _clip_poly_line ( const v_geo_coord_t& line, const geo_rect_t& rect, vv_geo_coord_t& clippedLine ) const;
        // void _close_segment ( const bool is_area, const pos_type_t coord_type, v_geo_coord_t& segment ) const;

    private:
        std::string             m_map_file_name;
        std::string             m_idx_file_name;
        std::ifstream           m_map_file;

        geo_coord_t             m_geo_center;
        geo_rect_t              m_geo_wnd;
        double                  m_geo_scale;
        double                  m_geo_angle;
        double                  m_geo_angle_sin;
        double                  m_geo_angle_cos;

        v_geo_offset_t          m_map_ids;
        l_geo_entry_t           m_map_cache;

        l_paint_entry_t         m_paint_list;

        // geo_coord_t             m_paint_center;
        // vv_geo_idx_rec_t        m_idx_map;

        l_geo_idx_rec_t         m_idx_list;

        geo_pixel_int_t*        m_video_buffer;
        uint32_t                m_video_buffer_size;
        paint_rect_t            m_video_rect;


        // 
        // geo_rect_t              m_idx_rect;
        // int                     m_idx_x_cnt;
        // int                     m_idx_x_step;
        // int                     m_idx_y_cnt;
        // int                     m_idx_y_step;
        // 
        // 
        // l_geo_idx_rec_t         m_map_idx2;
        // double                  m_map_step_x;
        // double                  m_map_step_y;
        // geo_rect_t              m_map_geo_rect;
        // 
        // geo_rect_t              m_paint_rect;
        // 
        // double                  m_paint_scale;
        // 
        // 
        // 
        // l_geo_entry_t           m_map_cache;
};

#endif
