#ifndef __SSEARCH_H__
#define __SSEARCH_H__

#include <stdint.h>
#include <string>

constexpr int search_width = 256;


class search_item {

public:
    search_item() {
        flag_stop = false;
        pat_num = -1;
        for (size_t i = 0; i < search_width; i++) {
            to[i] = -1;
        }
    }

public:
    int32_t  pat_num = -1;
    int32_t  to[search_width] = { 0 };
    int32_t  flag_stop = false;
};


class ssearcher {

    public:
        ssearcher() {};

    public:
        void init ( const char* const name );
        void add  ( const std::string& var, int  type );
        bool find ( const std::string& var, int& type ) const;

    public:
        std::string                 m_key_name;
        std::vector<search_item>    m_bor;

};

#endif

