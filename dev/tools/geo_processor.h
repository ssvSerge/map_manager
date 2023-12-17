#ifndef __GEO_PROCESSOR_H__
#define __GEO_PROCESSOR_H__

#include <stdint.h>
#include <string>
#include <fstream>

#include <geo_types.h>

#include <agg_basics.h>
#include <agg_rendering_buffer.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
#include <agg_trans_affine.h>
#include <agg_conv_stroke.h>
#include <agg_conv_transform.h>
#include <agg_path_storage.h>
#include <agg_rasterizer_scanline_aa.h>

#define AGG_BGR24
#include <platform/agg_platform_support.h>
#include <platform/win32/agg_win32_bmp.h>
#include <formats/pixel_formats.h>


typedef std::vector<int>                x_line_t;

typedef std::vector<x_line_t>           matrix_t;

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

typedef std::vector<geo_pixel_int_t>    video_buff_t;

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

typedef enum tag_segment_dir {
    DIR_UNDEF,
    DIR_UP,
    DIR_DOWN,
}   dir_t;

class segment_raw_t {

    public:
        map_pos_t p1;
        map_pos_t p2;

    public:
        segment_raw_t() {
            clear();
        }

        void clear() {
            p1.x = p2.x = 0;
            p1.y = p2.y = 0;
        }
};

class segment_t {

    public:
        segment_t() {
            valid       = false;
            dir_prev    = DIR_UNDEF;
            dir_my      = DIR_UNDEF;
            dir_next    = DIR_UNDEF;
            srt.clear();
            org.clear();
        }

    public:
        bool          valid;
        segment_raw_t org;
        segment_raw_t srt;
        dir_t         dir_prev;
        dir_t         dir_my;
        dir_t         dir_next;

    public:
        void set_prev_dir ( const dir_t& _dir ) {
            dir_prev = _dir;
        }

        void set_next_dir ( const dir_t& _dir ) {
            dir_next = _dir;
        }

        void set_coord ( const map_pos_t& _p1, const map_pos_t& _p2 ) {

            valid  = true;

            org.p1 = _p1;
            org.p2 = _p2;

            if (_p1.y <= _p2.y) {
                srt.p1 = _p1;
                srt.p2 = _p2;
                dir_my = DIR_DOWN;
            } else {
                srt.p1 = _p2;
                srt.p2 = _p1;
                dir_my = DIR_UP;
            }

            return;
        }
};

typedef std::vector<segment_t>  v_segment_t;

typedef enum tag_intersection_type {
    INTERSECTION_TYPE_UNKNOWN,
    INTERSECTION_TYPE_DOT,
    INTERSECTION_TYPE_UP,
    INTERSECTION_TYPE_DOWN
}   intersection_type_t;

class intersection_info {
    public:
        intersection_info() {
            cnt_pt   = 0;
            cnt_up   = 0;
            cnt_down = 0;
        }

        bool is_active() {
            if (cnt_pt > 0) {
                return true;
            }
            if (cnt_up) {
                return true;
            }
            if (cnt_down) {
                return true;
            }
            return false;
        }
    
    public:
        uint16_t   cnt_pt;
        uint16_t   cnt_up;
        uint16_t   cnt_down;
};

typedef std::vector<intersection_info> v_intersection_info;

struct agg_path_attributes {

    unsigned        idx;
    agg::srgba8     fill_color;
    agg::srgba8     stroke_color;
    double          stroke_width;

    agg_path_attributes() {

        idx = 0;

        stroke_width = 0;
        fill_color.r = 0;
        fill_color.g = 0;
        fill_color.b = 0;
        fill_color.a = 0;

        stroke_color.r = 0;
        stroke_color.g = 0;
        stroke_color.b = 0;
        stroke_color.a = 0;
    }

    agg_path_attributes(unsigned _idx, const agg::srgba8& fill, const agg::srgba8& stroke, double width) {
        idx = _idx;
        fill_color = fill;
        stroke_color = stroke;
        stroke_width = width;
    }
};

typedef std::vector<agg_path_attributes> v_agg_path_attributes;

struct agg_trans_roundoff {
    static void transform(double* x, double* y) {
        *x = floor(*x + 0.5);
        *y = floor(*y + 0.5);
    }
};


class geo_processor_t {

    public:
        geo_processor_t();

    public:
        void close ( void );
        void set_names ( const char* const idx_file_name, const char* const map_file_name );
        void load_idx(void);
        void get_pix ( const map_pos_t& pos, geo_pixel_t& px ) const;
        void process_map ( const geo_rect_t& wnd, const geo_coord_t& center, const double scale, const double angle );
//      void set_pix ( const map_pos_t& pos, const geo_pixel_t& px );
        void geo_intersect ( const pos_type_t coord_type, bool is_area, const geo_line_t& path, const geo_rect_t& in_rect, v_geo_line_t& clipped_path ) const;
        void set_screen_params ( uint32_t width, uint32_t height );
        void paint();

