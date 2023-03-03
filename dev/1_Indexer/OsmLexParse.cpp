#include "OsmLex.h"
#include <cassert>
#include <cstdarg>
#include <iostream>

#define ITEMS_CNT(x)         ( ( sizeof(x) ) / ( sizeof (x[0]) ) )

// motorwayWidth      = 20m;
// trunkWidth         = 18m;
// primaryWidth       = 14m;
// secondaryWidth     = 12m;
// tertiaryWidth      = 10m;
// roadWidth          =  8m;
// wayWidth           =  8m;
// stepWidth          =  3m;


static const cfg::osm_mapper_t railway_map[] = {
    {   "rail",                        DRAW_PATH_RAILWAY     },
    {   "disused",                     DRAW_SKIP             },
};

static const cfg::osm_mapper_t highway_map[] = {
    {   "motorway",                    DRAW_PATH_MOTORWAY     }, // Автомагистрали 
    {   "motorway_link",               DRAW_PATH_MOTORWAY     }, // 
    {   "trunk",                       DRAW_PATH_TRUNK        }, // Важные дороги
    {   "trunk_link",                  DRAW_PATH_TRUNK        }, // 
    {   "primary",                     DRAW_PATH_PRIMARY      }, // Дороги регионального значения
    {   "primary_link",                DRAW_PATH_PRIMARY      }, // 
    {   "secondary",                   DRAW_PATH_SECONDARY    }, // Дороги областного значения
    {   "secondary_link",              DRAW_PATH_SECONDARY    }, // 
    {   "tertiary",                    DRAW_PATH_TERTIARY     }, // Автомобильных дорог местного значения
    {   "tertiary_link",               DRAW_PATH_TERTIARY     }, // 
    {   "unclassified",                DRAW_PATH_ROAD         }, // Автомобильные дороги местного значения
    {   "residential",                 DRAW_PATH_ROAD         }, // Дороги внутри жилых зон.
    {   "living_street",               DRAW_PATH_ROAD         }, // Улицы такого же класса как residential.
    {   "service",                     DRAW_PATH_ROAD         }, // Служебные проезды
    {   "road",                        DRAW_SKIP              }, // Возможно дороги.
    {   "pedestrian",                  DRAW_PATH_FOOTWAY      }, // Улицы, выделенных для пешеходов.
    {   "track",                       DRAW_PATH_ROAD         }, // Дороги сельскохозяйственного назначения
    {   "footway",                     DRAW_PATH_FOOTWAY      }, // Пешеходные дорожки, тротуары
    {   "steps",                       DRAW_PATH_FOOTWAY      }, // Лестницы, лестничные пролёты.
    {   "path",                        DRAW_PATH_FOOTWAY      }, // стихийная тропа
    {   "bus_guideway",                DRAW_SKIP              }, // Дороги предназначенные только для автобусов.
    {   "escape",                      DRAW_SKIP              }, // Аварийная полоса
    {   "raceway",                     DRAW_SKIP              }, // Гоночная трасса
    {   "bridleway",                   DRAW_SKIP              }, // Дорожки для верховой езды.
    {   "corridor",                    DRAW_SKIP              }, // 
    {   "via_ferrata",                 DRAW_SKIP              }, // горные тропы
    {   "cycleway",                    DRAW_SKIP              }, // велодорожки
    {   "services",                    DRAW_SKIP              }, // СТО, Кафе, ...
    {   "bus_guideway",                DRAW_SKIP              },
    {   "construction",                DRAW_SKIP              },
    {   "proposed",                    DRAW_SKIP              },
    {   "platform",                    DRAW_SKIP              },
    {   "rest_area",                   DRAW_SKIP              },
    {   "virtual",                     DRAW_SKIP              },
    {   "bus_stop",                    DRAW_SKIP              },
    {   "elevator",                    DRAW_SKIP              },
    {   "emergency_bay",               DRAW_SKIP              }
};

static const cfg::osm_mapper_t waterway_map[] = {
    {   "stream",                      DRAW_RIVER     },
    {   "ditch",                       DRAW_RIVER     },
};

