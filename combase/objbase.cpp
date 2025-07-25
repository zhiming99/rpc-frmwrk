/*
 * =====================================================================================
 *
 *       Filename:  objbase.cpp
 *
 *    Description:  implementation of class CObjBase and other base classes
 *
 *        Version:  1.0
 *        Created:  04/12/2016 08:57:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include <string>
#include <vector>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "defines.h"
#include "autoptr.h"
#include "buffer.h"
#include "variant.h"
#include "objfctry.h"
#include <unordered_map>
#include <unordered_set>
#include <byteswap.h>

#include <memory>
#include <iostream>
#include <array>
#include "sha1.h"

#include <fstream>
#include <sstream>
#include <dirent.h>

#define MAX_DUMP_SIZE 512

namespace rpcf
{

EnumLogLvl g_dwLogLevel = logErr;

std::string DebugMsgInternal(
    gint32 ret, const std::string& strMsg,
    const char* szFunc, gint32 iLineNum )
{
    timespec ts = { 0 };
    clock_gettime( CLOCK_REALTIME, &ts );

    char szBuf[ MAX_DUMP_SIZE ];
    szBuf[ sizeof( szBuf ) - 1 ] = 0;
    snprintf( szBuf,
        sizeof( szBuf ) - 1,
        "[%ld.%09ld-%d]%s(%d): %s(%d)",
        ts.tv_sec,
        ts.tv_nsec,
        getpid(),
        szFunc,
        iLineNum,
        strMsg.c_str(),
        ret );
    return std::string( szBuf );
}

std::string DebugMsgEx(
    const char* szFunc, gint32 iLineNum,
    const std::string& strFmt, gint32 ret, ... ) 
{
    char szBuf[ MAX_DUMP_SIZE ];

    if( strFmt.size() >= sizeof( szBuf ) )
        return std::string( "" );

    gint32 iSize = sizeof( szBuf );
    szBuf[ iSize - 1 ] = 0;

    va_list argptr;
    va_start(argptr, ret );
    vsnprintf( szBuf, iSize - 1, strFmt.c_str(), argptr );
    va_end(argptr);

    return DebugMsgInternal( ret, szBuf, szFunc, iLineNum );
}

gint64 GetRandom()
{
    static bool bInit = false;
    timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    if( !bInit )
    {
        bInit = true;
        srand( ts.tv_nsec );
    }

    guint32 iPid = GetTid();
    guint32 dwRand = rand();

    // swap the bytes to increase the differences
    // between the two adjacent Get less than one
    // second.
    guint64 dwLowVal = bswap_32(
        ( ts.tv_nsec << 2 ) +
        ( dwRand & 0x3 ) ) ;

    guint64 qwHighVal =
        ( ( dwRand & 0xFFFF0000 ) | iPid );

    return ( qwHighVal << 32 )  + dwLowVal;
}

// Generate a 32bit hash using the sha1hash
gint32 GenStrHash(
    const stdstr& strMsg, guint32& dwHash )
{
    if( strMsg.empty() )
        return -EINVAL;

    SHA1 sha;
    sha.Input( strMsg.c_str(),
        strMsg.size() );

    guint32 arrDwords[ 5 ];
    if( !sha.Result( arrDwords ) )
        return ERROR_FAIL;

    guint32* ptr = ( guint32* )&dwHash;
    ptr[ 0 ] = arrDwords[ 3 ];
    return 0;
}

gint32 GetCmdOutput( std::string& strResult,
    const char* cmd )
{
    gint32 ret = 0;
    do{
        std::array<char, 128> buffer;
        std::string result;
        FILE* fp = nullptr;
        fp = popen( cmd, "r" );
        if( fp == nullptr )
        {
            ret = -errno;
            break;
        }
        std::unique_ptr< FILE, decltype(&pclose) >
            pipe( fp, pclose );

        if (!pipe)
        {
            ret = -EBADF;
            break;
        }
        while(fgets(buffer.data(),
            buffer.size(),
            pipe.get()) != nullptr)
        {
            strResult += buffer.data();
        }

        if( strResult.empty() )
        {
            ret = -ENOENT;
            break;
        }

        if( strResult[ 0 ] == '[' )
        {
            ret = -EBADF;
            break;
        }

        if( strResult[ 0 ] == ' ' ||
            strResult[ 0 ] == '\t' )
        {
            ret = -ENOTDIR;
            break;
        }

        std::string::size_type iPos =
            strResult.find_first_of( " \t\r\n" );

        if( iPos == std::string::npos )
            break;

        strResult = strResult.substr( 0, iPos );

    }while( 0 );

    return ret;
}

gint32 GetEnvLibPath( std::vector< std::string >& oSet )
{
    char* szEnv = secure_getenv(
        "LD_LIBRARY_PATH" );

    if( szEnv == nullptr )
        return -ENOENT;
    
    std::string s = szEnv;
    char delim = ':';

    size_t pos = 0;
    std::string token;
    pos = s.find_first_of( delim );
    if( pos == std::string::npos )
    {
        oSet.push_back( s );
        return STATUS_SUCCESS;
    }
    while( pos != std::string::npos )
    {
        if( pos > 0 )
        {
            token = s.substr( 0, pos );
            oSet.push_back( token );
        }
        s.erase( 0, pos + 1 );
        pos = s.find_first_of( delim );
    }
    return STATUS_SUCCESS;
}

gint32 GetLibPathName(
    std::string& strResult,
    const char* szLibName )
{
    if( szLibName == nullptr )
        szLibName = "libcombase.so";

    bool bFound = false;
    char szMap[ 128 ];

    snprintf( szMap, sizeof( szMap ) - 1,
        "/proc/%d/maps", getpid() );

    FILE* fp = fopen( szMap, "r");
    if( fp == nullptr )
        return -errno;

    char line[ 1024 ];
    while( fgets( line, sizeof( line ), fp ) != NULL)
    {
        char* szPath = strchr(line, '/');
        if( szPath == nullptr )
            continue;
        char* szLoc = strstr( szPath, szLibName );
        if( szLoc == nullptr )
            continue;

        char* szNewLine = strchr( szLoc, '\n' );
        if( szNewLine != nullptr )
            *szNewLine = 0;

        strResult = szPath;
        bFound = true;
        break;
    }

    fclose( fp );
    fp = nullptr;
    if( !bFound )
        return -ENOENT;
    return 0;
}

gint32 GetLibPath( std::string& strResult,
    const char* szLibName )
{
    gint32 ret = 0;
    do{
        ret = GetLibPathName(
            strResult, szLibName );

        if( ERROR( ret ) )
            break;

        size_t pos = strResult.rfind( '/' );
        if( pos == std::string::npos )
            strResult.clear();
        else
            strResult.erase( pos );

    }while( 0 );
    return ret;
}

#include <libgen.h>
gint32 GetModulePath( std::string& strResult )
{
    char cmd[ 1024 ];
    gint32 ret = readlink(
        "/proc/self/exe", cmd, sizeof( cmd ) - 1 );
    if( ret < 0 )
        return -errno;
    cmd[ ret ] = 0;
    strResult = dirname( cmd );
    return 0;
}

stdstr GetDirName( const stdstr& strPath )
{
    char szPath[ 1024 ];
    size_t pos = strPath.copy(
        szPath, sizeof( szPath ) - 1 );
    szPath[ pos ] = 0;
    stdstr strRet = dirname( szPath );
    return strRet;
}

gint32 FindInstCfg(
    const std::string& strFileName,
    std::string& strPath )
{
    ObjPtr pObj;
    gint32 ret = 0;

    if( strFileName.empty() )
        return -EINVAL;

    // relative path
    std::string strFile = "./";
    char buf[ 1024 ];
    buf[ sizeof( buf ) - 1 ] = 0;
    size_t iSize = std::min(
        strFileName.size() + 1, sizeof( buf ) - 1 );
    strncpy( buf,
        strFileName.c_str(), iSize );
    strFile += basename( buf );

    do{
        std::string strFullPath = "/etc/rpcf/";
        strFullPath += strFile;
        ret = access( strFullPath.c_str(), R_OK );
        if( ret == 0 )
        {
            strPath = strFullPath;
            break;
        }

        strFullPath =
            stdstr( "/usr/local" ) + strFullPath;
        ret = access( strFullPath.c_str(), R_OK );
        if( ret == 0 )
        {
            strPath = strFullPath;
            break;
        }

        strFullPath.clear();
        ret = GetLibPath( strFullPath );
        if( ERROR( ret ) )
            break;

        strFullPath += "/../etc/rpcf/" + strFile;
        ret = access( strFullPath.c_str(), R_OK );
        if( ret == 0 )
            strPath = strFullPath;

    }while( 0 );

    return ret;
}

gint32 ReadJsonCfgFile(
    const std::string& strFile,
    Json::Value& valConfig )
{

    gint32 ret = 0;
    FILE* fp = NULL;
    size_t iLen = 0;
    do{
        fp = fopen( strFile.c_str(), "rb" );
        if( fp == NULL )
        {
            ret = -errno;
            break;
        }
        ret = fseek( fp, 0, SEEK_END );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        iLen = ( size_t )ftell( fp );
        if( ERROR( iLen ) )
        {
            ret = -errno;
            break;
        }

        if( iLen < 100
            || iLen > 1024 * 1024 )
        {
            ret = -EBADMSG;
            break;
        }

        fseek( fp, 0, SEEK_SET );

        BufPtr buf( true );
        buf->Resize( iLen + 16 );
        size_t iSize = fread( buf->ptr(), 1, iLen, fp );
        if( iSize < iLen )
        {
            ret = -EBADF;
            break;
        }

        Json::CharReaderBuilder oBuilder;
        Json::CharReader* pReader = nullptr;
        pReader = oBuilder.newCharReader();
        if( pReader == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        if( !pReader->parse( buf->ptr(),
            buf->ptr() + buf->size(),
            &valConfig, nullptr ) )
        {
            ret = -EBADMSG;
        }

        delete pReader;

    }while( 0 );

    if( fp != nullptr )
    {
        fclose( fp );
    }

    return ret;
}


int Sem_Init( sem_t* psem, int pshared, unsigned int value )
{
    if( psem == nullptr )
        return -EINVAL;

    // NOTE: if we don't do this, it will crash when
    // calling sem_timedwait. but we don't bother to do
    // this if we are using sem_wait
    memset( psem, 0, sizeof( sem_t ) );

    gint32 ret = 0;
    if( sem_init( psem, pshared, value ) == -1 )
        ret = -errno;

    return ret;
}

int Sem_Timedwait( sem_t* psem, const timespec& ts )
{
    gint32 ret = 0;
    timespec sInterval;

    if (clock_gettime( CLOCK_REALTIME, &sInterval ) == -1)
        return -errno;

    gint32 iCarry = 0;

    if( ts.tv_nsec > 0 )
    {
        auto nsec = sInterval.tv_nsec + ts.tv_nsec;
        if( nsec > NSEC_PER_SEC )
        {
            iCarry = 1;
            nsec -= NSEC_PER_SEC;
        }
        sInterval.tv_nsec = nsec;
    }

    sInterval.tv_sec += ts.tv_sec + iCarry;

    // if( sem_wait( psem ) == -1 )
    if( sem_timedwait( psem, &sInterval ) == -1 )
        ret = -errno;

    return ret;
}

int Sem_TimedwaitSec( sem_t* psem, gint32 iSec )
{
    if( psem == nullptr || iSec <= 0 )
        return -EINVAL;

    timespec ts = { 0 };
    ts.tv_sec = iSec;

    gint32 ret = Sem_Timedwait( psem, ts );

    if( ret == -ETIMEDOUT )
        ret = -EAGAIN;
    else if( ret == -EINTR )
        ret = -EAGAIN;

    return ret;
}

int Sem_Post( sem_t* psem )
{
    if( psem == nullptr )
        return -EINVAL;

    gint32 ret = sem_post( psem );
    if( ret == -1 )
    {
        ret = -errno;
        DebugPrint( ret, 
            "@Thread %d: Sem_Post failed with %s",
            GetTid(), strerror( -ret )
            );
    }

    return ret;
}

int Sem_Wait( sem_t* psem )
{
    gint32 ret = 0;

    if( psem == nullptr )
        return -EINVAL;

    do{
        ret = sem_wait( psem );

        if( ret == -1 && errno == EINTR )
            continue;
        
        if( ret == -1 )
            ret = -errno;

        break;

    }while( 1 );

    return ret;
}

int Sem_Wait_Wakable( sem_t* psem )
{
    gint32 ret = 0;

    if( psem == nullptr )
        return -EINVAL;

    do{
        ret = sem_wait( psem );
        if( ret == -1 )
            ret = -errno;

        break;

    }while( 1 );

    return ret;
}

static guint8 HexCharToByte( const char arrHex[ 2 ] )
{
    guint8 ret = 0;
    const char* p = arrHex;

    if( *p >= '0' && *p <= '9' )
        ret = *p - 0x30;
    else if( *p >= 'A' && *p <= 'F' )
        ret = *p - 0x37;
    else if( *p >= 'a' && *p <= 'f' )
        ret = *p - 0x57;
    
    ret <<= 4;
    p++;

    if( *p >= '0' && *p <= '9' )
        ret |= *p - 0x30;
    else if( *p >= 'A' && *p <= 'F' )
        ret |= *p - 0x37;
    else if( *p >= 'a' && *p <= 'f' )
        ret |= *p - 0x57;
    return ret;
}

gint32 HexStringToBytes(
    const char* szHex,
    guint32 dwSize, guint8* pBuf )
{
    if( ( dwSize & 1 ) == 1 )
        return -EINVAL;

    if( pBuf == nullptr )
        return -EINVAL;

    const char* pSrc = szHex;
    guint8* pDest = pBuf;
    for( size_t i = 0; i < dwSize/2; i++ )
    {
       *pDest++= HexCharToByte( pSrc ); 
       pSrc += 2;
    }
    return 0;
}

/**
* @name ByteToHexChar
* @{ */
/** 
 * pRet must be a ptr to a byte array * of 2 bytes or longer @}
 * byte contains the binary byte to convert
 * the return value is stored in two chars, that is,
 * the higher 4 bits are converted to a hex digit in pRet[ 0 ],
 * and the lower 4 bits are converted to a hex digit in
 * pRet[ 1 ] */

