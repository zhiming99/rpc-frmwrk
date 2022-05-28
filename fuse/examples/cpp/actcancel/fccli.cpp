//compile: g++ -o fccli -ggdb -std=c++11 -lstdc++ -ldl fccli.cpp
#include "stdio.h"
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main( int argc, char** argv )
{
    if( argc < 2 )
        return EINVAL;

    int ret = 0;
    std::string strPath = argv[1];
    strPath += "/streams/stream_0";
    int fd = open( strPath.c_str(), O_CREAT | O_WRONLY, 0600 );
    if( fd < 0 )
    {
        perror(strerror(fd));
        printf( "usage: fctest.py <service path>\n");
        printf( "\t<service path> is /'path to mountpoint'/connection_X/ActiveCancel\n");
        return fd;
    }
    for( int i = 0; i < 10; i++)
        for(int j = 0; j < 101; j++ )
        {
            std::string strMsg = "Hello, World ";
            strMsg += std::to_string(j+1000*i);
            strMsg.push_back('\n');
            ret = write( fd, strMsg.c_str(), strMsg.size() );
            if( ret >= 0 )
                continue;
            ret = errno;
            if( ret == EAGAIN )
            {
                fd_set wfds;
                FD_ZERO( &wfds );
                FD_SET( fd, &wfds );
                select(fd+1, nullptr, &wfds, nullptr, nullptr );
                j--;
            }
            break;
        }

    close(fd);
    return ret;
}