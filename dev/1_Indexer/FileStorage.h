#ifndef __FILE_STORAGE_H__
#define __FILE_STORAGE_H__

#include <stdint.h>
#include <string>
#include "..\common\osm_idx.h"

class FileStorage {

    public:
        FileStorage ();

    public:
        void Create ( const char* const name );
        void View   ( const char* const name );
        void Close  ( void );

    public:
        void Store ( uint64_t id, const void* const data, int len );
        void Find  ( uint64_t id, const void* const data, int len );

    private:
        void create_names ( const char* const name );

    private:
        std::string     m_dat_file_name;
        std::string     m_idx_file_name;

    private:
        int             m_file_d = -1;
        int             m_file_i = -1;
        uint64_t        m_offset =  0;
        idx_t           m_idx    = { 0 };
};

#endif