static void BinToHexChar( guint8 byte, guint8* pRet )
{
    if( pRet == nullptr )
        return;

    if( ( byte & 0x0F ) < 0x0A )
        pRet[ 1 ] = ( byte & 0x0F ) + 0x30;
    else
        pRet[ 1 ] = ( byte & 0x0F ) - 9 + 0x40;
    
    if( ( byte & 0xF0 ) < 0xA0 )
        pRet[ 0 ] = ( byte >> 4 ) + 0x30;
    else
        pRet[ 0 ] = ( byte >> 4 ) - 9 + 0x40;

    return;
}


gint32 BytesToString( const guint8* bytes,
    guint32 dwSize, stdstr& strRet )
{
    if( bytes == nullptr
        || dwSize == 0
        || dwSize > 1024 )
    {
        return -EINVAL;
    }

    guint8* pszRet =
        ( guint8* )alloca( dwSize * 2 + 1 );

    pszRet[ dwSize * 2 ] = 0;

    for( guint32 i = 0; i < dwSize * 2; i += 2 )
    {
        BinToHexChar(
            bytes[ ( i >> 1 ) ],
            pszRet + i );
    }

    strRet = ( char* )pszRet;

    return 0;
}

// Generate a sha1 hash and return a string of 40
// characters.
gint32 GenShaHash(
    const char* pBuf, guint32 dwSize,
    std::string& strSess )
{
    SHA1 sha;
    char arrBytes[ 20 ];
    char* pTemp = arrBytes;
    sha.Input( pBuf, dwSize);
    if( !sha.Result( ( guint32* )pTemp ) )
        return ERROR_FAIL;

    guint32* pDwords = ( guint32* )pTemp;
    for( int i = 0; i < 5; i++, pDwords++ )
        *pDwords = htonl( *pDwords );

    gint32 ret = BytesToString( ( guint8* )pTemp,
        sizeof( arrBytes ), strSess );

    return ret;
}

