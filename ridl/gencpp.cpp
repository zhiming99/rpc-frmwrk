/*
 * =====================================================================================
 *
 *       Filename:  gencpp.cpp
 *
 *    Description:  implementations of proxy/server generator for C++
 *
 *        Version:  1.0
 *        Created:  02/25/2021 01:34:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "rpc.h"
#include "gencpp.h"
#include "sha1.h"

using namespace rpcfrmwrk;
extern std::string g_strAppName;

guint32 GenClsid( const std::string& strName )
{
    SHA1 sha;
    std::string strHash = g_strAppName + strName;
    sha.Input( strHash.c_str(), strHash.size() );

    unsigned char digest[20];
    sha.Result( (unsigned*)digest );
    char bottom = ( ( ( guint32 )
        UserClsidStart ) >> 24 ) + 1;
    for( int i = 0; i < sizeof( digest ); ++i )
        if( digest[ i ] >= bottom )
            break;
    if( sizeof( digest ) - i < sizeof( guint32 ) )
    {
        std::string strMsg = DebugMsg( -1,
            "error the SHA1 hash is not usable" );
        throw std::runtime_error( strMsg );
    }
    guint32 iClsid = 0;
    memcpy( &iClsid, digest + i, sizeof( iClsid );
    return iClsid;
}
