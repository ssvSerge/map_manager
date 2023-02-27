#include "OsmLex.h"
#include <cassert>
#include <cstdarg>
#include <iostream>

#define ITEMS_CNT(x)         ( ( sizeof(x) ) / ( sizeof (x[0]) ) )

static int   g_err_cnt = 0;

void _log_area ( const osm_tag_ctx_t& node_info ) {


    std::cout << "<way>" <<  std::endl;

    for ( int i = 0; i < node_info.cnt; i++ ) {
        std::cout << "  <tag ";
        std::cout << "k=\"" << node_info.list[i].k << "\" ";
        std::cout << "v=\"" << node_info.list[i].v << "\" ";
        std::cout << "/>" << std::endl;
    }

    std::cout << "</way>" << std::endl;
}

namespace cfg {

    bool OsmCfgParser::map_type (const osm_tag_ctx_t& node_info, osm_str_t k, const osm_mapper_t* const v_map, size_t cnt, osm_draw_type_t& type ) {

        size_t pos = 0;

        for (pos = 0; pos < node_info.cnt; pos++) {
            if (strcmp(node_info.list[pos].k, k) == 0) {
                break;
            }
        }

        if ( pos == node_info.cnt ) {
            return false;
        }

        type = DRAW_UNKNOWN;

        for ( size_t i = 0; i < cnt; i++ ) {
            if ( strcmp (node_info.list[pos].v, v_map[i].k ) == 0 ) {
                type = v_map[i].v;
                break;
            }
        }

        return true;
    }

    bool OsmCfgParser::find_key ( const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1, osm_str_t v2, osm_str_t v3 ) {

        size_t pos = 0;

        for ( pos = 0; pos < node_info.cnt; pos++ ) {
            if ( strcmp(node_info.list[pos].k, k) == 0 ) {
                break;
            }
        }

        if (pos == node_info.cnt) {
            return false;
        }

        if ( strcmp(node_info.list[pos].v, v1) == 0 ) {
            return true;
        }

        if (v2 == nullptr) {
            return false;
        }

        if (strcmp(node_info.list[pos].v, v2) == 0) {
            return true;
        }

        if (v3 == nullptr) {
            return false;
        }

        if (strcmp(node_info.list[pos].v, v3) == 0) {
            return true;
        }

        return false;
    }

    bool OsmCfgParser::_process_area ( const osm_tag_ctx_t& node_info, osm_draw_type_t& draw_type) {

        bool  res;

        // <tag k="leisure" v="track" />
        // <tag k="waterway" v="dam" />
        // <tag k="landuse" v="parking" />
        // <tag k="landuse" v="village_green" />
        // <tag k="railway" v="platform" />
        // <tag k="amenity" v="parking" />
        // <tag k="leisure" v="playground" />
        // <tag k="building" v="yes" />
        // <tag k="footway" v="traffic_island" />
        // <tag k="water" v="reservoir" />
        // <tag k="natural" v="scrub" />
        // <tag k="landuse" v="industrial" />
        // <tag k="landuse" v="brownfield" />
        // <tag k="leisure" v="pitch" />


        res = find_key(node_info, "area", "yes", "1");
        if (!res) {
            return false;
        }

        static const osm_mapper_t all_area[] = {
            {   "*",                           DRAW_SKIP        },
        };

        static const osm_mapper_t highway_area[] = {
            {   "pedestrian",                  DRAW_ASPHALT      },
            {   "living_street",               DRAW_ASPHALT      },
            {   "platform",                    DRAW_ASPHALT      },
            {   "residential",                 DRAW_ASPHALT      },
            {   "bus_stop",                    DRAW_ASPHALT      },
            {   "service",                     DRAW_ASPHALT      },
            {   "footway",                     DRAW_ASPHALT      },
            {   "track",                       DRAW_ASPHALT      },
            {   "unclassified",                DRAW_ASPHALT      },
            {   "corridor",                    DRAW_SKIP         },
            {   "elevator",                    DRAW_SKIP         },
        };

        static const osm_mapper_t transport_area[] = {
            {   "platform",                    DRAW_ASPHALT      },
        };

        static const osm_mapper_t manmade_area[] = {
            {   "platform",                    DRAW_ASPHALT      },
        };

        res = map_type(node_info, "aeroway", highway_area, ITEMS_CNT(highway_area), draw_type);
        if (res) {
            draw_type = DRAW_SKIP;
            return true;
        }

        res = map_type ( node_info, "highway", highway_area, ITEMS_CNT(highway_area), draw_type );
        if (res) {
            if (draw_type == DRAW_UNKNOWN) {
                g_err_cnt++;
            }
            return true;
        }

        res = map_type(node_info, "public_transport", transport_area, ITEMS_CNT(transport_area), draw_type);
        if (res) {
            g_err_cnt++;
            return true;
        }

        res = map_type(node_info, "man_made", manmade_area, ITEMS_CNT(manmade_area), draw_type);
        if (res) {
            g_err_cnt++;
            return true;
        }


        _log_area (node_info);

        return true;
    }