    private:
        void _load_idx ( l_geo_idx_rec_t& idx_list );
        void _process_rects ( const geo_coord_t& center, const double scale, const geo_rect_t& paint_wnd, geo_rect_t& map_rect, geo_rect_t& map_rect_ext ) const;
        void _extend_rect ( const geo_coord_t& center, const geo_rect_t& src_rect, geo_rect_t& dst_rect ) const;
        bool _is_scale_valid ( const double scale ) const;
        bool _is_view_rect_valid ( const geo_rect_t& view_rect ) const;
        bool _is_angle_valid ( const double angle ) const;
        void _alloc_img_buffer(const geo_rect_t& geo_rect);
        void _filter_idx_by_rect ( const geo_rect_t& base_rect, const l_geo_idx_rec_t& rect_list, v_geo_offset_t& out_list ) const;
        void _load_map_by_idx ( const v_uint32_t& map_entries, l_geo_entry_t& map_cache );
        void _load_map_entry ( const uint32_t map_entry, geo_entry_t& map_record );
        bool _is_overlapped ( const geo_rect_t& window, const pos_type_t pos_type, const geo_rect_t& slice ) const;
        void _draw_area ( void );
        void _draw_building ( void );
        void _draw_roads ( void );
        void _map_color ( const obj_type_t& obj_type, agg::srgba8& border_color, agg::srgba8& fill_color ) const;

//      void _find_idx_rect ( const l_geo_idx_rec_t& map_idx, geo_rect_t& map_rect, double& x_step, double& y_step ) const;
//      void _px_conv ( const geo_pixel_t& from, geo_pixel_int_t& to ) const;
//      void _px_conv ( const geo_pixel_int_t& from, geo_pixel_t& to ) const;
//      void _fill_solid ( const geo_pixel_t clr );
//      void _set_angle ( const double angle );
//      void _rotate_map_by_angle ( void );
//      void _trim_rotated_map_by_rect ( void );
//      void _rotate_geo_line ( v_geo_coord_t& coords ) const;
//      void _rotate_coord ( geo_coord_t& coord ) const;
//      void _trim_record ( const geo_rect_t& rect_path, const geo_entry_t& geo_path, const bool is_area, geo_entry_t& out_record ) const;
//      void _process_area ( geo_entry_t& geo_line );
//      void _poly_area ( const geo_line_t& region, const geo_pixel_t color );
//      void _poly_line ( const v_geo_coord_t& line, const int width, const geo_pixel_t color );
//      void _fill_poly ( geo_line_t& region, const geo_pixel_t border_clr, const geo_pixel_t fill_clr );
//      void _fill_poly ( const geo_coord_t& pos, const geo_pixel_t br_clr, const geo_pixel_t fill_clr, const bool ignore_bk );
//      void _line ( const geo_coord_t& from, const geo_coord_t& to, const geo_pixel_t color );
//      void _line ( const geo_coord_t& from, const geo_coord_t& to, int width, const geo_pixel_t color );
//      void _find_scale_pixel ( const geo_coord_t& center, const double scale );
//      void _calc_geo_rect ( const geo_coord_t& center, const geo_rect_t& wnd );
//      bool _pt_in_rect ( const map_pos_t pt, const geo_rect_t& wnd ) const;
//      void _get_view_rect ( const geo_rect_t& wnd, const geo_coord_t& center, const double scale, geo_rect_t& view_wnd ) const;
//      void _extend_view_rect ( const geo_coord_t& center, geo_rect_t& view_rect ) const;
//      bool _is_pt_on_segment ( const geo_coord_t& begin, const geo_coord_t& end, const geo_coord_t& pt ) const;
//      bool _pt_in_poly ( const v_geo_coord_t& polygon, const geo_coord_t& point ) const;
//      void _logged_line ( int x1, int y1, const int x2, const int y2, const geo_pixel_t& clr );
//      void _log_pos ( const map_pos_t& dot_pos, bool is_marked );
//      void _process_pt_list ( const geo_coord_t& base, const v_paint_offset_t& shift_list, const geo_pixel_t color );
//      void _pt_geo_to_map ( const map_pos_t& src, map_pos_t& dst ) const;
//      bool _are_collinear ( const geo_coord_t& p1, const geo_coord_t& p2, const geo_coord_t& p3 ) const;
//      void _clear_intersection ( void );
//      void _intersection ( const segment_t& s1, const segment_t& s2, map_pos_t& result );
//      void _commit_intersection ( const map_pos_t& base, size_t y, const geo_pixel_t& clr );
//      void _add_intersection ( const map_pos_t& base, const map_pos_t& pos, intersection_type_t type );
//      void _generate_paint_pos(geo_line_t& poly_line) const;
//      bool _find_pt_in_poly ( const map_pos_t& p1, const map_pos_t& p2, const map_pos_t& p3, map_pos_t& inside ) const;
//      double _calc_dist ( const map_pos_t& p1, const map_pos_t& p2, const map_pos_t& p3 ) const;
//      void _map_idx ( const geo_rect_t rect,  const double& x_step, const double& y_step, l_geo_idx_rec_t& map_idx );
//      void _calc_map_rect ( const geo_coord_t center, const double scale, const paint_rect_t wnd );
//      void _merge_idx ( const v_geo_idx_rec_t& rect_list, const v_uint32_t& in_list, v_uint32_t& map_entries );
//      void _reset_map ( void );
//      void _map_coords ( const v_geo_coord_t& geo_path, v_paint_coord_t& win_path );
//      void _win_coord ( const geo_coord_t& geo_pos, paint_coord_t& win_pos ) const;
//      bool _is_wnd_invalid ( const geo_rect_t& wnd, const geo_coord_t& center, const double scale ) const;
//      void _geo_rect_to_view ( const geo_coord_t& center, const geo_rect_t& screen_wnd, geo_rect_t& paint_wnd );
//      void _map_pt_pos ( const geo_coord_t& point, const geo_rect_t& square, const pos_type_t src, pt_code_pos_t& code ) const;
//      bool _get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const geo_coord_t& p3, const geo_coord_t& p4, const pos_type_t src, geo_coord_t& point) const;
//      bool _get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const pt_code_pos_t border, const geo_rect_t& rect, const pos_type_t src, geo_coord_t& point ) const;
//      void _clip_poly_line ( const v_geo_coord_t& line, const geo_rect_t& rect, vv_geo_coord_t& clippedLine ) const;
//      void _close_segment ( const bool is_area, const pos_type_t coord_type, v_geo_coord_t& segment ) const;

