#include <string.h>

#include "LexItem.h"

bool geo_lex_t::LexCmp ( const char* const key_in, const char* const key_exp, const char* const val_in, const char* const val_exp ) {

    if (strcmp(key_in, key_exp) != 0) {
        return false;
    }

    if ((val_in != nullptr) && (val_exp != nullptr)) {
        if (strcmp(val_in, val_exp) != 0) {
            return false;
        }
    }

    return true;
}

