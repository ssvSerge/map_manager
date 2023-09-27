#include "MemMapper.h"

#include <iostream>

#define  IO_BUFF_LEN    (10 * 1024 * 1024)

file_mapper_t::file_mapper_t () {
    m_iViewSize = IO_BUFF_LEN;
    m_hFile     = INVALID_HANDLE_VALUE;
    m_hView     = INVALID_HANDLE_VALUE;
    m_iPageId   = 0;
    m_iReadPos  = 0;
    m_iFileSize = 0;
    m_pData     = NULL;
}

file_mapper_t::~file_mapper_t () {

    Close();
}

void file_mapper_t::Close ( void ) {

    if (m_pData != NULL) {
        UnmapViewOfFile(m_pData);
        m_pData = NULL;
    }

    if (m_hView != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hView);
        m_hView = INVALID_HANDLE_VALUE;
    }

    if (m_hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }

    m_iFileSize = 0;
    m_iPageId = 0;
    m_iViewSize = 0;
}

void file_mapper_t::Revind ( void ) {

    m_iReadPos = 0;
}

bool file_mapper_t::Init ( std::string oInFileName ) {

    bool retVal = false;

    m_hFile = CreateFile(oInFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (m_hFile != INVALID_HANDLE_VALUE) {

        LARGE_INTEGER iInFileSize;
        SIZE_T        view_size;

        GetFileSizeEx(m_hFile, &iInFileSize);
        m_iFileSize = iInFileSize.QuadPart;

        view_size = m_iFileSize;
        if (view_size > IO_BUFF_LEN) {
            view_size = IO_BUFF_LEN;
        }

        m_hView = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (m_hView != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER iViewOffset;
            iViewOffset.QuadPart = 0;
            m_pData = (char*)MapViewOfFile(m_hView, FILE_MAP_READ, iViewOffset.HighPart, iViewOffset.LowPart, view_size);
            if (m_pData != NULL) {
                retVal = true;
            }
        }
    }

    if (!retVal) {
        std::cout << "InStream Error Init failed" << std::endl;
    }

    return retVal;
}

uint64_t file_mapper_t::GetSize ( void ) {

    return m_iFileSize;
}

char file_mapper_t::operator[] (uint64_t idx) {

    char ret_val = 0;

    uint64_t view_page_id;
    uint64_t view_page_offset;

    view_page_id = idx / m_iViewSize;
    view_page_offset = idx % m_iViewSize;

    if (idx >= m_iFileSize) {
        std::cout << "InStream Error idx out of range " << std::endl;
    }
    else {

        if (m_pData == NULL) {
            std::cout << "InStream Error m_pData is NULL " << std::endl;
        }
        else {

            if (view_page_id != m_iPageId) {

                LARGE_INTEGER  iViewOffset;
                int64_t        iViewSize;

                iViewOffset.QuadPart = view_page_id;
                iViewOffset.QuadPart *= m_iViewSize;

                iViewSize = m_iFileSize - iViewOffset.QuadPart;

                if (iViewSize > m_iViewSize) {
                    iViewSize = m_iViewSize;
                }

                UnmapViewOfFile(m_pData);

                m_pData = (char*)MapViewOfFile(m_hView, FILE_MAP_READ, iViewOffset.HighPart, iViewOffset.LowPart, iViewSize);

                m_iPageId = view_page_id;
            }

            ret_val = m_pData[view_page_offset];
        }

    }

    return ret_val;
}

bool file_mapper_t::Read ( uint64_t pos, void* buff, uint64_t cnt ) {

    uint8_t* dst = (uint8_t*)buff;

    bool ret_val = true;

    for (uint64_t i = pos; i < pos + cnt; i++) {

        if (i >= m_iFileSize) {
            ret_val = false;
            *dst = 0;
        }
        else {
            *dst = this->operator[] (i);
        }

        dst++;
    }

    return ret_val;
}

bool file_mapper_t::Read ( void* buff, uint64_t cnt ) {

    bool ret_val = Read(m_iReadPos, buff, cnt);
    m_iReadPos += cnt;

    return ret_val;
}