    public:
        agg::pixel_map          m_pmap_window;
        agg::rendering_buffer   m_rbuf_window;
        agg::rasterizer_scanline_aa<> m_rasterizer;
        uint32_t                m_bpp;
        agg::scanline_p8        m_scanline;
        agg::path_storage       m_agg_paths;
        v_agg_path_attributes   m_agg_paths_attribs_ex;
        double                  m_dx;
        double                  m_dy;

    private:
        std::string             m_map_file_name;
        std::ifstream           m_map_file;
        std::string             m_idx_file_name;
        std::ifstream           m_idx_file;

        double                  m_x_step;               // ��� �������� �� �����������.
        double                  m_y_step;               // ��� �������� �� ���������.
        double                  m_view_angle;           // ������� ���� �����.
        double                  m_view_angle_step;      // ������� ������� ��� ����������� �����.
        double                  m_scale_step;           // ������� ���������� ��� ����������� �����.
        double                  m_scale;                // ������� ���������� �����.
        geo_coord_t             m_center;               // 
        geo_rect_t              m_view_load;            // ������ ��� ��������.
        geo_rect_t              m_view_out;             // ���� ������ � ������������ "0, 0".
        geo_rect_t              m_view_geo;             // ����, ���������� � GPS �����������.
        v_geo_offset_t          m_map_ids;
        l_geo_entry_t           m_map_cache;
        l_geo_idx_rec_t         m_idx_list;



//      double                  m_geo_step_x;           // GPS ��� �� ����������� �� ������.
//      double                  m_geo_step_y;           // GPS ��� �� ��������� �� ������.
//      double                  m_prj_step_x;           // Mercator ��� �� ����������� �� ������.
//      double                  m_prj_step_y;           // Mercator ��� �� ��������� �� ������.
// 
// 
//      map_pos_t               m_min1;
//      map_pos_t               m_max1;
//      matrix_t                m_matrix;
//      double                  m_geo_rect_scale;

//      geo_coord_t             m_geo_center;
//      double                  m_geo_scale;
//      double                  m_geo_angle;
//      double                  m_geo_angle_sin;
//      double                  m_geo_angle_cos;
//      l_geo_entry_t           m_paint_list;
//      video_buff_t            m_video_buffer;
//      v_intersection_info     m_intersection_map;
//      geo_coord_t             m_paint_center;
//      vv_geo_idx_rec_t        m_idx_map;
//      geo_rect_t              m_idx_rect;
//      int                     m_idx_x_cnt;
//      int                     m_idx_x_step;
//      int                     m_idx_y_cnt;
//      int                     m_idx_y_step;
//      l_geo_idx_rec_t         m_map_idx2;
//      double                  m_map_step_x;
//      double                  m_map_step_y;
//      geo_rect_t              m_map_geo_rect;
//      geo_rect_t              m_paint_rect;
//      double                  m_paint_scale;
//      l_geo_entry_t           m_map_cache;
};

#endif