const char* szVersion = "Version: "
    VERSION_STRING "\n" "Features: " FEATURES "\n";

const char* Version()
{ return szVersion; }

CStdRTMutex::CStdRTMutex( stdrtmutex& oMutex ) :
    CMutex< stdrtmutex >( oMutex )
{}

CStdRTMutex::CStdRTMutex( stdrtmutex& oMutex,
    bool bTryLock )
{
    m_pMutex = &oMutex;
    if( !bTryLock )
    {
        Lock();
    }
    else
    {
        bool bLocked = m_pMutex->try_lock();
        if( bLocked )
        {
            m_bLocked = true;
        }
        else
        {
            std::string strMsg = DebugMsg(
                ERROR_FAIL, "Error Lock" );
            throw std::runtime_error( strMsg );
        }
    }
}

CStdRTMutex::CStdRTMutex( stdrtmutex& oMutex,
    guint32 dwTryTimeMs )
{
    m_pMutex = &oMutex;
    if( dwTryTimeMs == 0 )
    {
        Lock();
    }
    else
    {
        bool bLocked = m_pMutex->try_lock_for(
            std::chrono::milliseconds( dwTryTimeMs ) );

        if( bLocked )
        {
            m_bLocked = true;
        }
        else
        {
            std::string strMsg = DebugMsg(
                ERROR_FAIL, "Error Lock" );
            throw std::runtime_error( strMsg );
        }
    }
}

