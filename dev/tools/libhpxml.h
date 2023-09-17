#ifndef HPXML_H
#define HPXML_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bstring.h"


#define IS_XML1CHAR(x) (isalpha(x) || (x == '_') || (x == ':'))
#define IS_XMLCHAR(x)  (isalpha(x) || isdigit(x) || (x == '.') || (x == '-') || (x == '_') || (x == ':'))

#define hpx_init_simple() hpx_init(0, 10*1024*1024)

#define MMAP_PAGES (1L << 15)


typedef struct hpx_ctrl {
    //! data buffer containing pointer and number of bytes in buffer
    //bstring_t buf;
    struct bstringl buf;
    //! file descriptor of input file
    int fd;
    //! flag set if eof
    short eof;
    //! total length of buffer
    long len;
    //! current working position
    long pos;
    //! flag to deter if next element is in or out of tag
    int in_tag;
    //! flag set if data should be read from file
    short empty;
    //! flag set if data is memory mapped
    short mmap;
    //! pointer to madvise()'d region (MADV_WILLNEED)
    char *madv_ptr;
    //! system page size
    long pg_siz;
    //! length of advised region (multiple of sysconf(_SC_PAGESIZE))
    long pg_blk_siz;
}   hpx_ctrl_t;

typedef struct hpx_attr {
    bstring_t name;   //! name of attribute
    bstring_t value;  //! value of attribute
    char delim;       //! delimiter character of attribute value
}   hpx_attr_t;

typedef struct hpx_tag  {
    bstring_t    tag;
    int          type;
    long         line;
    int          nattr;
    int          mattr;
    hpx_attr_t   attr[];
}   hpx_tag_t;

typedef struct hpx_tree {
    hpx_tag_t *tag;
    int nsub;
    int msub;
    struct hpx_tree *subtag[];
}   hpx_tree_t;

enum {
   HPX_ILL,       // 0   ???
   HPX_OPEN,      // 1   <osm> <node> <way> <relation>, ...
   HPX_SINGLE,    // 2   <osm/> <node ... />, ...
   HPX_CLOSE,     // 3   <node/>
   HPX_LITERAL,   // 4   text between nodes ???
   HPX_ATT,       // 5   ???
   HPX_INSTR,     // 6   <xml>
   HPX_COMMENT    // 7   comments
};


long        hpx_lineno          (void);
void        hpx_tm_free         (hpx_tag_t *t);
void        hpx_tm_free_tree    (hpx_tree_t *);
hpx_tag_t*  hpx_tm_create       (int n);
int         hpx_process_elem    (bstring_t b, hpx_tag_t *p);
hpx_ctrl_t* hpx_init            (int fd, long len);
void        hpx_init_membuf     (hpx_ctrl_t *ctl, void *buf, int len);
void        hpx_free            (hpx_ctrl_t *ctl);
int         hpx_get_elem        (hpx_ctrl_t *ctl, bstring_t*  b, int* in_tag, long* lno);
long        hpx_get_eleml       (hpx_ctrl_t *ctl, bstringl_t* b, int* in_tag, long* lno);
int         hpx_fprintf_tag     (FILE *f, const hpx_tag_t *p);
int         hpx_tree_resize     (hpx_tree_t **tl, int n);

#ifdef __cplusplus
    }
#endif

#endif

