#ifndef __LEXITEM_H__
#define __LEXITEM_H__

class geo_lex_t {
    public:
        bool LexCmp ( const char* const key_in, const char* const key_exp, const char* const val_in = nullptr, const char* const val_exp = nullptr );
};

#endif
