/*
 * =====================================================================================
 *
 *       Filename:  parsrctx.cpp
 *
 *    Description:  implementations of classes for parser context
 *
 *        Version:  1.0
 *        Created:  05/08/2026 06:31:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2026 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <rpc.h>
#include "parsrctx.h"
using namespace rpcf;

namespace rpcf
{
    
FILECTX2::FILECTX2()
{
    m_oVal.Clear(); 
    m_oLocation.first_line = 1;
    m_oLocation.first_column =  1; 
    m_oLocation.last_line = 1;
    m_oLocation.last_column = 1;
}

FILECTX2::FILECTX2( const std::string& strPath )
    : FILECTX2()
{
    m_fp = fopen( strPath.c_str(), "r");
    if( m_fp != nullptr )
    {
        m_strPath = strPath;
    }
    else
    {
        std::string strMsg = "error cannot open file '";
        strMsg += strPath + "'";
        throw std::invalid_argument( strMsg );
    }
}

FILECTX2::~FILECTX2()
{
    if( m_fp != nullptr )
    {
        fclose( m_fp );
        m_fp = nullptr;
    }
}

FILECTX2::FILECTX2( const FILECTX2& rhs )
{
    m_fp = rhs.m_fp;
    m_oVal = rhs.m_oVal;
    m_strPath = rhs.m_strPath;
    m_strLastVal = rhs.m_strLastVal;
    memcpy( &m_oLocation,
        &rhs.m_oLocation,
        sizeof( m_oLocation ) );
}

FILE* CSTParserContext::TryOpenFile(
    const std::string& strFile )
{
    FILE* fp = fopen( strFile.c_str(), "r");
    if ( fp != nullptr )
        return fp;

    for( auto elem : m_vecInclPaths )
    {
        std::string strPath =
            elem + "/" + strFile;
        fp = fopen( strPath.c_str(), "r" );
        if( fp != nullptr )
            break;
    }
    return fp;
}

void ParserPrint(
    const char* szFile,
    gint32 iLineNo,
    const char* strMsg )
{
    char szBuf[ 512 ];
    sprintf( szBuf, "%s(%d): %s",
        szFile, iLineNo, strMsg );
    fprintf( stderr, szBuf );
}

}