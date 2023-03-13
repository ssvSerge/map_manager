#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>

#include "osm_processor.h"

//---------------------------------------------------------------------------//

#define ITEMS_CNT(x)         ( ( sizeof(x) ) / ( sizeof (x[0]) ) )
#define MAP_TYPE(p,c)        ( ((int)(p)<<16) + ((int)(c)) )
#define OSM_TYPE_PREV        ( (int)(xml_ctx[xml_ctx_cnt-2]) )
#define OSM_TYPE_CURR        ( (int)(xml_ctx[xml_ctx_cnt-1]) )

//---------------------------------------------------------------------------//

static const osm_mapper_t map_path_stairwell[] = {
    {   "stairwell",                   DRAW_UNKNOWN          },
    {   "flight_of_stairs",            DRAW_SKIP             },
    {   "stair_landing",               DRAW_SKIP             },
    {   "yes",                         DRAW_SKIP             },
};

static const osm_mapper_t map_path_golf[] = {
    {   "golf",                        DRAW_UNKNOWN          },
    {   "fairway",                     DRAW_SKIP             },
    {   "hole",                        DRAW_SKIP             },
    {   "tee",                         DRAW_SKIP             },
    {   "green",                       DRAW_SKIP             },
    {   "bunker",                      DRAW_SKIP             },
};

static const osm_mapper_t map_path_shop[] = {
    {   "shop",                        DRAW_UNKNOWN          },
    {   "stationery",                  DRAW_SKIP             },
    {   "car",                         DRAW_SKIP             },
    {   "garden_centre",               DRAW_SKIP             },
    {   "convenience",                 DRAW_SKIP             },
    {   "kiosk",                       DRAW_SKIP             },
    {   "vacant",                      DRAW_SKIP             },
    {   "supermarket",                 DRAW_SKIP             },
    {   "newsagent",                   DRAW_SKIP             },
    {   "optician",                    DRAW_SKIP             },
    {   "clothes",                     DRAW_SKIP             },
    {   "florist",                     DRAW_SKIP             },
    {   "car_repair",                  DRAW_SKIP             },
    {   "furniture",                   DRAW_SKIP             },
    {   "ticket",                      DRAW_SKIP             },
    {   "gift",                        DRAW_SKIP             },
    {   "storage_rental",              DRAW_SKIP             },
    {   "toys",                        DRAW_SKIP             },
    {   "chemist",                     DRAW_SKIP             },
    {   "books",                       DRAW_SKIP             },
    {   "variety_store",               DRAW_SKIP             },
    {   "fashion_accessories",         DRAW_SKIP             },
    {   "tea",                         DRAW_SKIP             },
    {   "pawnbroker",                  DRAW_SKIP             },
    {   "butcher",                     DRAW_SKIP             },
    {   "bakery",                      DRAW_SKIP             },
    {   "travel_agency",               DRAW_SKIP             },
    {   "cosmetics",                   DRAW_SKIP             },
    {   "bag",                         DRAW_SKIP             },
    {   "beauty",                      DRAW_SKIP             },
    {   "shoes",                       DRAW_SKIP             },
    {   "nutrition_supplements",       DRAW_SKIP             },
    {   "beverages",                   DRAW_SKIP             },
    {   "watches",                     DRAW_SKIP             },
    {   "model",                       DRAW_SKIP             },
};