    bool OsmCfgParser::_process_highway ( const osm_tag_ctx_t& node_info, osm_draw_type_t& draw_type ) {

        // motorwayWidth      = 20m;
        // trunkWidth         = 18m;
        // primaryWidth       = 14m;
        // secondaryWidth     = 12m;
        // tertiaryWidth      = 10m;
        // roadWidth          =  8m;
        // wayWidth           =  8m;
        // stepWidth          =  3m;

        bool                find_res;

        static const osm_mapper_t  highway_map[] = {
            {   "motorway",                  DRAW_MOTORWAY      }, // Автомагистрали 
            {   "motorway_link",             DRAW_MOTORWAY      }, // 
            {   "trunk",                     DRAW_TRUNK         }, // Важные дороги
            {   "trunk_link",                DRAW_TRUNK         }, // 
            {   "primary",                   DRAW_PRIMARY       }, // Дороги регионального значения
            {   "primary_link",              DRAW_PRIMARY       }, // 
            {   "secondary",                 DRAW_SECONDARY     }, // Дороги областного значения
            {   "secondary_link",            DRAW_SECONDARY     }, // 
            {   "tertiary",                  DRAW_TERTIARY      }, // Автомобильных дорог местного значения
            {   "tertiary_link",             DRAW_TERTIARY      }, // 
            {   "unclassified",              DRAW_ROAD          }, // Автомобильные дороги местного значения
            {   "residential",               DRAW_ROAD          }, // Дороги внутри жилых зон.
            {   "living_street",             DRAW_ROAD          }, // Улицы такого же класса как residential.
            {   "service",                   DRAW_ROAD          }, // Служебные проезды
            {   "road",                      DRAW_SKIP          }, // Возможно дороги.
            {   "pedestrian",                DRAW_FOOTWAY       }, // Улицы, выделенных для пешеходов.
            {   "track",                     DRAW_ROAD          }, // Дороги сельскохозяйственного назначения
            {   "footway",                   DRAW_FOOTWAY       }, // Пешеходные дорожки, тротуары
            {   "steps",                     DRAW_FOOTWAY       }, // Лестницы, лестничные пролёты.
            {   "path",                      DRAW_FOOTWAY       }, // стихийная тропа
            {   "bus_guideway",              DRAW_SKIP          }, // Дороги предназначенные только для автобусов.
            {   "escape",                    DRAW_SKIP          }, // Аварийная полоса
            {   "raceway",                   DRAW_SKIP          }, // Гоночная трасса
            {   "bridleway",                 DRAW_SKIP          }, // Дорожки для верховой езды.
            {   "corridor",                  DRAW_SKIP          }, // 
            {   "via_ferrata",               DRAW_SKIP          }, // горные тропы
            {   "cycleway",                  DRAW_SKIP          }, // велодорожки
            {   "services",                  DRAW_SKIP          }, // СТО, Кафе, ...
            {   "bus_guideway",              DRAW_SKIP          },
            {   "construction",              DRAW_SKIP          },
            {   "proposed",                  DRAW_SKIP          },
            {   "platform",                  DRAW_SKIP          },
            {   "rest_area",                 DRAW_SKIP          },
            {   "virtual",                   DRAW_SKIP          },
            {   "bus_stop",                  DRAW_SKIP          },
            {   "elevator",                  DRAW_SKIP          },
            {   "emergency_bay",             DRAW_SKIP          }
        };

        find_res = map_type ( node_info, "highway",  highway_map, ITEMS_CNT(highway_map), draw_type);

        if ( ! find_res ) {
            return false;
        }

        if ( draw_type == DRAW_UNKNOWN ) {
            g_err_cnt++;
        }

        return true;
    }
        
    bool OsmCfgParser::ParseWay ( const osm_tag_ctx_t& node_info ) {

        osm_draw_type_t draw_type;

        if ( _process_area(node_info, draw_type) ) {
            return true;
        }

        if ( _process_highway(node_info, draw_type) ) {
            return true;
        }

        return false;
    }

}
