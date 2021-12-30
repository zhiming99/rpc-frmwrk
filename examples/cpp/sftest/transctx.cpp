#include "rpc.h"
using namespace rpcf;
#include "transctx.h"
#include <sys/stat.h>
#include <regex>

bool IsDirExist(const std::string& path )
{
    struct stat info;
    if( stat( path.c_str(), &info ) != 0 )
        return false;
    return ( info.st_mode & S_IFDIR ) != 0;
}

gint32 MakeDir( const stdstr& path )
{
    mode_t mode = 0755;
    gint32 ret = mkdir( path.c_str(), mode );
    if( ret < 0 )
        ret = -errno;
    return ret;
}

bool IsFileExist(const std::string& path )
{
    struct stat info;
    if( stat( path.c_str(), &info ) != 0 )
        return false;
    return ( info.st_mode & S_IFREG ) != 0;
}

size_t GetFileSize(const std::string& path )
{
    struct stat info;
    if( stat( path.c_str(), &info ) != 0 )
        return 0;
    return info.st_size;
}

bool IsFileNameValid( const stdstr& strFile )
{
    std::regex e( "^[A-Za-z0-9_][A-Za-z0-9_.]*" );
    return std::regex_match( strFile, e );
}

