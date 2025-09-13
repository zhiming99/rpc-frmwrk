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

#define MAX_LOG_FILES 9
extern stdstr g_strAppInst;
extern RegFsPtr GetRegFs( bool bUser );
extern gint32 SplitPath( const stdstr& strPath,
    std::vector< stdstr >& vecComp );
extern InterfPtr GetAppManager();

#define LOG_MAGIC 0x706c6f67
#define NPLock CNamedProcessLock

#define ROTATE_LIMIT ( 1024 * 1024 )

enum EnumAvgAlgo : guint32
{
    algoDiff = 0,
    algoAvg = 1,
};

struct LOGHDR
{
    guint32 dwMagic = LOG_MAGIC;
    guint32 dwCounter = 0;
    guint16 wTypeId = 0;
    guint16 wRecSize = 0;
    guint8  byUnit = 0;
    guint8  reserved[ 3 ] = {0};
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

typedef std::vector< std::pair< time_t, Variant > > LOGRECVEC;
gint32 GetLatestLogs(
    const stdstr& strLogFile,
    guint32 dwNumRec,
    LOGRECVEC& vecRecs )
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
            strLogFile, O_RDONLY, hFile );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle( pAppReg, hFile );

        LOGHDR oHeader;
        guint32 dwSize = sizeof( oHeader );
        ret = pAppReg->ReadFile( hFile,
            ( char* )&oHeader, dwSize, 0 );
        if( ERROR( ret ) )
            break;

        oHeader.ntoh();
        if( oHeader.dwMagic != LOG_MAGIC )
        {
            ret = -EINVAL;
            break;
        }
        if( oHeader.wRecSize > sizeof( Variant ) )
        {
            ret = -EINVAL;
            break;
        }
        if( oHeader.dwCounter == 0 )
        {
            ret = -ENOENT;
            break;
        }

        struct stat stBuf;
        ret = pAppReg->GetAttr( hFile, stBuf );
        if( ERROR( ret ) )
            break;

        if( stBuf.st_size - sizeof( oHeader ) <
            ( size_t )oHeader.dwCounter * oHeader.wRecSize )
        {
            ret = -ERANGE;
            break;
        }

        guint32 dwOffset = sizeof( oHeader );
        guint32 dwActNum = dwNumRec;
        if( oHeader.dwCounter < dwNumRec )
            dwActNum = oHeader.dwCounter;

        dwOffset += oHeader.wRecSize *
            ( oHeader.dwCounter - dwActNum );
        if( stBuf.st_size <= ( size_t )dwOffset ||
            ( stBuf.st_size - dwOffset !=
                ( size_t )oHeader.wRecSize * dwActNum ) )
        {
            ret = -EBADMSG;
            break;
        }

        BufPtr pBuf( true );
        ret = pBuf->Resize(
            dwActNum * oHeader.wRecSize );
        if( ERROR( ret ) )
            break;

        dwSize = pBuf->size();
        ret = pAppReg->ReadFile( hFile,
            pBuf->ptr(), dwSize, dwOffset );
        if( ERROR( ret ) )
            break;

        auto buf = pBuf->ptr(); 
        for( int i = 0; i < dwActNum; i++ )
        {
            guint32 dwTimestamp;
            memcpy( &dwTimestamp,
                buf, sizeof( guint32 ) );
            dwTimestamp = htonl( dwTimestamp );
            Variant oVar;
            guint8* pData = ( guint8* )
                ( buf + sizeof( guint32 ) );
            switch( ( EnumTypeId )oHeader.wTypeId )
            {
            case typeByte:
                oVar = pData[0];
                break;
            case typeUInt16:
                {
                    guint32 wVal; 
                    memcpy( &wVal, pData, sizeof( guint16 ) );
                    wVal = ntohs( ( guint16& )pData );
                    oVar = wVal;
                    break;
                }
            case typeUInt32:
                {
                    guint32 dwVal; 
                    memcpy( &dwVal, pData, sizeof( guint32 ) );
                    dwVal = ntohl( dwVal );
                    oVar = dwVal;
                    break;
                }
            case typeFloat:
                {
                    guint32 dwVal; 
                    memcpy( &dwVal, pData, sizeof( guint32 ) );
                    dwVal = ntohl( dwVal );
                    oVar = *( ( float* )&dwVal );
                    break;
                }
            case typeUInt64:
                {
                    guint64 qwVal; 
                    memcpy( &qwVal, pData, sizeof( guint64 ) );
                    qwVal = ntohll( qwVal );
                    oVar = qwVal;
                    break;
                }
            case typeDouble:
                {
                    guint64 qwVal; 
                    memcpy( &qwVal, pData, sizeof( guint64 ) );
                    qwVal = ntohll( qwVal );
                    oVar = *( ( double* )&qwVal );
                    break;
                }
            default:
                ret = -ENOTSUP;
                break;
            }
            if( ERROR( ret ) )
                break;

            buf += oHeader.wRecSize;
            vecRecs.push_back( { dwTimestamp, oVar } );
        }

    }while( 0 );
    return ret;
}

