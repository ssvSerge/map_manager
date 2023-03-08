#include <cassert>

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

    {   "rail",                        DRAW_PATH_RAILWAY     },
    {   "disused",                     DRAW_SKIP             },
    {   "abandoned",                   DRAW_SKIP             },
    {   "razed",                       DRAW_SKIP             },
    {   "tram",                        DRAW_SKIP             },
};


static const osm_mapper_t map_path_waterway[] = {

    {   "waterway",                    DRAW_UNKNOWN           },

    {   "drain",                       DRAW_PATH_RIVER        },
    {   "stream",                      DRAW_PATH_RIVER        },
    {   "ditch",                       DRAW_PATH_RIVER        },
    {   "river",                       DRAW_PATH_RIVER        },
    {   "canal",                       DRAW_PATH_RIVER        },
    {   "weir",                        DRAW_PATH_RIVER        },
};

static const osm_mapper_t map_path_natural[] = {

    {   "natural",                     DRAW_UNKNOWN           },
    {   "tree_row",                    DRAW_PATH_FOOTWAY       },
};

static const osm_mapper_t map_path_transport[] = {

    {   "public_transport",            DRAW_UNKNOWN           },
    {   "platform",                    DRAW_SKIP              },
    {   "station",                     DRAW_SKIP              },
};

static const osm_mapper_t map_area_landuse[] = {

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

static const osm_mapper_t map_area_leisure[] = {

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

static const osm_mapper_t map_area_natural[] = {

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

static const osm_mapper_t map_area_amenity[] = {

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
};

static const osm_mapper_t map_area_manmade[] = {

    {   "manmade",                     DRAW_UNKNOWN           },

    {   "platform",                    DRAW_AREA_ASPHALT      },
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
}

void ways_resolve_type ( void ) {

    osm_draw_type_t draw_type = DRAW_UNKNOWN;

    if ( g_xml_tags.cnt == 0 ) {
        draw_type = DRAW_PENDING;
        return;
    }

    process_osm_param ( draw_type, DRAW_SKIP, "disused" );
    process_osm_param ( draw_type, DRAW_SKIP, "construction" );
    process_osm_param ( draw_type, DRAW_SKIP, "proposed" );

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
    } else {
    
        map_type ( draw_type, g_xml_tags, g_path_highway );
        map_type ( draw_type, g_xml_tags, g_path_railway );
        map_type ( draw_type, g_xml_tags, g_path_waterway );
        map_type ( draw_type, g_xml_tags, g_path_natural );
        map_type ( draw_type, g_xml_tags, g_path_transport );
        map_type ( draw_type, g_xml_tags, g_path_power );

    }

    _process_unused ( draw_type, g_xml_tags, g_ways_ignored );
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
