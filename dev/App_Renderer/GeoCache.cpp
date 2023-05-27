#include <fstream>
#include <string>
#include <set>
#include <algorithm>

#if 0

#include "GeoCache.h"

bool geo_index_t::LexCmp ( const char* const key_in, const char* const key_exp, const char* const val_in, const char* const val_exp ) {

    if (strcmp(key_in, key_exp) != 0) {
        return false;
    }

    if ((val_in != nullptr) && (val_exp != nullptr)) {
        if (strcmp(val_in, val_exp) != 0) {
            return false;
        }
    }

    return true;
}

void geo_index_t::SliceAdd ( void ) {

    m_geo_cache.push_back(m_new_idx);
}

void geo_index_t::ProcessLex ( const std::string& key, std::string& val ) {

    if (LexCmp(key.c_str(), "IDX", val.c_str(), "RECT")) {
        m_new_idx.clear();
        return;
    }
    if (LexCmp(key.c_str(), "IDX", val.c_str(), "END")) {
        SliceAdd();
        return;
    }

    if (LexCmp(key.c_str(), "POSITION")) {
        m_new_idx.read_rect(val.c_str());
        return;
    }

    if (LexCmp(key.c_str(), "OFF", val.c_str(), "BEGIN")) {
        m_new_idx.reset();
        return;
    }

    if (LexCmp(key.c_str(), "OFF", val.c_str(), "END")) {
        SliceAdd();
        return;
    }

    if (LexCmp(key.c_str(), "M")) {
        m_new_idx.idx_add(val);
        return;
    }
}

bool geo_index_t::ReadLex ( std::string& in_data, std::string& key, std::string& val ) {

    int idx = 0;

    key.clear();
    val.clear();

    if (in_data.size() == 0) {
        return false;
    }

    for (idx = 0; idx < in_data.size(); idx++) {
        if (in_data[idx] == ':') {
            idx++;
            break;
        }
        key += in_data[idx];
    }

    for (; idx < in_data.size(); idx++) {
        if (in_data[idx] == ';') {
            idx++;
            break;
        }
        val += in_data[idx];
    }

    in_data.erase(0, idx);

    return true;
}

void geo_index_t::LoadIdxFile ( void ) {

    std::ifstream idx_file;
    std::string   data;
    std::string   key;
    std::string   val;

    idx_file.open(m_idx_file.c_str());

    while (std::getline(idx_file, data)) {
        while (ReadLex(data, key, val)) {
            ProcessLex(key, val);
        }
    }
}

void geo_index_t::Attach ( set_off_t& offsets, const list_off_t& refs ) {

    auto it = refs.cbegin();

    while (it != refs.cend()) {
        offsets.insert(*it);
        it++;
    }
}

bool geo_index_t::IsOverlapped ( const geo_rect_t& window, const geo_rect_t& slice ) {

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

void geo_index_t::LoadIdx( const geo_rect_t& rect ) {

    set_off_t offs_list;

    m_offssets.clear();

    auto it_cache = m_geo_cache.cbegin();
    while (it_cache != m_geo_cache.cend()) {
        if (IsOverlapped(rect, it_cache->rect)) {
            Attach(offs_list, it_cache->memembers);
        }
        it_cache++;
    }

    m_offssets.reserve(offs_list.size());

    auto it_off = offs_list.cbegin();
    while (it_off != offs_list.cend()) {
        m_offssets.push_back(*it_off);
        it_off++;
    }

    std::sort(m_offssets.begin(), m_offssets.end());
}

void geo_index_t::LoadObject(offset_t off, std::ifstream& map_file) {

    std::string   data;
    std::string   key;
    std::string   val;

    map_file.seekg ( off, std::ios::beg );

    while (std::getline(map_file, data)) {
        while (ReadLex(data, key, val)) {
            ProcessLex(key, val);
        }
    }

}

void geo_index_t::CacheRegion (const geo_rect_t& rect) {

    std::ifstream idx_file;
    LoadIdx (rect);

    std::ifstream map_file;

    map_file.open(m_map_file);

    for ( auto it_off = m_offssets.cbegin(); it_off != m_offssets.cend(); it_off++ ) {
        LoadObject( *it_off, map_file );
    }


}

#endif
