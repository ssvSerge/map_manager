#include <queue>
#include <vector>

#include <geo_processor.h>
#include <geo_projection.h>

#include <gpc.h>

v_geo_coord_t   g_dummy_rect;
v_geo_coord_t   g_dummy_line_in;
vv_geo_coord_t  g_dummy_line_out;

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

        gpc_polygon                     data;
        std::vector<int>                m_hole;
        std::vector<gpc_vertex_list>    m_contour;
        std::vector<gpc_vertex>         m_vertex;
};

void __format ( const gpc_polygon& in_line, const gpc_polygon& rect, const gpc_polygon& out_line ) {

    geo_coord_t coord;

    g_dummy_rect.clear();
    g_dummy_line_in.clear();
    g_dummy_line_out.clear();

    for ( size_t i = 0; i < rect.num_contours; i++ ) {
        for ( size_t j = 0; j < rect.contour[i].num_vertices; j++ ) {
            coord.geo.x = rect.contour[i].vertex[j].x;
            coord.geo.y = rect.contour[i].vertex[j].y;
            g_dummy_rect.push_back(coord);
        }
    }
    g_dummy_rect.push_back ( g_dummy_rect.front() );

    for ( size_t i = 0; i < in_line.num_contours; i++ ) {
        for ( size_t j = 0; j < in_line.contour[i].num_vertices; j++ ) {
            coord.geo.x = in_line.contour[i].vertex[j].x;
            coord.geo.y = in_line.contour[i].vertex[j].y;
            g_dummy_line_in.push_back(coord);
        }
    }
    g_dummy_line_in.push_back (  g_dummy_line_in.front() );

    for ( size_t i = 0; i < out_line.num_contours; i++ ) {
        v_geo_coord_t  tmp;
        for ( size_t j = 0; j < out_line.contour[i].num_vertices; j++ ) {
            coord.geo.x = out_line.contour[i].vertex[j].x;
            coord.geo.y = out_line.contour[i].vertex[j].y;
            tmp.push_back(coord);
        }
        tmp.push_back( tmp.front() );

        g_dummy_line_out.push_back(tmp);

    }
}

static void _reset ( gpc_polygon& res ) {

    res.num_contours = 0;
    res.hole = nullptr;
    res.contour = nullptr;
}

static bool _pt_in_rect(const geo_rect_t& rect, const gpc_vertex& vertex ) {

    if ( vertex.x <  rect.min.ang.x ) {
        return false;
    }
    if ( vertex.x >= rect.max.ang.x ) {
        return false;
    }

    if ( vertex.y <  rect.min.ang.y ) {
        return false;
    }
    if ( vertex.y >= rect.max.ang.y ) {
        return false;
    }

    return true;
}

static void _validate_result ( const geo_rect_t& rect, const gpc_polygon& result ) {

    bool is_pt_valid;

    for ( int contour_id = 0; contour_id < result.num_contours; contour_id++ ) {

        for (int vertex_id = 0; vertex_id < result.contour[contour_id].num_vertices; vertex_id++) {
            is_pt_valid = _pt_in_rect ( rect, result.contour[contour_id].vertex[vertex_id] );
            if ( !is_pt_valid ) {
                assert(false);
            }
        }
    }

}

void geo_processor_t::geo_intersect ( const pos_type_t coord_type, bool is_area, const geo_line_t& inp_path, const geo_rect_t& rect, v_geo_line_t& out_path ) const {

    gpc_allocator_t     clip;
    gpc_allocator_t     subject;
    gpc_polygon         result;

    static int stop_cnt = 0;

    double  min_x;
    double  min_y;
    double  max_x;
    double  max_y;
    double  pos_x;
    double  pos_y;
    size_t  pt_cnt;

    _reset ( result );

    clip.alloc(4);

    rect.min.get ( coord_type, min_x, min_y );
    rect.max.get ( coord_type, max_x, max_y );

    max_x -= 1;
    max_y -= 1;

    clip.add ( 0, min_x, min_y );
    clip.add ( 1, max_x, min_y );
    clip.add ( 2, max_x, max_y );
    clip.add ( 3, min_x, max_y );
    
    pt_cnt = inp_path.m_coords.size();
    if ( is_area ) {
        pt_cnt--;
    }

    subject.alloc ( pt_cnt );

    for ( size_t i = 0; i < pt_cnt; i++ ) {
        inp_path.m_coords[i].get(coord_type, pos_x, pos_y);
        subject.add ( i, pos_x, pos_y );
    }
    
    gpc_polygon_clip ( GPC_INT, &subject.data, &clip.data, &result );

    if (result.num_contours > 2) {  
        stop_cnt++;
    }

    #if 1
        _validate_result ( rect, result );
    #endif


    #if 0
    __format ( subject.data, clip.data, result );
    #endif

    out_path.clear();

    geo_coord_t   coord;

    for (size_t i = 0; i < result.num_contours; i++) {

        geo_line_t    sector;

        sector.m_role = inp_path.m_role;
        sector.m_type = inp_path.m_type;
        sector.m_area = inp_path.m_area;

        if ( result.contour[i].num_vertices <= 1 ) {
            continue;
        }

        sector.m_coords.reserve( result.contour[i].num_vertices + 1 );

        for ( size_t y = 0; y < result.contour[i].num_vertices; y++ ) {
            coord.set ( coord_type, result.contour[i].vertex[y].x, result.contour[i].vertex[y].y );
            sector.m_coords.push_back ( coord );
        }

        if ( is_area ) {
            sector.m_coords.push_back ( sector.m_coords.front() );
        }

        out_path.push_back ( std::move(sector) );
    }

    gpc_free_polygon ( &result );
}

//---------------------------------------------------------------------------//