std::atomic<gint32> CObjBase::m_atmObjCount( 0 );

std::atomic< guint64 >
CObjBase::m_atmObjId( GetRandom() );

#ifdef DEBUG
struct cmp_obj
{
    bool operator()( CObjBase *a, CObjBase *b)
    {
        if( b == nullptr )
            return false;

        if( a == nullptr )
            return true;

        return a->GetObjId() < b->GetObjId();
    }
};

struct hash_obj
{
    size_t operator()( CObjBase* a ) const
    { return a->GetObjId(); };
};

stdmutex g_oObjListLock;
std::unordered_set< CObjBase*, hash_obj > g_vecObjs;
void DumpObjs( bool bAll = false)
{
    std::set< CObjBase*, cmp_obj > setObjs;
    {
        CStdMutex oLock( g_oObjListLock );
        for( auto pObj : g_vecObjs ) 
            setObjs.insert( pObj );
    }
    for( auto pObj : setObjs )
    {
        std::string strObj;
        if( pObj != nullptr )
        {
            if( pObj->GetClsid() == clsid( CBuffer ) ||
                pObj->GetClsid() == clsid( CConfigDb2 ) )
                if( !bAll )
                    continue;
            pObj->Dump( strObj );
            printf( "%s\n", strObj.c_str() );
        }
    }
}