static const cfg::osm_mapper_t highway_area[] = {
    {   "pedestrian",                  DRAW_AREA_ASPHALT      },
    {   "living_street",               DRAW_AREA_ASPHALT      },
    {   "platform",                    DRAW_AREA_ASPHALT      },
    {   "residential",                 DRAW_AREA_ASPHALT      },
    {   "bus_stop",                    DRAW_AREA_ASPHALT      },
    {   "service",                     DRAW_AREA_ASPHALT      },
    {   "footway",                     DRAW_AREA_ASPHALT      },
    {   "track",                       DRAW_AREA_ASPHALT      },
    {   "unclassified",                DRAW_AREA_ASPHALT      },
    {   "corridor",                    DRAW_SKIP              },
    {   "elevator",                    DRAW_SKIP              },
};

static const cfg::osm_mapper_t landuse_area[] = {
    {   "aerodrome",                   DRAW_AREA_ASPHALT      },
    {   "basin",                       DRAW_AREA_ASPHALT      },
    {   "cemetery",                    DRAW_AREA_ASPHALT      },
    {   "parking",                     DRAW_AREA_ASPHALT      },
    {   "industrial",                  DRAW_AREA_ASPHALT      },
    {   "retail",                      DRAW_AREA_ASPHALT      },
    {   "military",                    DRAW_AREA_ASPHALT      },
    {   "construction",                DRAW_AREA_ASPHALT      },
    {   "residential",                 DRAW_AREA_ASPHALT      },
    {   "commercial",                  DRAW_AREA_ASPHALT      },

    {   "brownfield",                  DRAW_AREA_GRASS        },
    {   "village_green",               DRAW_AREA_GRASS        },
    {   "grass",                       DRAW_AREA_GRASS        },
    {   "recreation_ground",           DRAW_AREA_GRASS        },
    {   "farmland",                    DRAW_AREA_GRASS        },
    {   "meadow",                      DRAW_AREA_GRASS        },
    {   "farmyard",                    DRAW_AREA_GRASS        },
    {   "vineyard",                    DRAW_AREA_GRASS        },
    {   "orchard",                     DRAW_AREA_GRASS        },
    {   "greenfield",                  DRAW_AREA_GRASS        },

    {   "quarry",                      DRAW_AREA_MOUNTAIN     },
    {   "forest",                      DRAW_AREA_FORSET       },

    {   "railway",                     DRAW_AREA_ASPHALT      },

    {   "allotments",                  DRAW_SKIP              },
    {   "flowerbed",                   DRAW_SKIP              },
    {   "garages",                     DRAW_SKIP              },
    {   "plant_nursery",               DRAW_SKIP              },
    {   "landfill",                    DRAW_SKIP              },
    {   "religious",                   DRAW_SKIP              },
    {   "depot",                       DRAW_SKIP              },
    {   "yes",                         DRAW_SKIP              },

};