static const osm_mapper_t map_path_barrier[] = {
    {   "barrier",                     DRAW_UNKNOWN          },
    {   "wall",                        DRAW_SKIP             },
    {   "fence",                       DRAW_SKIP             },
    {   "city_wall",                   DRAW_SKIP             },
    {   "retaining_wall",              DRAW_SKIP             },
    {   "gate",                        DRAW_SKIP             },
    {   "hedge",                       DRAW_SKIP             },
    {   "jersey_barrier",              DRAW_SKIP             },
    {   "bollard",                     DRAW_SKIP             },
    {   "handrail",                    DRAW_SKIP             },
    {   "kerb",                        DRAW_SKIP             },
    {   "lift_gate",                   DRAW_SKIP             },
    {   "guard_rail",                  DRAW_SKIP             },
    {   "block",                       DRAW_SKIP             },
    {   "entrance",                    DRAW_SKIP             },
    {   "banister",                    DRAW_SKIP             },
    {   "glass",                       DRAW_SKIP             },
    {   "chain",                       DRAW_SKIP             },
    {   "ditch",                       DRAW_SKIP             },
    {   "cable_barrier",               DRAW_SKIP             },
    {   "flood_wall",                  DRAW_SKIP             },
    {   "block_row",                   DRAW_SKIP             },
    {   "flowerpot",                   DRAW_SKIP             },
    {   "wicket_gate",                 DRAW_SKIP             },
    {   "rope",                        DRAW_SKIP             },
    {   "yes",                         DRAW_SKIP             },
};

