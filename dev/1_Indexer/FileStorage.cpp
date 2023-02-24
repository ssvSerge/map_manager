#include <cassert>
#include <fcntl.h>
#include <io.h>

#include "FileStorage.h"


FileStorage::FileStorage() {
}

void FileStorage::View( const char* const name ) {

    int open_flags = O_RDONLY | O_BINARY;

    create_names(name);

    m_file_d = open ( m_dat_file_name.c_str(), open_flags, O_RDWR );
    assert ( m_file_d != -1 );

    m_file_i = open ( m_idx_file_name.c_str(), open_flags, O_RDWR );
    assert ( m_file_i != -1 );
}

void FileStorage::Create ( const char* const name ) {

    int open_flags = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY;

    create_names ( name );

    m_file_d = open ( m_dat_file_name.c_str(), open_flags, _S_IREAD | _S_IWRITE );
    assert ( m_file_d != -1 );

    m_file_i = open ( m_idx_file_name.c_str(), open_flags, _S_IREAD | _S_IWRITE );
    assert ( m_file_i != -1 );
}

void FileStorage::Close ( void ) {

    close(m_file_d);
    m_file_d = -1;

    close(m_file_i);
    m_file_i = -1;
}

void FileStorage::create_names(const char* const name) {

    m_dat_file_name  = name;
    m_dat_file_name += ".dat";

    m_idx_file_name  = name;
    m_idx_file_name += ".idx";
}

void FileStorage::Store ( uint64_t id, const void* const data, int len ) {

    int io_cnt;

    m_idx.id   = id;
    m_idx.off  = m_offset;

    m_offset  += len;

    io_cnt = write ( m_file_d, data, len );
    if ( io_cnt != len ) {
        assert(false);
    }

    io_cnt = write ( m_file_i, &m_idx, sizeof(m_idx) );
    if (io_cnt != sizeof(m_idx) ) {
        assert(false);
    }
}
