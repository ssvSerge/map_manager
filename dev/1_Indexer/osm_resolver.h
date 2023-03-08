#ifndef __OSM_RESOLVER_H__
#define __OSM_RESOLVER_H__

void expand_node_tags  (void);
void resolve_node_type (void);
void store_node_info   (void);
void clean_node_info   (void);

void ways_init         (void);
void ways_expand_tags  (void);
void ways_resolve_type (void);
void ways_store_info   (void);
void ways_clean_info   (void);

void expand_rel_tags   (void);
void resolve_rel_type  (void);
void store_rel_info    (void);
void clean_rel_info    (void);

#endif

