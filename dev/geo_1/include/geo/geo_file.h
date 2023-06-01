#ifndef __GEO_FILE_H__
#define __GEO_FILE_H__

#include <string>
#include <fstream>

#include <geo/geo_parser.h>

class geo_file_t {
    public:
        void set_name( const char* const name ) {
            m_name = name;
            m_file.open(name);
        }

        void load_by_idx ( const vector_geo_off_t& cache, list_geo_objs_t& geo_map ) {

            geo_offset_t  pos;
            geo_obj_map_t geo_obj;

            geo_map.clear();

            for (size_t i = 0; i < cache.size(); i++) {
                pos = cache[i];
                read_map ( pos, geo_obj );
                geo_map.push_back(geo_obj);
            }
        }

        void trim_by_rect ( const geo_rect_t& rect, const list_geo_objs_t& in_map, list_geo_objs_t& out_map ) {
            
            geo_obj_map_t tmp;
            geo_set_t     in_rect;
            geo_set_t     in_area;
            geo_set_t     out_res;

            auto it_src = in_map.cbegin();

            while ( it_src != in_map.cend() ) {

                tmp = *it_src;

                auto geo_path_it = it_src->m_lines.begin();
                while (geo_path_it != it_src->m_lines.end()) {
                    in_area.push_back(geo_path_it->coords);
                    geo_path_it++;
                }

                out_res = Clipper2Lib::Intersect ( in_area, in_rect, Clipper2Lib::FillRule::NonZero, 13 );

                out_map.push_back(tmp);
                it_src++;
            }
        }

    private:
        void read_map ( geo_offset_t pos, geo_obj_map_t& geo_obj ) {
            
            char    ch;
            bool    eoc;
            bool    eor;

            geo_parser_t  parser;

            m_file.seekg(pos, std::ios::beg);

            while (m_file.read(&ch, 1)) {

                parser.load_param(ch, eoc, pos);
                pos++;

                if (!eoc) {
                    continue;
                }

                parser.process_map(geo_obj, eor);
                if (!eor) {
                    continue;
                }

                break;
            }

        }

    private:
        std::string    m_name;
        std::ifstream  m_file;
};

#endif