// find objects by class id
void DbgFindObjs( gint32 iClsid )
{
    for( auto pObj : g_vecObjs )
    {
        std::string strObj;
        if( pObj != nullptr )
        {
            if( pObj->GetClsid() == iClsid )
                printf( "0x%llx\n",
                    ( LONGWORD )( CObjBase* )pObj );
        }
    }
}
// find objects by class name
void DbgFindCN( const char* szName )
{
    for( auto pObj : g_vecObjs )
    {
        std::string strObj;
        if( pObj != nullptr )
        {
            EnumClsid iClsid = pObj->GetClsid();
            const char* szClass =
                CoGetClassName( iClsid );
            if( szClass == nullptr )
                continue;
            if( strcmp( szClass, szName ) == 0 )
                printf( "0x%llx\n", ( CObjBase* )pObj );
        }
    }
}
#endif

CObjBase::CObjBase()
    : m_atmRefCount( 1 )
{
    m_dwClsid = Clsid_Invalid;
    m_qwObjId = NewObjId();
    ++m_atmObjCount;
    m_dwMagic = *( guint32* )"ObjB";
#ifdef DEBUG
    {
        CStdMutex oLock( g_oObjListLock );
        g_vecObjs.insert( this );
    }
#endif
}

// copy constructor
CObjBase::CObjBase( const CObjBase& rhs )
    : CObjBase()
{
    m_dwClsid = rhs.m_dwClsid;
}

CObjBase::~CObjBase()
{
    --m_atmObjCount;
#ifdef DEBUG
    CStdMutex oLock( g_oObjListLock );
    g_vecObjs.erase( this );
#endif
}

gint32 CObjBase::SetClassId(
    EnumClsid iClsid )
{
    m_dwClsid = iClsid;
    return 0;
}

gint32 CObjBase::AddRef()
{
    return ++m_atmRefCount;
}

gint32 CObjBase::DecRef()
{
    return --m_atmRefCount;  
}

