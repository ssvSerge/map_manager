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

static const cfg::osm_mapper_t map_path_power[] = {

    {   "power",                       DRAW_UNKNOWN          },

    {   "line",                        DRAW_SKIP             },
    {   "substation",                  DRAW_SKIP             },
    {   "minor_line",                  DRAW_SKIP             },
};

static const cfg::osm_mapper_t map_path_railway[] = {

    {   "railway",                     DRAW_UNKNOWN          },

    {   "rail",                        DRAW_PATH_RAILWAY     },
    {   "disused",                     DRAW_SKIP             },
    {   "abandoned",                   DRAW_SKIP             },
    {   "razed",                       DRAW_SKIP             },
    {   "tram",                        DRAW_SKIP             },
};

static const cfg::osm_mapper_t map_path_highway[] = {

    {   "highway",                     DRAW_UNKNOWN           },

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

static const cfg::osm_mapper_t map_path_waterway[] = {

    {   "waterway",                    DRAW_UNKNOWN           },

    {   "drain",                       DRAW_PATH_RIVER        },
    {   "stream",                      DRAW_PATH_RIVER        },
    {   "ditch",                       DRAW_PATH_RIVER        },
    {   "river",                       DRAW_PATH_RIVER        },
    {   "canal",                       DRAW_PATH_RIVER        },
    {   "weir",                        DRAW_PATH_RIVER        },
};

static const cfg::osm_mapper_t map_path_natural[] = {

    {   "natural",                     DRAW_UNKNOWN           },
    {   "tree_row",                    DRAW_PATH_FOOTWAY       },
};

static const cfg::osm_mapper_t map_path_transport[] = {

    {   "public_transport",            DRAW_UNKNOWN           },
    {   "platform",                    DRAW_SKIP              },
    {   "station",                     DRAW_SKIP              },
};

static const cfg::osm_mapper_t map_area_highway[] = {

    {   "highway",                     DRAW_UNKNOWN           },

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

static const cfg::osm_mapper_t map_area_landuse[] = {

    {   "landuse",                     DRAW_UNKNOWN           },

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

static const cfg::osm_mapper_t map_area_transport[] = {

    {   "transport",                   DRAW_UNKNOWN           },

    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const cfg::osm_mapper_t map_area_manmade[] = {

    {   "manmade",                     DRAW_UNKNOWN           },

    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const cfg::osm_mapper_t map_area_waterway[] = {

    {   "waterway",                    DRAW_UNKNOWN           },

    {   "dam",                         DRAW_AREA_WATER        },
};

static const cfg::osm_mapper_t map_area_water[] = {

    {   "water",                       DRAW_UNKNOWN           },

    {   "reservoir",                   DRAW_AREA_WATER        },
};

static const cfg::osm_mapper_t map_area_leisure[] = {

    {   "leisure",                     DRAW_UNKNOWN           },

    {   "track",                       DRAW_SKIP              },
    {   "pitch",                       DRAW_SKIP              },
    {   "garden",                      DRAW_AREA_GRASS        },
    {   "playground",                  DRAW_AREA_GRASS        },
    {   "stadium",                     DRAW_AREA_GRASS        },
    {   "park",                        DRAW_AREA_GRASS        },
    {   "dog_park",                    DRAW_AREA_GRASS        },
    {   "ice_rink",                    DRAW_SKIP              },
    {   "swimming_pool",               DRAW_SKIP              },
    {   "hospital",                    DRAW_SKIP              },
};

static const cfg::osm_mapper_t map_area_natural[] = {

    {   "natural",                     DRAW_UNKNOWN           },

    {   "scrub",                       DRAW_AREA_FORSET      },
    {   "water",                       DRAW_AREA_WATER       },
    {   "grassland",                   DRAW_AREA_WATER       },
    {   "cliff",                       DRAW_SKIP             },
    {   "mud",                         DRAW_SKIP             },
    {   "wetland",                     DRAW_SKIP             },
    {   "wood",                        DRAW_SKIP             },
    {   "sand",                        DRAW_SKIP             },
};

static const cfg::osm_mapper_t map_area_amenity[] = {

    {   "amenity",                     DRAW_UNKNOWN           },

    {   "parking",                     DRAW_AREA_ASPHALT      },
    {   "parking_space",               DRAW_AREA_ASPHALT      },
    {   "bicycle_parking",             DRAW_SKIP              },
    {   "kindergarten",                DRAW_SKIP              },
    {   "school",                      DRAW_SKIP              },
    {   "college",                     DRAW_SKIP              },
    {   "clinic",                      DRAW_SKIP              },
    {   "fountain",                    DRAW_SKIP              },
};

static const cfg::osm_mapper_t map_area_railway[] = {

    {   "railway",                     DRAW_UNKNOWN           },

    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const cfg::osm_mapper_t map_area_boundary[] = {

    {   "boundary",                    DRAW_UNKNOWN           },

    {   "administrative",              DRAW_SKIP              },
};


static const cfg::osm_mapper_t all_area[] = {
    {   "*",                           DRAW_SKIP         },
};

static ssearcher        g_area_landuse;
static ssearcher        g_area_leisure;
static ssearcher        g_area_natural;
static ssearcher        g_area_amenity;
static ssearcher        g_area_waterway;
static ssearcher        g_area_manmade;
static ssearcher        g_area_highway;
static ssearcher        g_area_transport;
static ssearcher        g_area_water;
static ssearcher        g_area_railway;
static ssearcher        g_area_boundary;
static ssearcher        g_path_highway;
static ssearcher        g_path_railway;
static ssearcher        g_path_waterway;
static ssearcher        g_path_natural;
static ssearcher        g_path_transport;
static ssearcher        g_path_power;


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

    OsmCfgParser::OsmCfgParser() {

        _bor_init ( map_area_landuse,   ITEMS_CNT(map_area_landuse),    g_area_landuse   );
        _bor_init ( map_area_leisure,   ITEMS_CNT(map_area_leisure),    g_area_leisure   );
        _bor_init ( map_area_natural,   ITEMS_CNT(map_area_natural),    g_area_natural   );
        _bor_init ( map_area_amenity,   ITEMS_CNT(map_area_amenity),    g_area_amenity   );
        _bor_init ( map_area_waterway,  ITEMS_CNT(map_area_waterway),   g_area_waterway  );
        _bor_init ( map_area_manmade,   ITEMS_CNT(map_area_manmade),    g_area_manmade   );
        _bor_init ( map_area_highway,   ITEMS_CNT(map_area_highway),    g_area_highway   );
        _bor_init ( map_area_transport, ITEMS_CNT(map_area_transport),  g_area_transport );
        _bor_init ( map_area_water,     ITEMS_CNT(map_area_water),      g_area_water     );
        _bor_init ( map_area_railway,   ITEMS_CNT(map_area_railway),    g_area_railway   );
        _bor_init ( map_area_boundary,  ITEMS_CNT(map_area_boundary),   g_area_boundary  );

        _bor_init ( map_path_highway,   ITEMS_CNT(map_path_highway),    g_path_highway   );
        _bor_init ( map_path_railway,   ITEMS_CNT(map_path_railway),    g_path_railway   );
        _bor_init ( map_path_waterway,  ITEMS_CNT(map_path_waterway),   g_path_waterway  );
        _bor_init ( map_path_natural,   ITEMS_CNT(map_path_natural),    g_path_natural   );
        _bor_init ( map_path_transport, ITEMS_CNT(map_path_transport),  g_path_transport );
        _bor_init ( map_path_power,     ITEMS_CNT(map_path_power),      g_path_power     );
    }

    void OsmCfgParser::_bor_init ( const osm_mapper_t* const lex_list, size_t cnt, ssearcher& bor ) {

        if (lex_list == nullptr) {
            return;
        }

        if (lex_list[0].v != DRAW_UNKNOWN) {
            return;
        }

        bor.init ( lex_list[0].k );

        for ( size_t i = 1; i < cnt; i++ ) {
            bor.add(lex_list[i].k, lex_list[i].v);
        }

        return;
    }

    void  OsmCfgParser::map_type ( osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info, const ssearcher& bor ) {

        int     type;
        bool    find_res;
        size_t  pos = 0;

        if (draw_type != DRAW_UNKNOWN) {
            return;
        }

        for (pos = 0; pos < node_info.cnt; pos++) {
            if (strcmp(node_info.list[pos].k, bor.m_key_name.c_str() ) == 0) {
                break;
            }
        }

        if ( pos == node_info.cnt ) {
            return;
        }

        find_res = bor.find ( node_info.list[pos].v, type );
        if ( !find_res ) {
            return;
        }

        draw_type = static_cast<osm_draw_type_t> (type);
    }

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
            if (strcmp(node_info.list[pos].k, k) == 0 ) {
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

    void OsmCfgParser::ParseWay ( const osm_tag_ctx_t& node_info, osm_id_t id, ref_list_t& ref_list, osm_draw_type_t& draw_type ) {

        bool find_res;

        draw_type = DRAW_UNKNOWN;

        if ( node_info.cnt == 0 ) {
            draw_type = DRAW_PENDING;
        }

        find_res = find_key(node_info, "disused", nullptr );
        if (find_res) {
            draw_type = DRAW_SKIP;
        }
        find_res = find_key(node_info, "construction", nullptr );
        if (find_res) {
            draw_type = DRAW_SKIP;
        }
        find_res = find_key(node_info, "proposed", nullptr);
        if (find_res) {
            draw_type = DRAW_SKIP;
        }

        _process_building ( node_info, id, draw_type );

        map_type ( draw_type, node_info, g_area_highway   );
        map_type ( draw_type, node_info, g_area_transport );
        map_type ( draw_type, node_info, g_area_landuse   );
        map_type ( draw_type, node_info, g_area_leisure   );
        map_type ( draw_type, node_info, g_area_natural   );
        map_type ( draw_type, node_info, g_area_amenity   );
        map_type ( draw_type, node_info, g_area_waterway  );
        map_type ( draw_type, node_info, g_area_manmade   );
        map_type ( draw_type, node_info, g_area_water     );
        map_type ( draw_type, node_info, g_area_railway   );
        map_type ( draw_type, node_info, g_area_boundary  );

        find_res = find_key(node_info, "area", "yes", "true", "auto");
        if ( find_res ) {
            if ( draw_type == DRAW_UNKNOWN ) {
                draw_type = DRAW_AREA_UNKNOWN;
            }
        }

        if ( (draw_type > DRAW_AREA_BEGIN) && (draw_type < DRAW_AREA_END) ) {
            size_t   first_id = 0;
            size_t   last_id  = ref_list.size() - 1;
            if ( ref_list[first_id].id != ref_list[last_id].id ) {
                ref_list.push_back( ref_list[0] );
            }
        }

        map_type ( draw_type, node_info, g_path_highway   );
        map_type ( draw_type, node_info, g_path_railway   );
        map_type ( draw_type, node_info, g_path_waterway  );
        map_type ( draw_type, node_info, g_path_natural   );
        map_type ( draw_type, node_info, g_path_transport );
        map_type ( draw_type, node_info, g_path_power     );

        _process_unused ( node_info, id, draw_type );

        if ( draw_type == DRAW_UNKNOWN ) {
            _log_node(id, node_info);
        }
    }

}