static const osm_mapper_t map_path_power[] = {
    {   "power",                       DRAW_UNKNOWN          },

    {   "line",                        DRAW_SKIP             },
    {   "generator",                   DRAW_SKIP             },
    {   "cable",                       DRAW_SKIP             },
    {   "portal",                      DRAW_SKIP             },
    {   "plant",                       DRAW_SKIP             },
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
    {   "dam",                         DRAW_SKIP              },
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

static const osm_mapper_t map_area_place[] = {

    {   "place",                      DRAW_UNKNOWN           },

    {   "square",                     DRAW_AREA_ASPHALT      },
    {   "islet",                      DRAW_AREA_ASPHALT      },
    {   "locality",                   DRAW_AREA_ASPHALT      },
    {   "farm",                       DRAW_AREA_GRASS        },
    {   "island",                     DRAW_AREA_GRASS        },
    {   "plot",                       DRAW_SKIP              },
    {   "city_block",                 DRAW_SKIP              },
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

static const osm_mapper_t map_relation_route[] = {

    {   "type",                        DRAW_UNKNOWN           },

    {   "route",                       DRAW_SKIP              },
    {   "superroute",                  DRAW_SKIP              },
    {   "public_transport",            DRAW_SKIP              },
    {   "rf",                          DRAW_SKIP              },
};

static const osm_mapper_t map_relation_boundary[] = {

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
    {   "shop",                        DRAW_SKIP              },
    {   "indoor",                      DRAW_SKIP              },
    {   "was",                         DRAW_SKIP              },
    {   "historic",                    DRAW_SKIP              },
    {   "playground",                  DRAW_SKIP              },
    {   "removed",                     DRAW_SKIP              },
    {   "toilets",                     DRAW_SKIP              },
    {   "roof",                        DRAW_SKIP              },
    {   "voltage",                     DRAW_SKIP              },
    {   "eea",                         DRAW_SKIP              },
    {   "disc_golf",                   DRAW_SKIP              },
};

//---------------------------------------------------------------------------//

osm_processor_t::osm_processor_t() {

    m_callback_node = nullptr;
    m_callback_way  = nullptr;
    m_callback_rel  = nullptr;

    memset(&xml_ctx, 0, sizeof(xml_ctx));
    xml_ctx_cnt = 0;

    load_skiplist("skip.nodes", skiplist_nodes);
    load_skiplist("skip.ways",  skiplist_ways );
    load_skiplist("skip.rels",  skiplist_rels );
}

//---------------------------------------------------------------------------//

void osm_processor_t::log_node ( const char* const name, osm_id_t id, const osm_tag_ctx_t& node_info ) {

    return;

    std::cout << "<" << name << " id = \"" << id << "\">" << std::endl;

    for (int i = 0; i < node_info.cnt; i++) {
        std::cout << "  <tag ";
        std::cout << "k=\"" << node_info.list[i].k << "\" ";
        std::cout << "v=\"" << node_info.list[i].v << "\" ";
        std::cout << "/>" << std::endl;
    }

    std::cout << "</way>" << std::endl;
}

void osm_processor_t::load_skiplist ( const char* const file_name, ssearcher& bor ) {

    std::ifstream cfg_file;
    std::string   line;
    std::string   key;

    cfg_file.open(file_name);

    while (std::getline(cfg_file, line)) {
        if (!line.empty()) {
            bor.add(line, 0);
        }
    }
}

void osm_processor_t::bor_init ( const osm_mapper_t* const lex_list, size_t cnt, ssearcher& bor ) {

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

void osm_processor_t::map_ref(const hpx_attr_t* attr, ref_type_t& ref) {

    ref = REF_UNKNOWN;

    if ( bs_cmp(attr->value, "node") == 0 ) {
        ref = REF_NODE;
    } else
    if ( bs_cmp(attr->value, "way") == 0 ) {
        ref = REF_WAY;
    } else
    if ( bs_cmp(attr->value, "relation") == 0 ) {
        ref = REF_RELATION;
    } else {
        assert(false);
    }
}

void osm_processor_t::map_role(const hpx_attr_t* attr, ref_role_t& role) {

    role = ROLE_UNKNOWN;

    if ( bs_cmp(attr->value, "outer") == 0 ) {
        role = ROLE_OUTER;
    } else
    if ( bs_cmp(attr->value, "inner") == 0 ) {
        role = ROLE_INNER;
    }
}

//---------------------------------------------------------------------------//

void osm_processor_t::osm_push ( osm_node_t next_node ) {

    xml_ctx[xml_ctx_cnt] = next_node;
    xml_ctx_cnt++;
}

void osm_processor_t::osm_pop ( osm_node_t osm_node ) {

    xml_ctx_cnt--;

    if (xml_ctx[xml_ctx_cnt] == XML_NODE_NODE ) {
        node_expand_tags();
        node_resolve_type();
        node_store_info();
        clean_info();
    } else
    if (xml_ctx[xml_ctx_cnt] == XML_NODE_WAY ) {
        ways_expand_tags();
        ways_resolve_type();
        ways_store_info();
        clean_info();
    } else
    if (xml_ctx[xml_ctx_cnt] == XML_NODE_REL ) {
        rel_expand_tags();
        rel_resolve_type();
        rel_store_info();
        clean_info();
    }

    xml_ctx[xml_ctx_cnt] = XML_NODE_UNDEF;
}

void osm_processor_t::cp_val ( const hpx_attr_t& src, alloc_str_t& dst ) {

    strncpy_s(dst, src.value.buf, src.value.len);
    dst[src.value.len] = 0;
}

void osm_processor_t::process_root_node ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {

        if (bs_cmp(attr_list[i].name, "id") == 0) {
            osm_info.node_info.id = bs_tol(attr_list[i].value );
        } else
        if (bs_cmp(attr_list[i].name, "lat") == 0) {
            osm_info.node_info.lat = bs_tod(attr_list[i].value);
        } else
        if (bs_cmp(attr_list[i].name, "lon") == 0) {
            osm_info.node_info.lon = bs_tod(attr_list[i].value);
        }

    }
}

void osm_processor_t::store_attr ( int attr_cnt, const hpx_attr_t* new_item ) {

    int s = osm_info.xml_tags.cnt;
    osm_info.xml_tags.cnt++;

    assert ( osm_info.xml_tags.cnt < OSM_MAX_TAGS_CNT );

    for (int i = 0; i < attr_cnt; i++) {
        if ( bs_cmp(new_item[i].name, "k") == 0 ) {
            cp_val(new_item[i], osm_info.xml_tags.list[s].k);
        } else
        if (bs_cmp(new_item[i].name, "v") == 0) {
            cp_val(new_item[i], osm_info.xml_tags.list[s].v);
        }
    }
}

void osm_processor_t::store_ref ( const bstring_t& value, ref_type_t ref_type ) {

    ref_way_t next_item;

    next_item.id = bs_tol(value);
    next_item.ref = ref_type;

    osm_info.refs.push_back(next_item);
}

void osm_processor_t::process_root_way ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "id") == 0) {
            osm_info.node_info.id = bs_tol(attr_list[i].value);
            break;
        }
    }
}