gint32 CObjBase::Release()
{
    gint32 iRef = --m_atmRefCount;
    if( iRef == 0 )
        delete this;
#ifdef DEBUG
    else if( unlikely( iRef < 0 ) )
    {
        std::string strError =
#if BUILD_64
            "Release twice on invalid Object @0x%llx ";
#else
            "Release twice on invalid Object @0x%x ";
#endif
        std::string strMsg = DebugMsg(
            ERROR_FAIL, strError, this );
        throw std::runtime_error( strMsg );
    }
#endif
    return iRef;
}

gint32 CObjBase::GetRefCount()
{
    return m_atmRefCount;
}

EnumClsid CObjBase::GetClsid() const
{
    return m_dwClsid;
}

guint64 CObjBase::GetObjId() const
{
    return m_qwObjId;
}

const char* CObjBase::GetClassName() const
{
    return CoGetClassName( m_dwClsid );
}

gint32 CObjBase::Serialize(
    CBuffer& oBuf ) const
{
    return -ENOTSUP;
}

gint32 CObjBase::Deserialize(
    const char* pBuf, guint32 dwSize )
{
    return -ENOTSUP;
}

gint32 CObjBase::Deserialize(
    const CBuffer& obuf )
{
    return -ENOTSUP;
}

gint32 CObjBase::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    vecProps.push_back( propObjId );
    vecProps.push_back( propRefCount );
    vecProps.push_back( propClsid );
    return 0;
}

gint32 CObjBase::GetProperty(
    gint32 iProp, Variant& oVar ) const
{
    gint32 ret = 0;

    switch( iProp )
    {
    case propObjId:
        oVar = GetObjId();
        break;

    case propRefCount:
        oVar = ( gint32 )m_atmRefCount;
        break;

    case propClsid:
        oVar = GetClsid();
        break;

    default:
        ret = -ENOENT;
    }
    return ret;
}

gint32 CObjBase::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{

    gint32 ret = 0;
    switch( iProp )
    {
    case propObjId:
        iType = typeUInt64;
        break;

    case propClsid:
    case propRefCount:
        iType = typeUInt32;
        break;

    default:
        ret = -ENOENT;
    }
    return ret;
}

gint32 CObjBase::SetProperty(
    gint32 iProp, const Variant& oVar )
{ return -ENOTSUP; }

gint32 CObjBase::RemoveProperty(
    gint32 iProp )
{
    return -ENOTSUP;
}

gint32 CObjBase::Intitialize(
    const IConfigDb* pCfg )
{
    return 0;
}

guint64 CObjBase::NewObjId()
{ return m_atmObjId++; }

#ifdef DEBUG
extern std::unordered_map<guint32, std::string> g_mapId2Name;
void CObjBase::Dump( std::string& strDump )
{
    char szBuf[ 128 ];
#if BUILD_64 == 0
    sprintf( szBuf, "0x%08x: ", ( LONGWORD )this );
#else
    sprintf( szBuf, "0x%08lx: ", ( LONGWORD )this );
#endif
    strDump += szBuf;
    strDump += ", clsid: ";
    if( g_mapId2Name.find( m_dwClsid ) != g_mapId2Name.end() )
    {
        strDump += g_mapId2Name[ m_dwClsid ];
    }
    else
    {
        const char* pszName =
            CoGetClassName( m_dwClsid );
        if( pszName == nullptr )
            strDump += std::to_string( m_dwClsid );
        else
            strDump += pszName;
    }
    strDump += ", refcount: ";
    strDump += std::to_string( m_atmRefCount );
    strDump += ", objid: ";
    strDump += std::to_string( m_qwObjId );
}
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

