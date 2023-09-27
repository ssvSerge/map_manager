#ifndef __MEMMAPPER_H__
#define __MEMMAPPER_H__

#include <string>
#include <iostream>

#include <windows.h>

#define  IO_BUFF_LEN    (10 * 1024 * 1024)


class file_mapper_t {

    public:
        file_mapper_t ();
        ~file_mapper_t ();

    public:
        void Close  ( void );
        bool Init   ( std::string oInFileName );
        void Revind ( void );
        bool Read   ( void* buff, uint64_t cnt );
        bool Read   ( uint64_t pos, void* buff, uint64_t cnt );
        uint64_t GetSize ( void );

        char operator[] ( uint64_t idx );

    private:
        HANDLE          m_hFile;
        HANDLE          m_hView;
        uint64_t        m_iFileSize;
        uint64_t        m_iReadPos;
        const char*     m_pData;
        uint64_t        m_iPageId;
        uint32_t        m_iViewSize;
};

#endif
