#include <cassert>
#include <iostream>

#include "osm_tools.h"
#include "osm_resolver.h"

#include "..\common\ssearch.h"

#define ITEMS_CNT(x)         ( ( sizeof(x) ) / ( sizeof (x[0]) ) )


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
static ssearcher        g_path_bridge;
static ssearcher        g_ways_ignored;


static const osm_mapper_t map_path_power[] = {
    {   "power",                       DRAW_UNKNOWN          },

    {   "line",                        DRAW_SKIP             },
    {   "substation",                  DRAW_SKIP             },
    {   "minor_line",                  DRAW_SKIP             },
};

static const osm_mapper_t map_path_highway[] = {

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

static const osm_mapper_t map_path_railway[] = {

    {   "railway",                     DRAW_UNKNOWN          },
    {   "proposed",                    DRAW_SKIP             },

    {   "rail",                        DRAW_PATH_RAILWAY     },
    {   "disused",                     DRAW_SKIP             },
    {   "abandoned",                   DRAW_SKIP             },
    {   "razed",                       DRAW_SKIP             },
    {   "tram",                        DRAW_SKIP             },
    {   "funicular",                   DRAW_SKIP             },
    {   "platform_edge",               DRAW_SKIP             },
    {   "turntable",                   DRAW_SKIP             },
    {   "subway",                      DRAW_SKIP             },
    {   "dismantled",                  DRAW_SKIP             },
    {   "miniature" ,                  DRAW_SKIP             },
    {   "loading_ramp" ,               DRAW_SKIP             },
    {   "narrow_gauge" ,               DRAW_SKIP             },
    {   "preserved" ,                  DRAW_SKIP             },
};

static const osm_mapper_t map_path_bridge[] = {

    {   "bridge",                      DRAW_UNKNOWN          },

    {   "yes",                         DRAW_PATH_BRIDGE      },
    {   "auto",                        DRAW_PATH_BRIDGE      },
};

static const osm_mapper_t map_path_waterway[] = {

    {   "waterway",                    DRAW_UNKNOWN           },

    {   "drain",                       DRAW_PATH_RIVER        },
    {   "stream",                      DRAW_PATH_RIVER        },
    {   "ditch",                       DRAW_PATH_RIVER        },
    {   "river",                       DRAW_PATH_RIVER        },
    {   "canal",                       DRAW_PATH_RIVER        },
    {   "weir",                        DRAW_PATH_RIVER        },
    {   "fish_pass",                   DRAW_SKIP              },
    {   "lock_gate",                   DRAW_SKIP              },
    {   "boatyard",                    DRAW_SKIP              },
    {   "dock",                        DRAW_SKIP              },

};

static const osm_mapper_t map_path_natural[] = {

    {   "natural",                     DRAW_UNKNOWN           },
    {   "tree_row",                    DRAW_PATH_FOOTWAY       },
};

static const osm_mapper_t map_path_transport[] = {

    {   "public_transport",            DRAW_UNKNOWN           },
    {   "platform",                    DRAW_SKIP              },
    {   "station",                     DRAW_SKIP              },
    {   "service_point",               DRAW_SKIP              },
    {   "storage_tank",                DRAW_SKIP              },
    {   "embanmkment",                 DRAW_SKIP              },
    {   "bunker_silo",                 DRAW_SKIP              },
    {   "works",                       DRAW_SKIP              },
    {   "ventilation_shaft",           DRAW_SKIP              },
    {   "service_center",              DRAW_SKIP              },
};

static const osm_mapper_t map_area_landuse[] = {

    {   "landuse",                     DRAW_UNKNOWN           },

    {   "aerodrome",                   DRAW_AREA_ASPHALT      },
    {   "commercial",                  DRAW_AREA_ASPHALT      },
    {   "construction",                DRAW_AREA_ASPHALT      },
    {   "parking",                     DRAW_AREA_ASPHALT      },
    {   "industrial",                  DRAW_AREA_ASPHALT      },
    {   "retail",                      DRAW_AREA_ASPHALT      },
    {   "military",                    DRAW_AREA_ASPHALT      },
    {   "residential",                 DRAW_AREA_ASPHALT      },
    {   "garages",                     DRAW_AREA_ASPHALT      },
    {   "railway",                     DRAW_AREA_ASPHALT      },

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
    {   "plant_nursery",               DRAW_AREA_GRASS        },
    {   "landfill",                    DRAW_AREA_GRASS        },
    {   "religious",                   DRAW_AREA_GRASS        },
    {   "allotments",                  DRAW_AREA_GRASS        },
    {   "greenhouse_horticulture",     DRAW_AREA_GRASS        },
    {   "cemetery",                    DRAW_AREA_GRASS        },

    {   "forest",                      DRAW_AREA_FORSET       },

    {   "quarry",                      DRAW_AREA_MOUNTAIN     },

    {   "basin",                       DRAW_WATER             },
    {   "reservoir",                   DRAW_WATER             },
    {   "salt_pond",                   DRAW_WATER             },

    {   "allotments",                  DRAW_SKIP              },
    {   "flowerbed",                   DRAW_SKIP              },
    {   "depot",                       DRAW_SKIP              },
    {   "education",                   DRAW_SKIP              },
    {   "fairground",                  DRAW_SKIP              },
    {   "institutional",               DRAW_SKIP              },
    {   "aquaculture",                 DRAW_SKIP              },
    {   "conservation",                DRAW_SKIP              },
    {   "port",                        DRAW_SKIP              },
    {   "winter_sports",               DRAW_SKIP              },

    {   "animal_keeping",              DRAW_SKIP              },
    {   "churchyard",                  DRAW_SKIP              },
    {   "exposition",                  DRAW_SKIP              },
    {   "radio",                       DRAW_SKIP              },

    {   "yes",                         DRAW_SKIP              },

};

static const osm_mapper_t map_area_leisure[] = {

    {   "leisure",                     DRAW_UNKNOWN           },

    {   "track",                       DRAW_SKIP              },
    {   "pitch",                       DRAW_SKIP              },
    {   "garden",                      DRAW_AREA_GRASS        },
    {   "playground",                  DRAW_AREA_GRASS        },
    {   "stadium",                     DRAW_AREA_GRASS        },
    {   "park",                        DRAW_AREA_GRASS        },
    {   "dog_park",                    DRAW_AREA_GRASS        },
    {   "miniature_golf",              DRAW_AREA_GRASS        },
    {   "meadow",                      DRAW_AREA_GRASS        },

    {   "ice_rink",                    DRAW_SKIP              },
    {   "swimming_pool",               DRAW_SKIP              },
    {   "hospital",                    DRAW_SKIP              },
    {   "sports_centre",               DRAW_SKIP              },
    {   "nature_reserve",              DRAW_SKIP              },
    {   "golf_course",                 DRAW_SKIP              },
    {   "fitness_station",             DRAW_SKIP              },
    {   "horse_riding",                DRAW_SKIP              },
    {   "bandstand",                   DRAW_SKIP              },
    {   "beach_resort",                DRAW_SKIP              },
    {   "outdoor_seating",             DRAW_SKIP              },
    {   "long_jump",                   DRAW_SKIP              },
    {   "marina",                      DRAW_SKIP              },
    {   "fitness_centre",              DRAW_SKIP              },
    {   "bleachers",                   DRAW_SKIP              },
    {   "firepit",                     DRAW_SKIP              },
    {   "water_park",                  DRAW_SKIP              },
    {   "maze",                        DRAW_SKIP              },
    {   "racetrack",                   DRAW_SKIP              },
    {   "trampoline_park",             DRAW_SKIP              },
    {   "drawing_surface",             DRAW_SKIP              },
    {   "practice_pitch",              DRAW_SKIP              },
    {   "pitch;track",                 DRAW_SKIP              },
    {   "soccer_golf",                 DRAW_SKIP              },
    {   "yes",                         DRAW_SKIP              },
};

static const osm_mapper_t map_area_natural[] = {

    {   "natural",                     DRAW_UNKNOWN          },

    {   "stone",                       DRAW_AREA_STONE       },
    {   "scree",                       DRAW_AREA_STONE       },
    {   "grassland",                   DRAW_AREA_GRASS       },
    {   "heath",                       DRAW_AREA_GRASS       },
    {   "scrub",                       DRAW_AREA_GRASS       },
    {   "wood",                        DRAW_AREA_FORSET      },
    {   "bay",                         DRAW_AREA_WATER       },
    {   "water",                       DRAW_AREA_WATER       },
    {   "beach",                       DRAW_AREA_SAND        },
    {   "sand",                        DRAW_AREA_SAND        },
    {   "tree",                        DRAW_SKIP             },
    {   "tree_row",                    DRAW_SKIP             },
    {   "coastline",                   DRAW_SKIP             },
    {   "glacier",                     DRAW_SKIP             },
    {   "mud",                         DRAW_SKIP             },
    {   "shingle",                     DRAW_SKIP             },
    {   "shoal",                       DRAW_SKIP             },
    {   "wetland",                     DRAW_SKIP             },
    {   "arete",                       DRAW_SKIP             },
    {   "bare_rock",                   DRAW_SKIP             },
    {   "cave_entrance",               DRAW_SKIP             },
    {   "cliff",                       DRAW_SKIP             },
    {   "ridge",                       DRAW_SKIP             },
    {   "wetland",                     DRAW_SKIP             },
    {   "heath",                       DRAW_SKIP             },
    {   "rock",                        DRAW_SKIP             },
    {   "bedrock",                     DRAW_SKIP             },
    {   "sinkhole",                    DRAW_SKIP             },
    {   "earth_bank",                  DRAW_SKIP             },
    {   "valley",                      DRAW_SKIP             },
    {   "shrubbery",                   DRAW_SKIP             },
};

static const osm_mapper_t map_area_amenity[] = {

    {   "amenity",                     DRAW_UNKNOWN           },

    {   "parking",                     DRAW_AREA_ASPHALT      },
    {   "parking_space",               DRAW_AREA_ASPHALT      },
    {   "grave_yard",                  DRAW_SKIP              },
    {   "bicycle_parking",             DRAW_SKIP              },
    {   "kindergarten",                DRAW_SKIP              },
    {   "school",                      DRAW_SKIP              },
    {   "college",                     DRAW_SKIP              },
    {   "clinic",                      DRAW_SKIP              },
    {   "fountain",                    DRAW_SKIP              },
    {   "shelter",                     DRAW_SKIP              },
    {   "hospital",                    DRAW_SKIP              },
    {   "university",                  DRAW_SKIP              },
    {   "recycling",                   DRAW_SKIP              },
    {   "motorcycle_parking",          DRAW_SKIP              },
    {   "waste_disposal",              DRAW_SKIP              },
    {   "bench",                       DRAW_SKIP              },
    {   "toilets",                     DRAW_SKIP              },
    {   "trolley_bay",                 DRAW_SKIP              },
    {   "prison",                      DRAW_SKIP              },
    {   "police",                      DRAW_SKIP              },
    {   "marketplace",                 DRAW_SKIP              },
    {   "community_centre",            DRAW_SKIP              },
};

static const osm_mapper_t map_area_railway[] = {

    {   "railway",                     DRAW_UNKNOWN           },

    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const osm_mapper_t map_area_waterway[] = {

    {   "waterway",                    DRAW_UNKNOWN           },

    {   "dam",                         DRAW_AREA_WATER        },
};

static const osm_mapper_t map_area_water[] = {

    {   "water",                       DRAW_UNKNOWN           },

    {   "reservoir",                   DRAW_AREA_WATER        },
    {   "basin",                       DRAW_SKIP              },
};

static const osm_mapper_t map_area_manmade[] = {

    {   "man_made",                    DRAW_UNKNOWN           },

    {   "platform",                    DRAW_AREA_ASPHALT      },
    {   "bridge",                      DRAW_PATH_BRIDGE       },
    {   "tunnel",                      DRAW_PATH_TUNNEL       },

    {   "pipeline",                    DRAW_SKIP              },
    {   "embankment",                  DRAW_SKIP              },
    {   "pier",                        DRAW_SKIP              },
    {   "water_works",                 DRAW_SKIP              },
    {   "reservoir_covered",           DRAW_SKIP              },
    {   "courtyard",                   DRAW_SKIP              },
    {   "street_cabinet",              DRAW_SKIP              },
    {   "silo",                        DRAW_SKIP              },
    {   "wastewater_plant",            DRAW_SKIP              },
    {   "pillar",                      DRAW_SKIP              },
    {   "tower",                       DRAW_SKIP              },
    {   "monitoring_station",          DRAW_SKIP              },
    {   "water_well",                  DRAW_SKIP              },
    {   "water_tower",                 DRAW_SKIP              },
    {   "chimney",                     DRAW_SKIP              },
    {   "antenna",                     DRAW_SKIP              },
    {   "gasometer",                   DRAW_SKIP              },
    {   "storage_tank",                DRAW_SKIP              },
    {   "embanmkment",                 DRAW_SKIP              },
    {   "ventilation_shaft",           DRAW_SKIP              },
    {   "bunker_silo",                 DRAW_SKIP              },
    {   "spoil_heap",                  DRAW_SKIP              },
    {   "mineshaft",                   DRAW_SKIP              },
    {   "works",                       DRAW_SKIP              },
    {   "dyke",                        DRAW_SKIP              },
    {   "beehive",                     DRAW_SKIP              },
    {   "groyne",                      DRAW_SKIP              },
    {   "breakwater",                  DRAW_SKIP              },
    {   "crane",                       DRAW_SKIP              },
    {   "yes",                         DRAW_SKIP              },
};

static const osm_mapper_t map_area_highway[] = {

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

static const osm_mapper_t map_area_transport[] = {

    {   "transport",                   DRAW_UNKNOWN           },

    {   "platform",                    DRAW_AREA_ASPHALT      },
};

static const osm_mapper_t map_area_boundary[] = {

    {   "boundary",                    DRAW_UNKNOWN           },

    {   "administrative",              DRAW_SKIP              },
};

static const osm_mapper_t map_ways_unused[] = {

    {   "unused",                      DRAW_UNKNOWN           },

    {   "electrified",                 DRAW_SKIP              },
    {   "aeroway",                     DRAW_SKIP              },
    {   "bicycle",                     DRAW_SKIP              },
    {   "communication",               DRAW_SKIP              },
    {   "transformer",                 DRAW_SKIP              },
    {   "traffic_calming",             DRAW_SKIP              },
    {   "boundary",                    DRAW_SKIP              },
    {   "route",                       DRAW_SKIP              },
    {   "cycleway",                    DRAW_SKIP              },
};


void map_type ( osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info, const ssearcher& bor ) {

    int     type;
    bool    find_res;
    size_t  pos = 0;

    if (draw_type != DRAW_UNKNOWN) {
        return;
    }

    for (pos = 0; pos < node_info.cnt; pos++) {
        if (strcmp(node_info.list[pos].k, bor.m_key_name.c_str()) == 0) {
            break;
        }
    }

    if (pos == node_info.cnt) {
        return;
    }

    find_res = bor.find(node_info.list[pos].v, type);
    if (!find_res) {
        return;
    }

    draw_type = static_cast<osm_draw_type_t> (type);
}

void bor_init (const osm_mapper_t* const lex_list, size_t cnt, ssearcher& bor) {

    if (lex_list == nullptr) {
        return;
    }

    if (lex_list[0].v != DRAW_UNKNOWN) {
        return;
    }

    bor.init(lex_list[0].k);

    for (size_t i = 1; i < cnt; i++) {
        bor.add(lex_list[i].k, lex_list[i].v);
    }

    return;
}

bool find_key ( const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1 = nullptr, osm_str_t v2 = nullptr, osm_str_t v3 = nullptr) {

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

void process_osm_param ( osm_draw_type_t& draw_type, osm_draw_type_t new_type, const char* const name ) {

    bool find_res;

    if ( draw_type != DRAW_UNKNOWN) {
        return;
    }

    find_res = find_key ( g_xml_tags, name );
    if (find_res) {
        draw_type = new_type;
    }

}

static void _process_building ( osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info ) {

    bool find_res;

    if (draw_type != DRAW_UNKNOWN) {
        return;
    }

    find_res = find_key ( node_info, "building" );
    if ( !find_res ) {
        return;
    }

    find_res = find_key ( node_info, "building", "false", "0", "no" );
    if ( find_res ) {
        return;
    }

    draw_type = DRAW_BUILDING;
}

static bool _is_area ( const osm_tag_ctx_t& xml_tags ) {

    bool find_res;

    find_res = find_key ( xml_tags, "area" );
    if ( !find_res ) {
        return false;
    }

    find_res = find_key ( xml_tags, "area", "false", "0", "no" );
    if ( find_res ) {
        return false;
    }

    return true;
}

static void _process_unused (osm_draw_type_t& draw_type, const osm_tag_ctx_t& xml_tags, const ssearcher& bor ) {

    bool find_res;
    int  type;

    if ( draw_type != DRAW_UNKNOWN ) {
        return;
    }

    for ( int i = 0; i < xml_tags.cnt; i++ ) {
        find_res = bor.find ( xml_tags.list[i].k, type );
        if ( find_res ) {
            draw_type = DRAW_SKIP;
            break;
        }
    }

}

static void _log_node ( osm_id_t id, const osm_tag_ctx_t& node_info ) {

    std::cout << "<way id=\"" << id << "\">" << std::endl;

    for (int i = 0; i < node_info.cnt; i++) {
        std::cout << "  <tag ";
        std::cout << "k=\"" << node_info.list[i].k << "\" ";
        std::cout << "v=\"" << node_info.list[i].v << "\" ";
        std::cout << "/>" << std::endl;
    }

    std::cout << "</way>" << std::endl;
}

void ways_init (void) {

    bor_init ( map_ways_unused,    ITEMS_CNT(map_ways_unused),    g_ways_ignored );

    bor_init ( map_area_landuse,   ITEMS_CNT(map_area_landuse),   g_area_landuse );
    bor_init ( map_area_leisure,   ITEMS_CNT(map_area_leisure),   g_area_leisure );
    bor_init ( map_area_natural,   ITEMS_CNT(map_area_natural),   g_area_natural );
    bor_init ( map_area_amenity,   ITEMS_CNT(map_area_amenity),   g_area_amenity );
    bor_init ( map_area_waterway,  ITEMS_CNT(map_area_waterway),  g_area_waterway );
    bor_init ( map_area_manmade,   ITEMS_CNT(map_area_manmade),   g_area_manmade );
    bor_init ( map_area_highway,   ITEMS_CNT(map_area_highway),   g_area_highway );
    bor_init ( map_area_transport, ITEMS_CNT(map_area_transport), g_area_transport );
    bor_init ( map_area_water,     ITEMS_CNT(map_area_water),     g_area_water );
    bor_init ( map_area_railway,   ITEMS_CNT(map_area_railway),   g_area_railway );
    bor_init ( map_area_boundary,  ITEMS_CNT(map_area_boundary),  g_area_boundary );

    bor_init ( map_path_highway,   ITEMS_CNT(map_path_highway),   g_path_highway );
    bor_init ( map_path_railway,   ITEMS_CNT(map_path_railway),   g_path_railway );
    bor_init ( map_path_waterway,  ITEMS_CNT(map_path_waterway),  g_path_waterway );
    bor_init ( map_path_natural,   ITEMS_CNT(map_path_natural),   g_path_natural );
    bor_init ( map_path_transport, ITEMS_CNT(map_path_transport), g_path_transport );
    bor_init ( map_path_power,     ITEMS_CNT(map_path_power),     g_path_power );
    bor_init ( map_path_bridge,    ITEMS_CNT(map_path_bridge),    g_path_bridge );
}

void ways_expand_tags ( void ) {

    test_and_add ( "disused" );
    test_and_add ( "bridge" );
    test_and_add ( "building" );
    test_and_add ( "abandoned" );
    test_and_add ( "area" );
    test_and_add ( "proposed" );
    test_and_add ( "construction" );
    test_and_add ( "cycleway" );
    test_and_add ( "demolished" );
}

void ways_resolve_type ( void ) {

    osm_draw_type_t draw_type = DRAW_UNKNOWN;

    if ( g_xml_tags.cnt == 0 ) {
        draw_type = DRAW_PENDING;
        return;
    }

    process_osm_param ( draw_type, DRAW_SKIP, "abandoned" );
    process_osm_param ( draw_type, DRAW_SKIP, "disused" );
    process_osm_param ( draw_type, DRAW_SKIP, "construction" );
    process_osm_param ( draw_type, DRAW_SKIP, "proposed" );
    process_osm_param ( draw_type, DRAW_SKIP, "demolished" );

    _process_building ( draw_type, g_xml_tags );

    if ( _is_area(g_xml_tags) ) {

        map_type ( draw_type, g_xml_tags, g_area_highway );
        map_type ( draw_type, g_xml_tags, g_area_transport );
        map_type ( draw_type, g_xml_tags, g_area_landuse );
        map_type ( draw_type, g_xml_tags, g_area_leisure );
        map_type ( draw_type, g_xml_tags, g_area_natural );
        map_type ( draw_type, g_xml_tags, g_area_amenity );
        map_type ( draw_type, g_xml_tags, g_area_waterway );
        map_type ( draw_type, g_xml_tags, g_area_manmade );
        map_type ( draw_type, g_xml_tags, g_area_water );
        map_type ( draw_type, g_xml_tags, g_area_railway );
        map_type ( draw_type, g_xml_tags, g_area_boundary );

        if ( draw_type == DRAW_UNKNOWN ) {
            draw_type = DRAW_AREA_UNKNOWN;
        }

    } else {
    
        map_type ( draw_type, g_xml_tags, g_path_highway );
        map_type ( draw_type, g_xml_tags, g_path_railway );
        map_type ( draw_type, g_xml_tags, g_path_waterway );
        map_type ( draw_type, g_xml_tags, g_path_natural );
        map_type ( draw_type, g_xml_tags, g_path_transport );
        map_type ( draw_type, g_xml_tags, g_path_power );
        map_type ( draw_type, g_xml_tags, g_path_bridge );

        map_type ( draw_type, g_xml_tags, g_area_amenity );
        map_type ( draw_type, g_xml_tags, g_area_water );
        map_type ( draw_type, g_xml_tags, g_area_waterway );
        map_type ( draw_type, g_xml_tags, g_area_leisure );
        map_type ( draw_type, g_xml_tags, g_area_natural );
        map_type ( draw_type, g_xml_tags, g_area_landuse );
        map_type ( draw_type, g_xml_tags, g_area_manmade );

    }

    _process_unused ( draw_type, g_xml_tags, g_ways_ignored );

    if ( draw_type == DRAW_UNKNOWN ) {
        _log_node ( g_node_info.id, g_xml_tags );
    }

    g_node_info.type = draw_type;
}

void ways_store_info ( void ) {

}

void ways_clean_info ( void ) {

    assert(g_xml_tags.cnt < OSM_MAX_TAGS_CNT);

    while (g_xml_tags.cnt > 0) {
        g_xml_tags.cnt--;
        g_xml_tags.list[g_xml_tags.cnt].k[0] = 0;
        g_xml_tags.list[g_xml_tags.cnt].v[0] = 0;
    }

    memset(&g_node_info, 0, sizeof(g_node_info));
    g_ref_list.clear();
}