gint32 Execve(
    const char* cmd,
    char* const args[],
    char* const env[],
    const char* szOutput )
{
    int ret = 0;
    pid_t pid = fork();
    if (pid == -1)
        return ERROR_FAIL;

    else if (pid != 0)
    {
        int status = 0;
        while( ( ret = waitpid( pid, &status, 0 ) ) == -1 )
        {
            if (errno != EINTR) {
                /* Handle error */
                ret = -errno;
                break;
            }
        }
        if ((ret == 0) ||
            !(WIFEXITED(status) && !WEXITSTATUS(status)))
        {
            /* Report unexpected child status */
            ret = -ECHILD;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        if( szOutput != nullptr )
        {
           int fd = open( szOutput,
               O_RDWR | O_CREAT | O_TRUNC,
               S_IRUSR | S_IWUSR); 

           dup2( fd, 1 );
           dup2( fd, 2 );
           close( fd );
        }
        /* ... Initialize env as a sanitized copy of
         * environ ... */
        if( execve(cmd, args, env) == -1 )
        {
            /* Handle error */
            printf( "error running %s(%d)", cmd, errno );
            ret = -errno;
        }
    }
    return ret;
}

CSharedLock::CSharedLock()
{
    Sem_Init( &m_semReader, 0, 0 );
    Sem_Init( &m_semWriter, 0, 0 );
}

CSharedLock::~CSharedLock()
{
    sem_destroy( &m_semReader );
    sem_destroy( &m_semWriter );
}

gint32 CSharedLock::LockRead()
{
    CStdMutex oLock( m_oLock );
    if( !m_bWrite && m_queue.empty() )
    {
        m_iReadCount++;
        return STATUS_SUCCESS;
    }
    m_queue.push_back( true );
    oLock.Unlock();
    return Sem_Wait( &m_semReader );
}

gint32 CSharedLock::LockWrite()
{
    CStdMutex oLock( m_oLock );
    if( m_bWrite || m_queue.size() )
    {
        m_queue.push_back( false );
        oLock.Unlock();
        return Sem_Wait( &m_semWriter );
    }
    else if( m_iReadCount > 0 )
    {
        m_queue.push_back( false );
        oLock.Unlock();
        return Sem_Wait( &m_semWriter );
    }
    else if( m_iReadCount == 0 )
    {
        m_bWrite = true;
        return 0;
    }
    return -EFAULT;
}

gint32 CSharedLock::ReleaseRead()
{
    CStdMutex oLock( m_oLock );
    --m_iReadCount;
    if( m_iReadCount > 0 )
        return STATUS_SUCCESS;

    if( unlikely( m_iReadCount < 0 ) )
        return -EFAULT;

    if( m_queue.empty() )
        return STATUS_SUCCESS;

    if( m_queue.front() == true )
        return -EFAULT;

    m_bWrite = true;
    m_queue.pop_front();
    return Sem_Post( &m_semWriter );
}

gint32 CSharedLock::ReleaseWrite()
{
    CStdMutex oLock( m_oLock );
    if( m_queue.empty() )
    {
        m_bWrite = false;
    }
    else if( m_queue.front() == false )
    {
        m_bWrite = true;
        m_queue.pop_front();
        Sem_Post( &m_semWriter );
    }
    else
    {
        m_bWrite = false;
        while( m_queue.front() == true )
        {
            m_iReadCount++;
            m_queue.pop_front();
            Sem_Post( &m_semReader );
            if( m_queue.empty() )
                break;
        }
    }
    return STATUS_SUCCESS;
}

gint32 CSharedLock::TryLockRead()
{
    CStdMutex oLock( m_oLock );
    if( !m_bWrite && m_queue.empty() )
    {
        m_iReadCount++;
        return STATUS_SUCCESS;
    }
    return -EAGAIN;
}

gint32 CSharedLock::TryLockWrite()
{
    CStdMutex oLock( m_oLock );
    if( m_bWrite || m_queue.size() ) 
        return -EAGAIN;

    if( m_iReadCount == 0 )
    {
        m_bWrite = true;
        return 0;
    }
    return -EFAULT;
}

#include <pwd.h>
stdstr GetHomeDir()
{
    struct passwd* pwd = getpwuid( getuid() );
    return stdstr( pwd->pw_dir );
}


guint64 GetVmSize()
{
    std::ifstream oFile("/proc/self/status");
    stdstr strLine;
    guint64 qwVmSizeKb = 0;

    while( std::getline( oFile, strLine ) )
    {
        if (strLine.substr(0, 7) == "VmSize:")
        {
            std::istringstream iss(strLine.substr(7));
            iss >> qwVmSizeKb;
            break;
        }
    }
    return qwVmSizeKb;
}

struct CPUData {
    guint64 user = 0;
    guint64 nice = 0;
    guint64 system = 0;
    guint64 idle = 0;
    guint64 iowait = 0;
    guint64 irq = 0;
    guint64 softirq = 0;
    guint64 steal = 0;
    guint64 guest = 0;
    guint64 guest_nice = 0;
};

static CPUData GetCPUData() {
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);
    std::string cpu;
    CPUData data;
    iss >> cpu >> data.user >>
        data.nice >> data.system >>
        data.idle >> data.iowait >>
        data.irq >> data.softirq >>
        data.steal >> data.guest >>
        data.guest_nice;
    return data;
}