void osm_processor_t::process_way_nd ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "ref") == 0) {
            store_ref(attr_list[i].value, REF_NODE);
            break;
        }
    }
}

void osm_processor_t::process_node_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

    int  type;
    bool skip;

    skip = skiplist_nodes.find(attr_list->value, type);
    if (skip) {
        return;
    }

    store_attr(attr_cnt, attr_list);
}

void osm_processor_t::process_way_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

    int  type;
    bool skip;

    skip = skiplist_ways.find(attr_list->value, type);
    if (skip) {
        return;
    }

    store_attr(attr_cnt, attr_list);
}

void osm_processor_t::process_root_rel ( int attr_cnt, const hpx_attr_t* attr_list ) {

    for (int i = 0; i < attr_cnt; i++) {
        if (bs_cmp(attr_list[i].name, "id") == 0) {
            osm_info.node_info.id = bs_tol(attr_list[i].value);
            break;
        }
    }
}

void osm_processor_t::process_rel_member ( int attr_cnt, const hpx_attr_t* attr_list ) {

    ref_way_t way;

    way.id   = 0;
    way.ref  = REF_UNKNOWN;
    way.role = ROLE_UNKNOWN;

    for (int i = 0; i < attr_cnt; i++) {

        if (bs_cmp(attr_list[i].name, "ref") == 0) {
            way.id = bs_tol(attr_list[i].value);
        } else
        if (bs_cmp(attr_list[i].name, "type") == 0) {
            map_ref ( &attr_list[i], way.ref );
        } else
        if (bs_cmp(attr_list[i].name, "role") == 0) {
            map_role ( &attr_list[i], way.role );
        } 

    }

    osm_info.refs.push_back(way);
}

void osm_processor_t::process_rel_tag ( int attr_cnt, const hpx_attr_t* attr_list ) {

    int  type;
    bool skip;

    skip = skiplist_rels.find(attr_list->value, type);
    if (skip) {
        return;
    }

    store_attr(attr_cnt, attr_list);
}

void osm_processor_t::process_open ( int attr_cnt, const hpx_attr_t* attr_list ) {

    switch ( MAP_TYPE(OSM_TYPE_PREV, OSM_TYPE_CURR) ) {


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_NODE):
            process_root_node(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_NODE, XML_NODE_TAG):
            process_node_tag(attr_cnt, attr_list);
            break;


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_WAY):
            process_root_way(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_WAY, XML_NODE_ND):
            process_way_nd(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_WAY, XML_NODE_TAG):
            process_way_tag(attr_cnt, attr_list);
            break;


        case MAP_TYPE(XML_NODE_ROOT, XML_NODE_REL):
            process_root_rel(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_REL, XML_NODE_MEMBER):
            process_rel_member(attr_cnt, attr_list);
            break;
        case MAP_TYPE(XML_NODE_REL, XML_NODE_TAG):
            process_rel_tag(attr_cnt, attr_list);
            break;


        default:
            assert(false);
    }

}

