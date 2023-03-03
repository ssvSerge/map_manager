#include <cassert>

#include "..\common\osm_types.h"

void _clean_ctx ( osm_tag_ctx_t& ctx ) {

    assert(ctx.cnt < OSM_MAX_TAGS_CNT);

    while ( ctx.cnt > 0 ) {
        ctx.cnt--;
        ctx.list[ctx.cnt].k[0] = 0;
        ctx.list[ctx.cnt].v[0] = 0;
    }
}