static CPUData GetProcessCPUData(int pid) {
    std::ifstream file(
        "/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);
    CPUData data;
    std::string temp;
    for (int i = 1; i <= 13; ++i) {
        iss >> temp;
    }
    iss >> data.user >> data.system;
    return data;
}

static float CalculateCpuUsage(
    CPUData prev_cpu, CPUData current_cpu,
    CPUData prev_process,
    CPUData current_process)
{
    guint64 total_diff =
        (current_cpu.user - prev_cpu.user) +
        (current_cpu.nice - prev_cpu.nice) +
        (current_cpu.system - prev_cpu.system) +
        (current_cpu.idle - prev_cpu.idle) +
        (current_cpu.iowait - prev_cpu.iowait) +
        (current_cpu.irq - prev_cpu.irq) +
        (current_cpu.softirq - prev_cpu.softirq) +
        (current_cpu.steal - prev_cpu.steal) +
        (current_cpu.guest - prev_cpu.guest) +
        (current_cpu.guest_nice - prev_cpu.guest_nice);

    guint64 process_diff =
        (current_process.user - prev_process.user) +
        (current_process.system - prev_process.system);

    if (total_diff == 0)
        return 0.0f;

    return (float)process_diff * 100.0f / (float)total_diff;
}

float GetCpuUsage()
{
    static CPUData oPrevData;
    static CPUData oPrevProcData;
    CPUData oCurData = GetCPUData();
    CPUData oCurProcData =
        GetProcessCPUData( getpid() );
    float ret = CalculateCpuUsage(
        oPrevData, oCurData,
        oPrevProcData, oCurProcData);
    oPrevData = oCurData;
    oPrevProcData = oCurProcData;
    return ret;
}

gint32 GetOpenFileCount(
    gint32 pid, guint32& dwCount )
{
    std::stringstream oPath;
    oPath << "/proc/" << pid << "/fd";
    std::string strPath = oPath.str();

    DIR *dir = opendir( strPath.c_str() );
    if (!dir)
        return -errno;

    dwCount = 0;
    struct dirent *entry;
    while( ( entry = readdir(dir) ) != nullptr )
    {
        if (entry->d_type == DT_REG ||
            entry->d_type == DT_LNK)
            dwCount++;
    }
    closedir( dir );
    return 0;
}

CProcessLock::CProcessLock(
    const stdstr& strLockPath,
    bool bRemove )
{
    gint32 ret = 0;
    do{
        m_bRemove = bRemove;
        m_strLockFile = strLockPath + ".lock";
        m_iFd = open( m_strLockFile.c_str(),
            O_CREAT | O_RDWR, 0600);
        if( m_iFd < 0 )
        {
            OutputMsg( errno,
                "Failed to open lock file: %s",
                m_strLockFile.c_str());
            ret = -errno;
            break;
        }
        struct flock lock;
        memset(&lock, 0, sizeof(lock));
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
        ret = fcntl( m_iFd, F_SETLKW, &lock );
        if( ret < 0 )
        {
            OutputMsg(errno,
                "Failed to acquire lock on file: %s",
                m_strLockFile.c_str());
            ret = -errno;
            break;
        }
    }while( 0 );
    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error in CProcessLock ctor" );
        throw std::runtime_error( strMsg );
    }
}

CProcessLock::~CProcessLock()
{
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_UNLCK;
    fcntl( m_iFd, F_SETLK, &lock );
    close( m_iFd );
    m_iFd = -1;
    if( m_bRemove )
        unlink( m_strLockFile.c_str() ); 
}

stdstr SimpleCmdLine(
    int argc, char** argv )
{
    stdstr strCmd;
    for (int i = 0; i < argc; ++i)
    {
        strCmd += argv[i];
        if (i < argc - 1)
            strCmd += " ";
    }
    return strCmd;
}

}