void osm_processor_t::process_item ( int xml_type, const bstring_t& name, int attr_cnt, const hpx_attr_t* attr_list ) {

    osm_node_t osm_node = XML_NODE_UNDEF;

    if ( bs_cmp(name, "node") == 0 ) {
        osm_node = XML_NODE_NODE;
    } else
    if ( bs_cmp(name, "tag") == 0 ) {
        osm_node = XML_NODE_TAG;
    } else
    if ( bs_cmp(name, "way") == 0 ) {
        osm_node = XML_NODE_WAY;
    } else
    if ( bs_cmp(name, "relation") == 0 ) {
        osm_node = XML_NODE_REL;
    } else
    if ( bs_cmp(name, "member") == 0 ) {
        osm_node = XML_NODE_MEMBER;
    } else
    if ( bs_cmp(name, "nd") == 0 ) {
        osm_node = XML_NODE_ND;
    } else
    if ( bs_cmp(name, "osm") == 0 ) {
        osm_node = XML_NODE_SKIP;
    } else
    if ( bs_cmp(name, "bounds") == 0 ) {
        osm_node = XML_NODE_SKIP;
    } else
    if ( bs_cmp(name, "xml") == 0 ) {
        osm_node = XML_NODE_SKIP;
    }

    assert(osm_node != XML_NODE_UNDEF);

    if (osm_node == XML_NODE_SKIP) {
        return;
    }

    switch (xml_type) {

        case HPX_OPEN: // open tag
            osm_push(osm_node);
            process_open(attr_cnt, attr_list);
            break;

        case HPX_SINGLE: // open tag + close tag
            osm_push(osm_node);
            process_open(attr_cnt, attr_list);
            osm_pop(osm_node);
            break;

        case HPX_CLOSE:
            osm_pop(osm_node);
            break;

        case HPX_INSTR:
        case HPX_COMMENT:
            break;

        case HPX_ILL:
        case HPX_LITERAL:
        case HPX_ATT:
            assert(false);
            break;
    }

}

void osm_processor_t::key_cmp ( const char* const key, const char* const val, bool& r1, bool& r2 ) {

    size_t i = 0;

    for (i = 0; i < OSM_STR_MAX_LEN - 1; i++) {
        if (key[i] == 0) {
            break;
        }
        if (key[i] != val[i]) {
            return;
        }
    }

    if (val[i] == 0) {
        r1 = true;
    }

    if (val[i] == ':') {
        r2 = true;
    }
}

void osm_processor_t::test_and_add ( const char* const key ) {

    int  i;
    bool key_defined = false;
    bool ns_defined = false;

    for (i = 0; i < osm_info.xml_tags.cnt; i++) {
        key_cmp(key, osm_info.xml_tags.list[i].k, key_defined, ns_defined);
    }

    if (key_defined) {
        // Key exists.
        return;
    }

    if (!ns_defined) {
        // namespese doesn't exists
        return;
    }

    int s = osm_info.xml_tags.cnt;
    osm_info.xml_tags.cnt++;

    assert(osm_info.xml_tags.cnt < OSM_MAX_TAGS_CNT);

    strncpy_s(osm_info.xml_tags.list[s].k, key, OSM_STR_MAX_LEN - 1);
    strncpy_s(osm_info.xml_tags.list[s].v, "auto", OSM_STR_MAX_LEN - 1);
}

