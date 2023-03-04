#include <vector>

#include "ssearch.h"

void ssearcher::init ( const char* const name ) {

    m_key_name = name;
}

void ssearcher::add ( const std::string& var, int type ) {

    int num = 0;
    uint8_t ch;

    if (m_bor.size() == 0 ) {
        m_bor.emplace_back ( search_item() );
    }

    for ( size_t i = 0; i < var.size(); i++ ) {

        ch = var[i];

        if (m_bor[num].to[  ch  ] == -1) {
            m_bor.emplace_back ( search_item() );
            m_bor[num].to[  ch  ] = (int32_t) (m_bor.size() - 1);
        }

        num = m_bor[num].to[ ch ];
    }
    m_bor[num].flag_stop = true;
    m_bor[num].pat_num   = type;
}

bool ssearcher::find ( const std::string& var, int& type ) const {

    int     num = 0;
    uint8_t ch;

    if (m_bor.size() == 0) {
        return false;
    }

    for ( size_t i = 0; i < var.length(); i++ ) {

        ch = var[i];

        if (m_bor[num].to[ ch ] == -1) {
            return false;
        }
        num = m_bor[num].to[ ch ];
    }

    type = num = m_bor[num].pat_num;

    return true;
}