static const cfg::osm_mapper_t transport_area[] = {
    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const cfg::osm_mapper_t manmade_area[] = {
    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const cfg::osm_mapper_t waterway_area[] = {
    {   "dam",                         DRAW_AREA_WATER        },
};

static const cfg::osm_mapper_t water_area[] = {
    {   "reservoir",                   DRAW_AREA_WATER        },
};

static const cfg::osm_mapper_t leisure_area[] = {
    {   "track",                       DRAW_SKIP              },
    {   "pitch",                       DRAW_SKIP              },
    {   "garden",                      DRAW_AREA_GRASS        },
    {   "playground",                  DRAW_AREA_GRASS        },
};

static const cfg::osm_mapper_t natural_area[] = {
    {   "scrub",                       DRAW_AREA_FORSET      },
    {   "water",                       DRAW_AREA_WATER       },
};

static const cfg::osm_mapper_t amenity_area[] = {
    {   "parking",                     DRAW_AREA_ASPHALT      },
    {   "parking_space",               DRAW_AREA_ASPHALT      },
};

static const cfg::osm_mapper_t railway_area[] = {
    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const cfg::osm_mapper_t all_area[] = {
    {   "*",                           DRAW_SKIP         },
};

static int   g_err_cnt = 0;

void _log_node (osm_id_t id, const osm_tag_ctx_t& node_info ) {


    std::cout << "<way id=\"" << id << "\">" << std::endl;

    for ( int i = 0; i < node_info.cnt; i++ ) {
        std::cout << "  <tag ";
        std::cout << "k=\"" << node_info.list[i].k << "\" ";
        std::cout << "v=\"" << node_info.list[i].v << "\" ";
        std::cout << "/>" << std::endl;
    }

    std::cout << "</way>" << std::endl;
}

namespace cfg {

    void OsmCfgParser::map_type (osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info, osm_str_t k, const osm_mapper_t* const v_map, size_t cnt, osm_draw_type_t& type ) {

        size_t pos = 0;

        if ( draw_type != DRAW_UNKNOWN ) {
            return;
        }

        for (pos = 0; pos < node_info.cnt; pos++) {
            if (strcmp(node_info.list[pos].k, k) == 0) {
                break;
            }
        }

        if ( pos == node_info.cnt ) {
            return;
        }

        type = DRAW_UNKNOWN;

        for ( size_t i = 0; i < cnt; i++ ) {
            if ( strcmp (node_info.list[pos].v, v_map[i].k ) == 0 ) {
                type = v_map[i].v;
                break;
            }
        }

    }

    bool OsmCfgParser::find_key ( const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1, osm_str_t v2, osm_str_t v3 ) {

        size_t pos = 0;

        for ( pos = 0; pos < node_info.cnt; pos++ ) {
            if ( strcmp(node_info.list[pos].k, k) == 0 ) {
                break;
            }
        }

        if ( pos == node_info.cnt ) {
            return false;
        }

        if ( (v1==nullptr) && (v2==nullptr) && (v3==nullptr)) {
            return true;
        }

        if (v1 != nullptr) {
            if (strcmp(node_info.list[pos].v, v1) == 0) {
                return true;
            }
        }

        if (v2 != nullptr) {
            if (strcmp(node_info.list[pos].v, v2) == 0) {
                return true;
            }
        }

        if (v3 != nullptr) {
            if (strcmp(node_info.list[pos].v, v3) == 0) {
                return true;
            }
        }

        return false;
    }

    void OsmCfgParser::_process_area ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type) {

        bool find_res;

        if ( draw_type != DRAW_UNKNOWN ) {
            return;
        }

        find_res = find_key ( node_info, "area", "yes", "true", "1" );
        if ( !find_res ) {
            return;
        }

        _process_building ( node_info, id, draw_type );

        map_type ( draw_type, node_info, "highway",          highway_area,   ITEMS_CNT(highway_area),   draw_type );
        map_type ( draw_type, node_info, "landuse",          landuse_area,   ITEMS_CNT(landuse_area),   draw_type );
        map_type ( draw_type, node_info, "public_transport", transport_area, ITEMS_CNT(transport_area), draw_type );
        map_type ( draw_type, node_info, "man_made",         manmade_area,   ITEMS_CNT(manmade_area),   draw_type );
        map_type ( draw_type, node_info, "waterway",         waterway_area,  ITEMS_CNT(waterway_area),  draw_type );
        map_type ( draw_type, node_info, "water",            water_area,     ITEMS_CNT(water_area),     draw_type );
        map_type ( draw_type, node_info, "leisure",          leisure_area,   ITEMS_CNT(leisure_area),   draw_type );
        map_type ( draw_type, node_info, "natural",          natural_area,   ITEMS_CNT(natural_area),   draw_type );
        map_type ( draw_type, node_info, "amenity",          amenity_area,   ITEMS_CNT(amenity_area),   draw_type );
        map_type ( draw_type, node_info, "railway",          railway_area,   ITEMS_CNT(railway_area),   draw_type );
        map_type ( draw_type, node_info, "aeroway",          all_area,       ITEMS_CNT(all_area),       draw_type );

        // _log_node (id, node_info);
    }

    void OsmCfgParser::_process_highway ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type ) {

        if ( draw_type != DRAW_UNKNOWN ) {
            return;
        }
        map_type ( draw_type, node_info, "highway",  highway_map, ITEMS_CNT(highway_map), draw_type);
        map_type ( draw_type, node_info, "railway",  railway_map, ITEMS_CNT(railway_map), draw_type);
    }

    void OsmCfgParser::_process_waterway ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type ) {

        if ( draw_type != DRAW_UNKNOWN ) {
            return;
        }
        map_type(draw_type, node_info, "waterway", waterway_map, ITEMS_CNT(waterway_map), draw_type);
    }
        
    void OsmCfgParser::_process_building ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type ) {

        bool find_res;

        if (draw_type != DRAW_UNKNOWN) {
            return;
        }

        find_res = find_key(node_info, "building", nullptr, nullptr, nullptr);
        if ( !find_res ) {
            find_res = find_key(node_info, "building:part", nullptr, nullptr, nullptr );
            if ( !find_res ) {
                return;
            }
        }

        find_res = find_key(node_info, "building", "false", "0", "no");
        if (find_res) {
            return;
        }

        find_res = find_key(node_info, "building:part", "false", "0", "no");
        if (find_res) {
            return;
        }

        draw_type = DRAW_BUILDING;
    }

    void OsmCfgParser::_process_landuse(const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type) {

        if (draw_type != DRAW_UNKNOWN) {
            return;
        }
        map_type(draw_type, node_info, "landuse", landuse_area, ITEMS_CNT(landuse_area), draw_type);
    }

    void OsmCfgParser::_process_amenity(const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type) {

        if (draw_type != DRAW_UNKNOWN) {
            return;
        }
        map_type(draw_type, node_info, "amenity", amenity_area, ITEMS_CNT(amenity_area), draw_type);
    }

    void OsmCfgParser::_process_unused ( const osm_tag_ctx_t& node_info, osm_id_t id, osm_draw_type_t& draw_type ) {

        if (draw_type != DRAW_UNKNOWN) {
            return;
        }

        bool find_res;

        find_res = find_key (node_info, "electrified", nullptr );
        if ( find_res ) {
            draw_type = DRAW_SKIP;
            return;
        }

        find_res = find_key(node_info, "aeroway", nullptr);
        if (find_res) {
            draw_type = DRAW_SKIP;
            return;
        }

        find_res = find_key(node_info, "bicycle", nullptr);
        if (find_res) {
            draw_type = DRAW_SKIP;
            return;
        }

        find_res = find_key(node_info, "communication", nullptr);
        if (find_res) {
            draw_type = DRAW_SKIP;
            return;
        }

        find_res = find_key(node_info, "transformer", nullptr);
        if (find_res) {
            draw_type = DRAW_SKIP;
            return;
        }

        find_res = find_key(node_info, "traffic_calming", nullptr);
        if (find_res) {
            draw_type = DRAW_SKIP;
            return;
        }

    }

    static int val = 0;

    bool OsmCfgParser::ParseWay ( const osm_tag_ctx_t& node_info, osm_id_t id, ref_list_t& ref_list, osm_draw_type_t& draw_type ) {

        draw_type = DRAW_UNKNOWN;

        if ( node_info.cnt == 0 ) {
            draw_type = DRAW_PENDING;
        }

        if (id == 4019771) {
            val++;
        }

        _process_highway  ( node_info, id, draw_type );
        _process_waterway ( node_info, id, draw_type );
        _process_area     ( node_info, id, draw_type );
        _process_building ( node_info, id, draw_type );

        map_type ( draw_type, node_info, "highway",          highway_area,   ITEMS_CNT(highway_area),   draw_type );
        map_type ( draw_type, node_info, "landuse",          landuse_area,   ITEMS_CNT(landuse_area),   draw_type );
        map_type ( draw_type, node_info, "amenity",          amenity_area,   ITEMS_CNT(amenity_area),   draw_type );
        map_type ( draw_type, node_info, "natural",          natural_area,   ITEMS_CNT(natural_area),   draw_type );
        map_type ( draw_type, node_info, "leisure",          leisure_area,   ITEMS_CNT(leisure_area),   draw_type );
        map_type ( draw_type, node_info, "public_transport", transport_area, ITEMS_CNT(transport_area), draw_type );

        _process_unused ( node_info, id, draw_type );

            // if ( ref_list.size() < 3 ) {
            // 
            //      draw_type = DRAW_SKIP;
            // 
            // } else {
            // 
            //     size_t   first_id = 0;
            //     size_t   last_id  = ref_list.size() - 1;
            // 
            //     if ( ref_list[first_id].id != ref_list[last_id].id ) {
            //         ref_list.push_back( ref_list[0] );
            //     }
            // 
            // }

        if ( draw_type == DRAW_UNKNOWN ) {
            _log_node(id, node_info);
        }

        return false;
    }

}

