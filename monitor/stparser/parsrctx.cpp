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
#include <iostream>
#include <sstream>

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

gint32 CSTParserContext::GetFileIdx(
    const stdstr& strFile ) const
{
    for( int i = 0; i < m_vecFiles.size(); i++ )
        if( m_vecFiles[ i ] == strFile )
            return i;
    return -1; 
}

bool CSTParserContext::IsFileOnStack(
    const stdstr& strFile ) const
{
    bool ret = false;
    for( int i = 0; i < m_vecFileStack.size(); i++ )
        if( m_vecFileStack[ i ]->m_strPath == strFile )
            return true;
    return false; 
}

FILE* CSTParserContext::TryOpenFile(
    const std::string& strFile,
    stdstr& strFullPath )
{
    FILE* fp = nullptr;
    stdstr strCurPath;
    do{
        if( strFile.empty() )
            break;

        if( m_strCurFile.size() && strFile[ 0 ] != '/' )
        {
            strCurPath =
                GetDirName( m_strCurFile );

            char* pPath = realpath(
                strCurPath.c_str(), nullptr );
            if( pPath == nullptr )
                break;

            strCurPath = pPath;
            free( pPath );
            strCurPath += "/";
            strCurPath += strFile;
        }
        else if( m_strCurFile.empty() )
        {
            // find in the working dir
            char* pPath = realpath( "./", nullptr );
            if( pPath == nullptr )
                break;
            strCurPath = pPath;
            free( pPath );
            strCurPath += "/";
            strCurPath += strFile;
        }
        else
        {
            strCurPath = strFile;
        }

        fp = fopen( strCurPath.c_str(), "r");
        if ( fp != nullptr )
            break;

        if( strFile[ 0 ] == '/' )
            break;

        for( auto elem : m_vecInclPaths )
        {
            strCurPath =
                elem + "/" + strFile;
            fp = fopen( strCurPath.c_str(), "r" );
            if( fp != nullptr )
                break;
        }

    }while( 0 );
    if( fp )
    {
        bool bExist = false;
        for( auto& i : m_vecFiles )
        {
            if( i == strFile )
            {
                bExist = true;
                break;
            }
        }
        if( !bExist )
            m_vecFiles.push_back( strCurPath );
        strFullPath = strCurPath;
    }
    return fp;
}

}


void ParserPrint(
    const char* szFile,
    gint32 iLineNo,
    const char* strMsg,
    bool bErr )
{
    std::ostringstream ss;

    ss << szFile << "(" << iLineNo << "): "
       << strMsg << "\n";

    if( bErr )
        std::cerr << ss.str();
    else
        std::cout << ss.str();
}

