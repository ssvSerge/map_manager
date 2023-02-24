#include <cassert>

#include "..\common\osm_types.h"


void _clean_ctx ( osm_tag_ctx_t& ctx ) {

    assert(ctx.pos_1 < OSM_MAX_TAGS_CNT);

    while ( ctx.pos_1 > 0 ) {
        ctx.pos_1--;
        ctx.list[ctx.pos_1].k[0] = 0;
        ctx.list[ctx.pos_1].v[0] = 0;
    }
}

