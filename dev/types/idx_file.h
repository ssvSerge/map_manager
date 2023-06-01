#ifndef __IDX_FILE_H__
#define __IDX_FILE_H__

#include <string>
#include <fstream>

#include <geo_types.h>

class geo_idx_record_t {

    public:
        void clear ( void ) {
            m_rect.clear();
            m_memembers.clear();
        }

        void idx_member ( const std::string& val ) {
            geo_offset_t offset;
            (void)sscanf_s(val.c_str(), "%d", &offset);
            m_memembers.push_back(offset);
        }

    public:
        vector_geo_path_t       m_rect;
        vector_geo_offset_t     m_memembers;
};

#endif


