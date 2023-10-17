#include <queue>
#include <vector>

#include <geo_processor.h>
#include <geo_projection.h>

#include <gpc.h>

class gpc_allocator_t {

    public:

        gpc_allocator_t () {
            data.num_contours = 0;
            data.hole = nullptr;
            data.contour = nullptr;
        }

        void add ( size_t pos, double x, double y ) {
            data.contour[0].vertex[pos].x = x;
            data.contour[0].vertex[pos].y = y;
        }

        void alloc ( size_t cnt ) {

            if ( m_hole.size() < 1 ) {
                m_hole.resize(1);
                m_hole[0] = 0;
            }

            if ( m_contour.size() < 1 ) {
                m_contour.resize(1);
            }

            if ( m_vertex.size() < cnt ) {
                m_vertex.resize(cnt);
            }

            data.num_contours = 1;
            data.hole = m_hole.data();
            data.contour = m_contour.data();

            data.contour[0].num_vertices = (int)cnt;
            data.contour[0].vertex = m_vertex.data();
        }

    public:
        gpc_polygon                     data;

    private:
        std::vector<int>                m_hole;
        std::vector<gpc_vertex_list>    m_contour;
        std::vector<gpc_vertex>         m_vertex;
};

v_geo_coord_t g_dummy_rect;
v_geo_coord_t g_dummy_line;


void __format ( const geo_rect_t& rect, const pos_type_t coord_type, const v_geo_coord_t& polyline ) {

    map_pos_t     dummy_pt1, dummy_pt2, dummy_pt3;
    geo_coord_t   dummy;

    static int stop_cnt = 0;

    g_dummy_rect.clear();
    g_dummy_line.clear();

    rect.min.get(coord_type, dummy_pt1);
    rect.max.get(coord_type, dummy_pt2);

    dummy.set_x(coord_type, dummy_pt1.x);
    dummy.set_y(coord_type, dummy_pt1.y);
    g_dummy_rect.push_back(dummy);

    dummy.set_x(coord_type, dummy_pt2.x);
    dummy.set_y(coord_type, dummy_pt1.y);
    g_dummy_rect.push_back(dummy);

    dummy.set_x(coord_type, dummy_pt2.x);
    dummy.set_y(coord_type, dummy_pt2.y);
    g_dummy_rect.push_back(dummy);

    dummy.set_x(coord_type, dummy_pt1.x);
    dummy.set_y(coord_type, dummy_pt2.y);
    g_dummy_rect.push_back(dummy);

    dummy.set_x(coord_type, dummy_pt1.x);
    dummy.set_y(coord_type, dummy_pt1.y);
    g_dummy_rect.push_back(dummy);

    for (size_t i = 0; i < polyline.size(); i++) {
        polyline[i].get(coord_type, dummy_pt3);
        dummy.set_x(coord_type, dummy_pt3.x);
        dummy.set_y(coord_type, dummy_pt3.y);
        g_dummy_line.push_back(dummy);
    }

    stop_cnt++;

}

static void _reset (gpc_polygon& res ) {
    res.num_contours = 0;
    res.hole = nullptr;
    res.contour = nullptr;
}

void geo_processor_t::geo_intersect ( const v_geo_coord_t& polyline, const geo_rect_t& rect, const pos_type_t coord_type, bool is_area, vv_geo_coord_t& clippedLine ) const {

    gpc_allocator_t subject, clip;

    gpc_polygon result;

    map_pos_t   coord_min;
    map_pos_t   coord_max;
    map_pos_t   coord_src;
    size_t      poly_cnt;

    // __format ( rect, coord_type, polyline );

    clippedLine.clear();
    _reset(result);

    if ( polyline.size() < 2 ) {
        return;
    }

    clip.alloc(4);
    rect.min.get ( coord_type, coord_min );
    rect.max.get ( coord_type, coord_max );
     
    clip.add ( 0, coord_min.x, coord_min.y );
    clip.add ( 1, coord_max.x, coord_min.y );
    clip.add ( 2, coord_max.x, coord_max.y );
    clip.add ( 3, coord_min.x, coord_max.y );

    poly_cnt = polyline.size();
    if ( is_area ) {
        poly_cnt--;
    }

    subject.alloc ( poly_cnt );
    for ( size_t i = 0; i < poly_cnt; i++ ) {
        polyline[i].get ( coord_type, coord_src );
        subject.add ( i, coord_src.x, coord_src.y );
    }

    gpc_polygon_clip ( GPC_INT, &subject.data, &clip.data, &result );

    for ( size_t i = 0; i < result.num_contours; i++ ) {

        v_geo_coord_t sector;
        map_pos_t     pos;
        geo_coord_t   coord;

        sector.clear();

        if ( result.contour[i].num_vertices > 0 ) {

            for ( size_t y = 0; y < result.contour[i].num_vertices; y++ ) {

                pos.x = result.contour[i].vertex[y].x;
                pos.y = result.contour[i].vertex[y].y;

                coord.set(coord_type, pos);

                sector.push_back(coord);
            }

            if ( is_area ) {
                pos.x = result.contour[i].vertex[0].x;
                pos.y = result.contour[i].vertex[0].y;
                coord.set(coord_type, pos);
                sector.push_back(coord);
            }

            clippedLine.push_back(std::move(sector));
        }

    }

    gpc_free_polygon ( &result );

    return;
}

//---------------------------------------------------------------------------//