gint32 UpdateAverages( gint32 idx )
{
    if( idx < 0 || idx > 3 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        InterfPtr pIf = GetAppManager();
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        CAppManager_SvrImpl* pam = pIf;
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

            if( !pam->IsAppOnline( vecComps[ 0 ] ) )
                continue;

            guint32 dwAlgo = ( guint32 )algoDiff;
            Variant varAlgo;
            ret = pAppReg->GetValue(
                strPt2Log + "/avgalgo", varAlgo );
            if( SUCCEEDED( ret ) )
               dwAlgo = varAlgo;

            LOGRECVEC vecRecs;
            ret = GetLatestLogs(
                strLogFile, 8, vecRecs );

            if( SUCCEEDED( ret ) )
            {
                time_t& ts = vecRecs.back().first;
                Variant& oLast = vecRecs.back().second;

                time_t& ts1 = vecRecs.front().first;
                Variant& oFirst = vecRecs.front().second;
                if( ts1 >= ts )
                    continue;
                guint32 dwSecs = ts - ts1;
                double dblAvg = .0;
                if( dwAlgo == algoDiff )
                {
                    switch( oLast.GetTypeId() )
                    {
                    case typeByte:
                        {
                            guint8 diff = 
                            ( ( guint8& )oLast - ( guint8&) oFirst );
                            dblAvg = ( ( double )diff ) / (double)( ts - ts1 );
                            break;
                        }
                    case typeUInt16:
                        {
                            guint16 diff = 
                            ( ( guint16& )oLast - ( guint16&) oFirst );
                            dblAvg = ( ( double )diff ) / (double)( ts - ts1 );
                            break;
                        }
                    case typeUInt32:
                        {
                            guint32 diff = 
                            ( ( guint32& )oLast - ( guint32&) oFirst );
                            dblAvg = ( ( double )diff ) / (double)( ts - ts1 );
                            break;
                        }
                    case typeFloat:
                        {
                            float diff = 
                            ( ( float& )oLast - ( float&) oFirst );
                            dblAvg = ( ( double )diff ) / (double)( ts - ts1 );
                            break;
                        }
                    case typeUInt64:
                        {
                            guint64 diff = 
                            ( ( guint64& )oLast - ( guint64&) oFirst );
                            dblAvg = ( ( double )diff ) / (double)( ts - ts1 );
                            break;
                        }
                    case typeDouble:
                        {
                            double diff = 
                            ( ( double& )oLast - ( double&) oFirst );
                            dblAvg = diff / (double)( ts - ts1 );
                            break;
                        }
                    default:
                        ret = -ENOTSUP;
                        break;
                    }
                    if( ERROR( ret ) )
                        continue;
                }
                else if( dwAlgo == algoAvg )
                {
                    switch( oLast.GetTypeId() )
                    {
                    case typeByte:
                        {
                            guint8 diff = 
                            ( ( guint8& )oLast + ( guint8&) oFirst );
                            dblAvg = ( ( double )diff ) / 2;
                            break;
                        }
                    case typeUInt16:
                        {
                            guint16 diff = 
                            ( ( guint16& )oLast + ( guint16&) oFirst );
                            dblAvg = ( ( double )diff ) / 2;
                            break;
                        }
                    case typeUInt32:
                        {
                            guint32 diff = 
                            ( ( guint32& )oLast + ( guint32&) oFirst );
                            dblAvg = ( ( double )diff ) / 2;
                            break;
                        }
                    case typeFloat:
                        {
                            float diff = 
                            ( ( float& )oLast + ( float&) oFirst );
                            dblAvg = ( ( double )diff ) / 2;
                            break;
                        }
                    case typeUInt64:
                        {
                            guint64 diff = 
                            ( ( guint64& )oLast + ( guint64&) oFirst );
                            dblAvg = ( ( double )diff ) / 2;
                            break;
                        }
                    case typeDouble:
                        {
                            double diff = 
                            ( ( double& )oLast + ( double&) oFirst );
                            dblAvg = ( ( double )diff ) / 2;
                            break;
                        }
                    default:
                        ret = -ENOTSUP;
                        break;
                    }
                    if( ERROR( ret ) )
                        continue;
                }
                Variant oVar( dblAvg );
                stdstr strAvg = strPt2Log + "/average";
                pAppReg->SetValue( strAvg, dblAvg );
            }
        }

    }while( 0 );
    return ret;
}

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
        oHeader.ntoh();

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
        guint32 timestamp = htonl( ts.tv_sec );
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
                    htons( ( guint16& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &wVal, sizeof( wVal ) );
                break;
            }
        case typeUInt32:
        case typeFloat:
            {
                guint32 dwVal =
                    htonl( ( guint32& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &dwVal, sizeof( dwVal ) );
                break;
            }
        case typeUInt64:
        case typeDouble:
            {
                guint64 qwVal =
                    htonll( ( guint64& )oVar );
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
        oHeader.hton();
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

        oHeader.ntoh();
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
        guint32 timestamp = htonl( ts.tv_sec );
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
                    htons( ( guint16& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &wVal, sizeof( wVal ) );
                break;
            }
        case typeUInt32:
        case typeFloat:
            {
                guint32 dwVal =
                    htonl( ( guint32& )oVar );
                memcpy( buf + sizeof( timestamp ),
                    &dwVal, sizeof( dwVal ) );
                break;
            }
        case typeUInt64:
        case typeDouble:
            {
                guint64 qwVal =
                    htonll( ( guint64& )oVar );
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
        oHeader.hton();
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
        InterfPtr pIf = GetAppManager();
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        CAppManager_SvrImpl* pam = pIf;
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

            if( !pam->IsAppOnline( vecComps[ 0 ] ) )
                continue;

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

            NPLock( strLogFile + ".lock" );
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

gint32 ShiftFiles(
    RegFsPtr& pAppReg,
    const stdstr& strParentDir,
    const stdstr& strPrefix )
{
    gint32 ret = 0;
    do{
        stdstr strCurPath =
        strParentDir + "/" + strPrefix + "-";
        int i = MAX_LOG_FILES;
        ret = pAppReg->Access( strCurPath +
            std::to_string( i ), F_OK );
        if( SUCCEEDED( ret ) )
        {
            pAppReg->RemoveFile(
                strCurPath + std::to_string( i ) );
        }
        for( i = MAX_LOG_FILES - 1; i > 0; i-- )
        {
            stdstr si = std::to_string( i );
            stdstr sn = std::to_string( i + 1 );

            ret = pAppReg->Access(
                strCurPath + si, F_OK );
            if( ERROR( ret ) )
                continue;

            ret = pAppReg->Rename(
                strCurPath + si,
                strCurPath + sn );
            if( ERROR( ret ) )
            {
                OutputMsg( ret,
                    "Error shift files %s->%s",
                    ( strCurPath + si ).c_str(),
                    ( strCurPath + sn ).c_str() );
                break;
            }
        }

        NPLock( strCurPath + "0.lock" );

        RFHANDLE hFile = INVALID_HANDLE;
        ret = pAppReg->OpenFile( strCurPath + "0",
            O_RDWR, hFile );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle( pAppReg, hFile );
        struct stat stBuf;
        BufPtr pBuf( true );
        ret = pAppReg->GetAttr( hFile, stBuf );
        if( SUCCEEDED( ret ) )
        {
            guint32 dwSize = stBuf.st_size;
            ret = pBuf->Resize( dwSize );
            if( ERROR( ret ) )
                break;
            ret = pAppReg->ReadFile(
                hFile, pBuf->ptr(), dwSize, 0 );
            if( ERROR( ret ) )
            {
                OutputMsg( ret, "Error copy logfile %s",
                    ( strCurPath + "0" ).c_str() );
                break;
            }
            ret = pAppReg->Truncate(
                hFile, sizeof( LOGHDR ) );
            if( ERROR( ret ) )
                break;
            LOGHDR oHeader;
            memcpy( &oHeader,
                pBuf->ptr(), sizeof( LOGHDR ) );
            oHeader.dwCounter = 0;
            dwSize = sizeof( oHeader );
            ret = pAppReg->WriteFile( hFile,
                ( char* )&oHeader, dwSize, 0 );
            if( ERROR( ret ) )
                break;
        }

        if( pBuf->size() > sizeof( LOGHDR ) )
        {
            RFHANDLE hFile1;
            CAccessContext oac;
            oac.dwUid = UID_ADMIN;
            oac.dwGid = GID_ADMIN;
            ret = pAppReg->CreateFile( strCurPath + "1",
                0644, O_WRONLY | O_TRUNC, hFile1, &oac );
            if( SUCCEEDED( ret ) )
            {
                CFileHandle oHandle( pAppReg, hFile1 );
                guint32 dwSize = pBuf->size();
                ret = pAppReg->WriteFile( hFile1,
                    pBuf->ptr(), dwSize, 0 );
            }
        }

    }while( 0 );
    return ret;
}

gint32 RotateLog( gint32 idx )
{
    if( idx < 0 || idx > 3 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        InterfPtr pIf = GetAppManager();
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        CAppManager_SvrImpl* pam = pIf;
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

            struct stat stBuf;
            ret = pAppReg->GetAttr( strLogFile, stBuf );
            if( ERROR( ret ) )
                continue;
            if( stBuf.st_size <= ROTATE_LIMIT )
                continue;

            ret = ShiftFiles( pAppReg,
                strPt2Log + "/logs/",
                vecComps[ 2 ] );
        }

    }while( 0 );
    return ret;
}

