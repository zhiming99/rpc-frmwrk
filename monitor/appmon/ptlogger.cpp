/*
 * =====================================================================================
 *
 *       Filename:  ptlogger.cpp
 *
 *    Description:  implementation of point logger 
 *
 *        Version:  1.0
 *        Created:  07/30/2025 09:20:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "rpc.h"
#include <memory>
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "appmon.h"
#include "AppManagersvr.h"
#include "AppMonitorsvr.h"
#include <stdlib.h>
#include "monconst.h"
#include "blkalloc.h"

extern stdstr g_strAppInst;
extern RegFsPtr GetRegFs( bool bUser );
extern gint32 SplitPath( const stdstr& strPath,
    std::vector< stdstr >& vecComp );

#define LOG_MAGIC 0x706c6f67
struct LOGHDR
{
    guint32 dwMagic = LOG_MAGIC;
    guint32 dwCounter = 0;
    guint16 wTypeId = 0;
    guint16 wRecSize = 0;
    void hton()
    {
        dwMagic = htonl( dwMagic );
        dwCounter = htonl( dwCounter );
        wTypeId = htons( wTypeId );
        wRecSize = htons( wRecSize );
    }
    void ntoh()
    {
        dwMagic = ntohl( dwMagic );
        dwCounter = ntohl( dwCounter );
        wTypeId = ntohs( wTypeId );
        wRecSize = ntohs( wRecSize );
    }
};

typedef std::unique_ptr<FILE, decltype(std::fclose)*> FPHandle;

gint32 AppendLogExternal(
    const stdstr& strLogFile,
    const Variant& oVar )
{
    gint32 ret = 0;
    
    do{
        FPHandle fp( fopen(
            strLogFile.c_str(), "r+b" ), fclose );
        if( fp.get() == nullptr )
        {
            ret = -errno;
            break;
        }

        LOGHDR oHeader;
        ret = fread( &oHeader, sizeof( oHeader ), 1, fp.get() );
        if( ERROR( ret ) )
            break;
        oHeader.hton();

        if( oHeader.dwMagic != LOG_MAGIC )
        {
            ret = -EINVAL;
            break;
        }
        if( oVar.GetTypeId() !=
            ( EnumTypeId )oHeader.wTypeId )
        {
            ret = -EINVAL;
            break;
        }
        if( oHeader.wRecSize > sizeof( Variant ) )
        {
            ret = -EINVAL;
            break;
        }

        timespec ts;
        ret = clock_gettime( CLOCK_REALTIME, &ts );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        guint8 buf[ 32 ]; 
        guint32 timestamp = ntohl( ts.tv_sec );
        memcpy( buf, &timestamp, sizeof( timestamp ) );
        switch( ( EnumTypeId )oHeader.wTypeId )
        {
        case typeByte:
            buf[ sizeof( timestamp ) ] =
                ( guint8& )oVar;
            break;
        case typeUInt16:
            {
                guint16 wVal =
                    ntohs( ( guint16& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &wVal, sizeof( wVal ) );
                break;
            }
        case typeUInt32:
        case typeFloat:
            {
                guint32 dwVal =
                    ntohl( ( guint32& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &dwVal, sizeof( dwVal ) );
                break;
            }
        case typeUInt64:
        case typeDouble:
            {
                guint64 qwVal =
                    ntohll( ( guint64& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &qwVal, sizeof( qwVal ) );
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }
        if( ERROR( ret ) )
            break;
        fseek( fp.get(), 0, SEEK_END );
        ret = fwrite( buf, 1, oHeader.wRecSize, fp.get() );
        if( ret != oHeader.wRecSize )
        {
            ret = ERROR_FAIL;
            break;
        }
        oHeader.dwCounter += 1;
        oHeader.ntoh();
        fseek( fp.get(), 0, SEEK_SET );
        fwrite( &oHeader, 1, sizeof( oHeader ), fp.get() );
    }while( 0 );
    return ret;
}

gint32 AppendLog(
    const stdstr& strLogFile,
    const Variant& oVar )
{
    gint32 ret = 0;
    do{
        RegFsPtr pAppReg = GetRegFs( false );
        if( pAppReg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        RFHANDLE hFile = INVALID_HANDLE;
        ret = pAppReg->OpenFile(
            strLogFile, O_RDWR, hFile );
        if( ERROR( ret ) )
            break;

        LOGHDR oHeader;
        CFileHandle oHandle( pAppReg, hFile );
        guint32 dwSize = sizeof( oHeader );
        ret = pAppReg->ReadFile( hFile,
            ( char* )&oHeader, dwSize, 0 );
        if( ERROR( ret ) )
            break;

        oHeader.hton();
        if( oHeader.dwMagic != LOG_MAGIC )
        {
            ret = -EINVAL;
            break;
        }
        if( oVar.GetTypeId() !=
            ( EnumTypeId )oHeader.wTypeId )
        {
            ret = -EINVAL;
            break;
        }
        if( oHeader.wRecSize > sizeof( Variant ) )
        {
            ret = -EINVAL;
            break;
        }

        timespec ts;
        ret = clock_gettime( CLOCK_REALTIME, &ts );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        struct stat stBuf;
        ret = pAppReg->GetAttr( hFile, stBuf );
        if( ERROR( ret ) )
            break;

        guint8 buf[ 32 ]; 
        guint32 timestamp = ntohl( ts.tv_sec );
        memcpy( buf, &timestamp, sizeof( timestamp ) );
        switch( ( EnumTypeId )oHeader.wTypeId )
        {
        case typeByte:
            buf[ sizeof( timestamp ) ] =
                ( guint8& )oVar;
            break;
        case typeUInt16:
            {
                guint16 wVal =
                    ntohs( ( guint16& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &wVal, sizeof( wVal ) );
                break;
            }
        case typeUInt32:
        case typeFloat:
            {
                guint32 dwVal =
                    ntohl( ( guint32& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &dwVal, sizeof( dwVal ) );
                break;
            }
        case typeUInt64:
        case typeDouble:
            {
                guint64 qwVal =
                    ntohll( ( guint64& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &qwVal, sizeof( qwVal ) );
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }
        if( ERROR( ret ) )
            break;

        dwSize = oHeader.wRecSize;
        ret = pAppReg->WriteFile( hFile,
            ( char* )buf, dwSize, stBuf.st_size );
        if( ERROR( ret ) )
        {
            pAppReg->Truncate(
                hFile, stBuf.st_size );
        }

        oHeader.dwCounter+=1;
        oHeader.ntoh();
        dwSize = sizeof( oHeader );
        ret = pAppReg->WriteFile( hFile,
            ( char* )&oHeader, dwSize, 0 );

    }while( 0 );
    return ret;
}

gint32 LogPoints(
    IConfigDb* context, gint32 idx )
{
    if( idx < 0 || idx > 3 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        RegFsPtr pAppReg = GetRegFs( false );
        if( pAppReg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        stdstr strPtPath =
            g_strAppInst + "/points/ptlogger";  
        strPtPath.append( 1, ( char )idx + 0x31 );

        RFHANDLE hDir = INVALID_HANDLE;
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strPtPath;
        strPath += "/logptrs";
        std::vector< KEYPTR_SLOT > vecks;
        ret = pAppReg->OpenDir(
            strPath, O_RDONLY, hDir );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle( pAppReg, hDir );
        ret = pAppReg->ReadDir( hDir, vecks );
        if( ERROR( ret ) )
            break;
        if( vecks.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( auto& elem : vecks )
        {
            Variant oVar;
            ret = pAppReg->GetValue( hDir,
                elem.szKey, oVar );
            if( ERROR( ret ) )
                continue;

            std::vector< stdstr > vecComps;
            ret = SplitPath(
                ( stdstr& )oVar, vecComps );
            if( ERROR( ret ) )
            {
                DebugPrint( -EINVAL,
                    "Error log link is not valid" );
                continue;
            }

            if( vecComps.size() < 3 )
            {
                DebugPrint( -EINVAL,
                    "Error log link is not valid" );
                continue;
            }

            stdstr strPt2Log = "/" APPS_ROOT_DIR "/";
            strPt2Log += vecComps[ 0 ] +
                "/points/" + vecComps[ 1 ];

            stdstr strLogFile = strPt2Log +
                "/logs/" + vecComps[ 2 ] + "-0";

            bool bExternal = false;
            ret = pAppReg->Access( strLogFile, F_OK );
            if( ERROR( ret ) )
            {
                stdstr strLogLink = strPt2Log +
                    "/logs/" + vecComps[ 2 ] + "-extfile";
                
                RFHANDLE hFile2;
                ret = pAppReg->OpenFile(
                    strLogLink, O_RDONLY, hFile2 );
                if( ERROR( ret ) )
                {
                    DebugPrint( ret,
                        "Error open external log link %s",
                        strLogLink.c_str() );
                   continue; 
                }
                CFileHandle oHandle2( pAppReg, hFile2 );
                struct stat stBuf2;
                ret = pAppReg->GetAttr( hFile2, stBuf2 );
                if( ERROR( ret ) )
                {
                    DebugPrint( ret,
                        "Error read external log link %s",
                        strLogLink.c_str() );
                   continue; 
                }
                if( stBuf2.st_size == 0 )
                {
                    DebugPrint( ret,
                        "Error external log link %s is empty",
                        strLogLink.c_str() );
                   continue; 
                }
                BufPtr pBuf( true );
                guint32 dwSize = stBuf2.st_size;
                pBuf->Resize( dwSize );
                ret = pAppReg->ReadFile(
                    hFile2, pBuf->ptr(), dwSize, 0 );
                if( ERROR( ret ) )
                {
                    DebugPrint( ret,
                        "Error read external log link %s",
                        strLogLink.c_str() );
                   continue; 
                }
                strLogFile.clear();
                strLogFile.append( pBuf->ptr(), dwSize );
                bExternal = true;
            }

            Variant oVar2;
            ret = pAppReg->GetValue(
                strPt2Log + "/value", oVar2 );
            if( ERROR( ret ) )
            {
                DebugPrint( ret,
                    "Error get point value %s",
                    ( ( stdstr& )oVar ).c_str() );
               continue; 
            }
            if( bExternal )
                ret = AppendLogExternal(
                    strLogFile, oVar2 );
            else
                ret = AppendLog(
                    strLogFile, oVar2 );
            if( ERROR( ret ) )
            {
                DebugPrint( ret,
                    "Error logging %s to %s",
                    ( ( stdstr& )oVar ).c_str(),
                    strLogFile.c_str() );
                continue;
            }
        }

    }while( 0 );
    return ret;
}

gint32 RotateLog( gint32 idx )
{
    return 0;
}

