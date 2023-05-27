#ifndef __GEO_IDX_H__
#define __GEO_IDX_H__

#include <string>
#include <algorithm>

#include <geo/geo_idx.h>
#include <geo/geo_obj.h>
#include <geo/geo_parser.h>

class geo_idx_record_t {

    public:
        void clear ( void ) {
            m_rect.clear();
            m_memembers.clear();
        }

        void reset ( void ) {
            m_memembers.clear();
        }

        void idx_add ( const std::string& val ) {
            uint32_t offset;
            (void)sscanf_s(val.c_str(), "%d", &offset);
            m_memembers.push_back(offset);
        }

    public:
        geo_rect_t          m_rect;
        vector_geo_off_t    m_memembers;
};

typedef std::vector<geo_idx_record_t>    vector_geo_idx_record_t;

class geo_indexer_t {

    public:
        void set_idx_name ( const char* const name ) {
            m_idx_file = name;
        }

        void set_map_name ( const char* const name ) {
            m_map_file = name;
        }

        void load_idx_file ( void ) {

            std::ifstream    idx_file;
            geo_idx_rec_t    idx_obj;

            bool    eor;
            bool    eoc;
            char    ch;

            idx_file.open ( m_idx_file );

            while ( idx_file.read(&ch, 1) ) {
                m_geo_parser.load_param ( ch, eor, 0 );
                if ( eor ) {
                    m_geo_parser.process_idx ( idx_obj, eoc );
                    if ( eoc ) {
                        m_idx_list.push_back (idx_obj);
                    }
                }
            }

            return;
        }

        void cache_region ( const geo_rect_t& rect ) {

            size_t idx_cnt = 0;
            set_geo_off_t offs_list;

            auto it_cache = m_idx_list.cbegin();
            while (it_cache != m_idx_list.cend()) {
                if ( is_verlapped(rect, it_cache->m_rect ) ) {
                    idx_cnt += it_cache->m_list_off.size();
                    Attach ( offs_list, it_cache->m_list_off );
                }
                it_cache++;
            }

            m_geo_cache.clear();
            m_geo_cache.resize ( offs_list.size() );
            std::copy ( offs_list.begin(), offs_list.end(), m_geo_cache.begin() );
            std::sort ( m_geo_cache.begin(), m_geo_cache.end() );
        }

        private:
            bool is_verlapped ( const geo_rect_t& window, const geo_rect_t& slice ) {
                if (window.min_lon < slice.max_lon) {
                    return false;
                }
                if (window.max_lon < slice.min_lon) {
                    return false;
                }
                if (window.max_lat < slice.min_lat) {
                    return false;
                }
                if (window.min_lat < slice.max_lat) {
                    return false;
                }
                return true;
            }

            void Attach ( set_geo_off_t& offsets, const vector_geo_off_t& refs ) {
                auto it = refs.cbegin();
                while (it != refs.cend()) {
                    offsets.insert(*it);
                    it++;
                }
            }


    private:
        std::string             m_idx_file;
        std::string             m_map_file;

    private:
        geo_parser_t            m_geo_parser;
        vector_geo_idx_rec_t    m_idx_list;
        vector_geo_off_t        m_geo_cache;
};


#endif
