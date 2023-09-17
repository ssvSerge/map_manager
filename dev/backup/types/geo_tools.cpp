#include <geo_tools.h>

#if 0

//-----------------------------------------------------------------//

typedef enum tag_pt_code_pos {
    CODE_POS_IN,
    CODE_POS_LEFT,
    CODE_POS_RIGHT,
    CODE_POS_TOP,
    CODE_POS_BOTTOM
}   pt_code_pos_t;

//-----------------------------------------------------------------//

static void _map_pt_pos ( const geo_coord_t& _point, const geo_rect_t& _rect, const pos_type_t& _type, pt_code_pos_t& code) {

    map_pos_t pt;
    map_pos_t cmin, cmax;

    _point.get    ( pt,   _type );
    _rect.min.get ( cmin, _type );
    _rect.max.get ( cmax, _type );

    if ( pt.x < cmin.x ) {
        code = CODE_POS_LEFT;
    } else
    if ( pt.x > cmax.x ) {
        code = CODE_POS_RIGHT;
    } else
    if ( pt.y > cmax.y ) {
        code = CODE_POS_TOP;
    } else
    if ( pt.y < cmin.y ) {
        code = CODE_POS_BOTTOM;
    } else {
        code = CODE_POS_IN;
    }
}

static bool _get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const geo_coord_t& p3, const geo_coord_t& p4, geo_coord_t& point ) {

    assert ( (p3.x == p4.x)  ||  (p3.y == p4.y)  );
    assert ( p3.x <= p4.x );
    assert ( p3.y <= p4.y );

    if ( p3.x == p4.x ) {

        double y = p1.y + ((p2.y - p1.y) * (p3.x - p1.x)) / (p2.x - p1.x);

        if ( (y > p4.y) || (y < p3.y) ) {
            return false;
        }

        if ( p1.y < p2.y ) {
            if ( (y < p1.y) || (y > p2.y) ) {
                return false;
            }
        } else {
            if ( (y < p2.y) || (y > p1.y) ) {
                return false;
            }
        }

        point.x = p3.x;
        point.y = y;

    } else {

        double x = p1.x + ((p2.x - p1.x) * (p3.y - p1.y)) / (p2.y - p1.y);

        if ( (x > p4.x) || (x < p3.x) ) {
            return false;
        }

        if ( p1.x < p2.x ) {
            if ( (x < p1.x) || (x > p2.x) ) {
                return false;
            }
        } else {
            if ( (x < p2.x) || (x > p1.x) ) {
                return false;
            }
        }

        point.x = x;
        point.y = p3.y;
    }

    return true;
}

static bool _get_intersection_pt ( const geo_coord_t& p1, const geo_coord_t& p2, const pt_code_pos_t border, const geo_rect_t& rect, geo_coord_t& point ) {

    geo_coord_t p3, p4;

    switch (border) {

        case CODE_POS_LEFT:
            p3.x = p4.x = rect.min.x;
            p3.y = rect.min.y;
            p4.y = rect.max.y;
            break;

        case CODE_POS_RIGHT:
            p3.x = p4.x = rect.max.x;
            p3.y = rect.min.y;
            p4.y = rect.max.y;
            break;

        case CODE_POS_TOP:
            p3.y = p4.y = rect.max.y;
            p3.x = rect.min.x;
            p4.x = rect.max.x;
            break;

        case CODE_POS_BOTTOM:
            p3.y = p4.y = rect.min.y;
            p3.x = rect.min.x;
            p4.x = rect.max.x;
            break;
    }

    return _get_intersection_pt ( p1, p2, p3, p4, point );
}

//-----------------------------------------------------------------//

void geo_intersect ( const v_geo_coord_t& polyline, const geo_rect_t& rect, const pos_type_t& _type, vv_geo_coord_t& clippedLine ) {

    pt_code_pos_t pos_prev, pos_new;
    v_geo_coord_t segment;
    geo_coord_t   pt;

    clippedLine.clear();

    if ( polyline.size() < 2 ) {
        return;
    }

    _map_pt_pos ( polyline[0], rect, _type, pos_prev );
    if ( pos_prev == CODE_POS_IN ) {
        segment.push_back( polyline[0] );
    }

    for ( size_t i = 1; i < polyline.size(); i++ ) {
        
        _map_pt_pos ( polyline[i], rect, _type, pos_new );

        if ( pos_new == CODE_POS_IN ) {

            if ( pos_prev != CODE_POS_IN ) {
                if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_prev, rect, pt) ) {
                    segment.push_back ( pt );
                }
            }

            segment.push_back ( polyline[i] );

            pos_prev = pos_new;
            continue;
        }

        if ( pos_prev == CODE_POS_IN ) {

            if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_new, rect, pt) ) {
                segment.push_back(pt);
                if (segment.size() >= 2) {
                    clippedLine.push_back(segment);
                }
                segment.clear();
            }

            pos_prev = pos_new;
            continue;
        }

        if ( pos_prev == pos_new ) {
            continue;
        }

        if ( segment.size() > 0 ) {
            clippedLine.push_back ( segment );
            segment.clear();
            continue;
        }

        if ( pos_prev == pos_new ) {
            continue;
        }

        if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_prev, rect, pt) ) {
            segment.push_back(pt);
            if ( _get_intersection_pt(polyline[i - 1], polyline[i], pos_new, rect, pt) ) {
                segment.push_back(pt);
            }
            clippedLine.push_back(segment);
            segment.clear();
        }

        pos_prev = pos_new;

    }

    if ( segment.size() > 0 ) {
        clippedLine.push_back ( segment );
    }

    return;
}

//-----------------------------------------------------------------//

#endif