bool osm_processor_t::find_key ( const osm_tag_ctx_t& node_info, osm_str_t k, osm_str_t v1, osm_str_t v2, osm_str_t v3 ) {

    size_t pos = 0;

    for (pos = 0; pos < node_info.cnt; pos++) {
        if (strcmp(node_info.list[pos].k, k) == 0) {
            break;
        }
    }

    if (pos == node_info.cnt) {
        return false;
    }

    if ((v1 == nullptr) && (v2 == nullptr) && (v3 == nullptr)) {
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

void osm_processor_t::map_type ( osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info, const ssearcher& bor ) {

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

bool osm_processor_t::is_area ( const osm_tag_ctx_t& xml_tags ) {

    bool find_res;

    find_res = find_key(xml_tags, "area");
    if (!find_res) {
        return false;
    }

    find_res = find_key(xml_tags, "area", "false", "0", "no");
    if (find_res) {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------//

void osm_processor_t::nodes_init ( void ) {

}

void osm_processor_t::node_expand_tags ( void ) {

}

void osm_processor_t::node_resolve_type ( void ) {

    osm_info.node_info.type = DRAW_UNKNOWN;
}

void osm_processor_t::node_store_info ( void ) {

    if (m_callback_node != nullptr) {
        m_callback_node(osm_info);
    }
}

void osm_processor_t::clean_info( void ) {

    assert(osm_info.xml_tags.cnt < OSM_MAX_TAGS_CNT);

    while (osm_info.xml_tags.cnt > 0) {
        osm_info.xml_tags.cnt--;
        osm_info.xml_tags.list[osm_info.xml_tags.cnt].k[0] = 0;
        osm_info.xml_tags.list[osm_info.xml_tags.cnt].v[0] = 0;
    }

    memset(&osm_info.node_info, 0, sizeof(osm_info.node_info));
    osm_info.refs.clear();
}

//---------------------------------------------------------------------------//

void osm_processor_t::ways_init ( void ) {

    bor_init ( map_ways_unused,       ITEMS_CNT(map_ways_unused),     ways_ignored );
               
    bor_init ( map_area_landuse,      ITEMS_CNT(map_area_landuse),    area_landuse );
    bor_init ( map_area_leisure,      ITEMS_CNT(map_area_leisure),    area_leisure );
    bor_init ( map_area_natural,      ITEMS_CNT(map_area_natural),    area_natural );
    bor_init ( map_area_amenity,      ITEMS_CNT(map_area_amenity),    area_amenity );
    bor_init ( map_area_waterway,     ITEMS_CNT(map_area_waterway),   area_waterway );
    bor_init ( map_area_manmade,      ITEMS_CNT(map_area_manmade),    area_manmade );
    bor_init ( map_area_highway,      ITEMS_CNT(map_area_highway),    area_highway );
    bor_init ( map_area_transport,    ITEMS_CNT(map_area_transport),  area_transport );
    bor_init ( map_area_water,        ITEMS_CNT(map_area_water),      area_water );
    bor_init ( map_area_railway,      ITEMS_CNT(map_area_railway),    area_railway );
    bor_init ( map_area_boundary,     ITEMS_CNT(map_area_boundary),   area_boundary );
    bor_init ( map_area_place,        ITEMS_CNT(map_area_place),      area_place );
               
    bor_init ( map_path_highway,      ITEMS_CNT(map_path_highway),    path_highway );
    bor_init ( map_path_railway,      ITEMS_CNT(map_path_railway),    path_railway );
    bor_init ( map_path_waterway,     ITEMS_CNT(map_path_waterway),   path_waterway );
    bor_init ( map_path_natural,      ITEMS_CNT(map_path_natural),    path_natural );
    bor_init ( map_path_transport,    ITEMS_CNT(map_path_transport),  path_transport );
    bor_init ( map_path_power,        ITEMS_CNT(map_path_power),      path_power );
    bor_init ( map_path_bridge,       ITEMS_CNT(map_path_bridge),     path_bridge );
    bor_init ( map_path_stairwell,    ITEMS_CNT(map_path_stairwell),  path_stairwell );
    bor_init ( map_path_barrier,      ITEMS_CNT(map_path_barrier),    path_barrier );
    bor_init ( map_path_golf,         ITEMS_CNT(map_path_golf),       path_golf );
    bor_init ( map_path_shop,         ITEMS_CNT(map_path_shop),       path_shop );

    bor_init ( map_ways_unused,       ITEMS_CNT(map_ways_unused),     ways_ignored );
}

void osm_processor_t::ways_expand_tags ( void ) {

    test_and_add("disused");
    test_and_add("bridge");
    test_and_add("building");
    test_and_add("abandoned");
    test_and_add("area");
    test_and_add("proposed");
    test_and_add("construction");
    test_and_add("cycleway");
    test_and_add("demolished");
    test_and_add("was");
    test_and_add("removed");
    test_and_add("toilets");
    test_and_add("roof");
    test_and_add("addr");
    test_and_add("eea");
}

void osm_processor_t::ways_resolve_type ( void ) {

    osm_draw_type_t draw_type = DRAW_UNKNOWN;

    if ( osm_info.xml_tags.cnt == 0 ) {
        draw_type = DRAW_PENDING;
    }

    process_osm_param(draw_type, DRAW_SKIP, "abandoned");
    process_osm_param(draw_type, DRAW_SKIP, "disused");
    process_osm_param(draw_type, DRAW_SKIP, "construction");
    process_osm_param(draw_type, DRAW_SKIP, "proposed");
    process_osm_param(draw_type, DRAW_SKIP, "demolished");

    process_building(draw_type, osm_info.xml_tags);

    if (is_area(osm_info.xml_tags)) {

        map_type(draw_type, osm_info.xml_tags, area_highway);
        map_type(draw_type, osm_info.xml_tags, area_transport);
        map_type(draw_type, osm_info.xml_tags, area_landuse);
        map_type(draw_type, osm_info.xml_tags, area_leisure);
        map_type(draw_type, osm_info.xml_tags, area_natural);
        map_type(draw_type, osm_info.xml_tags, area_amenity);
        map_type(draw_type, osm_info.xml_tags, area_waterway);
        map_type(draw_type, osm_info.xml_tags, area_manmade);
        map_type(draw_type, osm_info.xml_tags, area_water);
        map_type(draw_type, osm_info.xml_tags, area_railway);
        map_type(draw_type, osm_info.xml_tags, area_boundary);

        if (draw_type == DRAW_UNKNOWN) {
            draw_type = DRAW_AREA_UNKNOWN;
        }

    }
    else {

        map_type(draw_type, osm_info.xml_tags, path_highway);
        map_type(draw_type, osm_info.xml_tags, path_railway);
        map_type(draw_type, osm_info.xml_tags, path_waterway);
        map_type(draw_type, osm_info.xml_tags, path_natural);
        map_type(draw_type, osm_info.xml_tags, path_transport);
        map_type(draw_type, osm_info.xml_tags, path_power);
        map_type(draw_type, osm_info.xml_tags, path_bridge);
        map_type(draw_type, osm_info.xml_tags, path_stairwell);
        map_type(draw_type, osm_info.xml_tags, path_barrier);
        map_type(draw_type, osm_info.xml_tags, path_golf);
        map_type(draw_type, osm_info.xml_tags, path_shop);

        map_type(draw_type, osm_info.xml_tags, area_amenity);
        map_type(draw_type, osm_info.xml_tags, area_water);
        map_type(draw_type, osm_info.xml_tags, area_waterway);
        map_type(draw_type, osm_info.xml_tags, area_leisure);
        map_type(draw_type, osm_info.xml_tags, area_natural);
        map_type(draw_type, osm_info.xml_tags, area_landuse);
        map_type(draw_type, osm_info.xml_tags, area_manmade);
        map_type(draw_type, osm_info.xml_tags, area_place);
    }

    process_unused ( draw_type, osm_info.xml_tags, ways_ignored );

    if (draw_type == DRAW_UNKNOWN) {
        log_node( "node", osm_info.node_info.id, osm_info.xml_tags);
    }

    osm_info.node_info.type = draw_type;
}

void osm_processor_t::ways_store_info ( void ) {

    if (m_callback_way != nullptr) {
        m_callback_way(osm_info);
    }
}

//---------------------------------------------------------------------------//

void osm_processor_t::rels_init ( void ) {

    bor_init ( map_relation_route,    ITEMS_CNT(map_relation_route),    rels_types );
    bor_init ( map_relation_boundary, ITEMS_CNT(map_relation_boundary), rels_boundary );
}

void osm_processor_t::rel_expand_tags ( void ) {

    // test_and_add("disused");
}

void osm_processor_t::rel_resolve_type ( void ) {

    osm_draw_type_t draw_type = DRAW_UNKNOWN;

    process_building ( draw_type, osm_info.xml_tags );
    map_type ( draw_type, osm_info.xml_tags, rels_types );
    map_type ( draw_type, osm_info.xml_tags, rels_boundary );

    if ( draw_type == DRAW_UNKNOWN ) {
        log_node( "relation", osm_info.node_info.id, osm_info.xml_tags );
    }

    osm_info.node_info.type = draw_type;
}

void osm_processor_t::rel_store_info ( void ) {

    if (m_callback_rel != nullptr) {
        m_callback_rel(osm_info);
    }
}

//---------------------------------------------------------------------------//

void osm_processor_t::process_osm_param ( osm_draw_type_t& draw_type, osm_draw_type_t new_type, const char* const name ) {

    bool find_res;

    if (draw_type != DRAW_UNKNOWN) {
        return;
    }

    find_res = find_key(osm_info.xml_tags, name);
    if (find_res) {
        draw_type = new_type;
    }

}

void osm_processor_t::process_building ( osm_draw_type_t& draw_type, const osm_tag_ctx_t& node_info ) {

    bool find_res;

    if (draw_type != DRAW_UNKNOWN) {
        return;
    }

    find_res = find_key(node_info, "building");
    if (!find_res) {
        return;
    }

    find_res = find_key(node_info, "building", "false", "0", "no");
    if (find_res) {
        return;
    }

    draw_type = DRAW_BUILDING;
}

void osm_processor_t::process_unused ( osm_draw_type_t& draw_type, const osm_tag_ctx_t& xml_tags, const ssearcher& bor ) {

    bool find_res;
    int  type;

    if (draw_type != DRAW_UNKNOWN) {
        return;
    }

    for (int i = 0; i < xml_tags.cnt; i++) {
        find_res = bor.find(xml_tags.list[i].k, type);
        if (find_res) {
            draw_type = DRAW_SKIP;
            break;
        }
    }

}

//---------------------------------------------------------------------------//

void osm_processor_t::configure ( osm_object_t add_node, osm_object_t add_way, osm_object_t add_rel ) {

    m_callback_node = add_node;
    m_callback_way  = add_way;
    m_callback_rel  = add_rel;
}

bool osm_processor_t::process_file ( const char* const file_name ) {

    int         fd      = 0;
    hpx_ctrl_t* ctrl    = nullptr;
    hpx_tag_t*  xml_obj = nullptr;
    bstring_t   xml_str = { 0 };
    long        lno     = 0;
    int         io_res  = 0;

    _sopen_s(&fd, file_name, _O_BINARY | _O_RDONLY, _SH_DENYWR, _S_IREAD);
    if (fd == -1) {
        return false;
    }

    ctrl = hpx_init(fd, 100 * 1024 * 1024);
    if (ctrl == NULL) {
        return false;
    }

    xml_obj = hpx_tm_create(64);
    if (xml_obj == NULL) {
        return false;
    }

    nodes_init();
    ways_init();
    rels_init();

    xml_ctx[0] = XML_NODE_ROOT;
    xml_ctx_cnt++;

    for ( ; ; ) {

        io_res = hpx_get_elem(ctrl, &xml_str, NULL, &lno);
        if (io_res == 0) {
            break;
        }

        io_res = hpx_process_elem(xml_str, xml_obj);
        if (io_res != 0) {
            printf("[%ld] ERROR in element: %.*s\n", lno, xml_str.len, xml_str.buf);
            break;
        }

        process_item(xml_obj->type, xml_obj->tag, xml_obj->nattr, xml_obj->attr);
    }

    if ( !ctrl->eof ) {
        assert(false);
        return false;
    }

    hpx_tm_free(xml_obj);
    hpx_free(ctrl);

    _close(fd);

    return true;
}
