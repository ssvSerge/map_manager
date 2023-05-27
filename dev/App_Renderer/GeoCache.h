#ifndef __GEOCACHE_H__
#define __GEOCACHE_H__

#if 0

#include <string>
#include <vector>
#include <list>
#include <set>
#include <fstream>

typedef uint32_t                   offset_t;

typedef std::vector<offset_t>      list_off_t;
typedef std::set<offset_t>         set_off_t;

class geo_rect_t {

    public:
        void clear() {
            min_lon = -333;
            min_lat = -333;
            max_lon = -333;
            max_lat = -333;
        }

        void load(const std::string& val) {
            (void)sscanf_s(val.c_str(), "%lf %lf %lf %lf", &min_lon, &min_lat, &max_lon, &max_lat);
        }

    public:
        double   min_lon;
        double   min_lat;
        double   max_lon;
        double   max_lat;
};

class geo_cache_t {

    public:
        void clear() {
            rect.clear();
            memembers.clear();
        }

        void read_rect(const std::string& val) {
            rect.load(val);
        }

        void reset() {
            memembers.clear();
        }

        void idx_add(const std::string& val) {
            uint32_t offset;
            (void)sscanf_s(val.c_str(), "%d", &offset);
            memembers.push_back(offset);
        }

    public:
        geo_rect_t		rect;
        list_off_t		memembers;
};

typedef std::list<geo_cache_t>              list_geo_idx_t;

typedef std::list<list_geo_idx_t>           list_list_geo_idx_t;

class geo_index_t {

    public:
        void SetIdxName ( const char* const name ) {
            m_idx_file = name;
        }

        void SetMapName ( const char* const name ) {
            m_map_file = name;
        }

        void LoadIdxFile ( void );
        void CacheRegion ( const geo_rect_t& rect );

    private:
        bool ReadLex ( std::string& in_data, std::string& key, std::string& val );
        void ProcessLex ( const std::string& key, std::string& val );
        bool LexCmp ( const char* const key_in, const char* const key_exp, const char* const val_in = nullptr, const char* const val_exp = nullptr );
        void SliceAdd ( void );
        bool IsOverlapped ( const geo_rect_t& window, const geo_rect_t& slice );
        void Attach ( set_off_t& offsets, const list_off_t& refs );
        void LoadIdx(const geo_rect_t& rect);
        void LoadObject(offset_t off, std::ifstream& map_file );

    private:
        std::string             m_idx_file;
        std::string             m_map_file;
        geo_cache_t             m_new_idx;

    private:
        list_geo_idx_t          m_geo_cache;
        list_off_t              m_offssets;
};

#endif

#endif
