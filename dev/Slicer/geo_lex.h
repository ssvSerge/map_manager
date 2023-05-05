#ifndef __GEOLEX_H__
#define __GEOLEX_H__

#define    MSG_LEN     (128)

typedef struct tag_lex {
    char        msg[MSG_LEN];
    int         pos;
    size_t      off;
}   lex_t;



#endif

