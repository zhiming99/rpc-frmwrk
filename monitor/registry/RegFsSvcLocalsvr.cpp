// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -O . -I ../registry -l regfs.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
#include "RegistryFs.h"
#include "RegFsSvcLocalsvr.h"

namespace rpcf
{

gint32 VarToJson( const Variant& oVar,
    stdstr& strJson )
{
    gint32 ret = 0;
    Json::StreamWriterBuilder oBuilder;
    oBuilder["commentStyle"] = "None";
    Json::Value oVal( Json::objectValue );
    oVal[ "t" ] = oVar.GetTypeId();
    char buf[ 64 ];
    switch( oVar.GetTypeId() )
    {
    case typeByte:
        {
            snprintf( buf, sizeof( buf ),
                "%d", oVar.m_byVal );
            oVal[ "v" ] = buf;
            break;
        }
    case typeUInt16:
        {
            snprintf( buf, sizeof( buf ),
                "%d", oVar.m_wVal );
            oVal[ "v" ] = buf;
            break;
        }
    case typeUInt32:
        {
            snprintf( buf, sizeof( buf ),
                "%d", oVar.m_dwVal );
            oVal[ "v" ] = buf;
            break;
        }
    case typeUInt64:
        {
            snprintf( buf, sizeof( buf ),
                "%lld", oVar.m_qwVal );
            oVal[ "v" ] = buf;
            break;
        }
    case typeFloat:
        {
            snprintf( buf, sizeof( buf ),
                "%.7f", oVar.m_fVal );
            oVal[ "v" ] = buf;
            break;
        }
    case typeDouble:
        {
            snprintf( buf, sizeof( buf ),
                "%.15g", oVar.m_dblVal );
            oVal[ "v" ] = buf;
            break;
        }
    case typeString:
        {
            oVal[ "v" ] = oVar.m_strVal;
            break;
        }
    case typeNone:
        break;
    case typeDMsg:
    case typeObj:
    case typeByteArr:
    default:
        ret = -ENOTSUP;
        break;
    }
    if( ERROR( ret ) )
        return ret;
    strJson = Json::writeString(
            oBuilder, oVal );
    return ret;
}

gint32 JsonToVar( const stdstr& strJson,
    Variant& oVar )
{
    gint32 ret = 0;
    do{
        Json::CharReaderBuilder oBuilder;
        Json::CharReader* pReader =
            oBuilder.newCharReader();
        Json::Value oVal;
        if( !pReader->parse( strJson.c_str(),
            strJson.c_str() + strJson.size(),
            &oVal, nullptr ) )
        {
            ret = -EINVAL;
            break;
        }
        if( !oVal.isMember( "t" ) )
        {
            ret = -EINVAL;
            break;
        }
        if( !oVal.isMember( "v" ) )
        {
            oVar.m_iType = typeNone;
            break;
        }

        guint32 dwType = oVal[ "t" ].asInt();
        switch( ( EnumTypeId )dwType )
        {
        case typeByte:
            {
                oVar.m_byVal =
                    ( guint8 )oVal[ "v" ].asUInt();
                break;
            }
        case typeUInt16:
            {
                oVar.m_wVal =
                    ( guint16 )oVal[ "v" ].asUInt();
                break;
            }
        case typeUInt32:
            {
                oVar.m_dwVal =
                    oVal[ "v" ].asUInt();
                break;
            }
        case typeUInt64:
            {
                oVar.m_qwVal =
                    oVal[ "v" ].asUInt64();
                break;
            }
        case typeFloat:
            {
                oVar.m_qwVal =
                    oVal[ "v" ].asFloat();
                break;
            }
        case typeDouble:
            {
                oVar.m_qwVal =
                    oVal[ "v" ].asDouble();
                break;
            }
        case typeString:
            {
                oVar.m_strVal =
                    oVal[ "v" ].asString();
                break;
            }
        case typeDMsg:
        case typeObj:
        case typeByteArr:
        default:
            ret = -ENOTSUP;
            break;
        }
        if( ERROR( ret ) )
            break;
    }while( 0 );
    return ret;
}

CRegFsSvcLocal_SvrImpl::CRegFsSvcLocal_SvrImpl(
    const IConfigDb* pCfg ) :
    super::virtbase( pCfg ), super( pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid(CRegFsSvcLocal_SvrImpl) );
        CParamList oParams;
        ret = oParams.CopyProp(
            propConfigPath, pCfg );
        if( ERROR( ret ) )
            break;

        oParams.CopyProp( 0, pCfg );

        oParams.SetPointer(
            propIoMgr, this->GetIoMgr() );
        ret = m_pRegFs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
    }while( 0 );
    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error in CRegFsSvcLocal_SvrImpl ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRegFsSvcLocal_SvrImpl::OnPostStart(
    IEventSink* pCallback )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    gint32 ret = m_pRegFs->Start();
    if( ERROR( ret ) )
        return ret;
    StartQpsTask();
    return super::OnPostStart( pCallback );
}

gint32 CRegFsSvcLocal_SvrImpl::OnPreStop(
    IEventSink* pCallback )
{
    if( !m_pRegFs.IsEmpty() )
        m_pRegFs->Stop();
    StopQpsTask();
    return super::OnPreStop( pCallback );
}

gint32 CRegFsSvcLocal_SvrImpl::InvokeUserMethod(
    IConfigDb* pParams,
    IEventSink* pCallback )
{
    gint32 ret = AllocReqToken();
    if( ERROR( ret ) )
        return ret;
    return super::InvokeUserMethod(
        pParams, pCallback );
}


// IRegFsLocal Server
/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::CreateFile(
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/,
    guint64& hFile /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->CreateFile(
        strPath, dwMode, dwFlags, hFile );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::MakeDir(
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->MakeDir(
        strPath, dwMode );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::OpenFile(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/,
    guint64& hFile /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->OpenFile(
        strPath, dwFlags, hFile );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::CloseFile(
    guint64 hFile /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->CloseFile( hFile );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::RemoveFile(
    const std::string& strPath /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->RemoveFile( strPath );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::ReadFile(
    guint64 hFile /*[ In ]*/,
    guint32 dwSize /*[ In ]*/,
    guint32 dwOff /*[ In ]*/,
    BufPtr& buffer /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    if( dwSize == 0  ||
        dwOff >= MAX_FILE_SIZE )
        return -EINVAL;

    gint32 ret = 0;
    BufPtr pBuf( true );
    ret = pBuf->Resize( std::min( dwSize,
        MAX_FILE_SIZE - dwOff ) );
    if( ERROR( ret ) )
        return ret;

    dwSize = pBuf->size();
    ret = m_pRegFs->ReadFile( hFile,
        pBuf->ptr(), dwSize, dwOff );
    if( ERROR( ret ) )
        return ret;
    buffer = pBuf;
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::WriteFile(
    guint64 hFile /*[ In ]*/,
    BufPtr& pBuf /*[ In ]*/,
    guint32 dwOff /*[ In ]*/,
    guint32& dwSizeWrite /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    if( pBuf.IsEmpty() ||
        pBuf->size() == 0 ||
        dwOff >= MAX_FILE_SIZE )
        return -EINVAL;

    gint32 ret = 0;
    guint32 dwSize = std::min( pBuf->size(),
        MAX_FILE_SIZE - dwOff );
    ret = m_pRegFs->WriteFile( hFile,
        pBuf->ptr(), dwSize, dwOff );
    if( ERROR( ret ) )
        return ret;
    dwSizeWrite = dwSize;
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Truncate(
    guint64 hFile /*[ In ]*/,
    guint32 dwOff /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    if( dwOff >= MAX_FILE_SIZE )
        return -EINVAL;
    return m_pRegFs->Truncate( hFile, dwOff );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::CloseDir(
    guint64 hFile /*[ In ]*/ )
{ 
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->CloseFile( hFile );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::RemoveDir(
    const std::string& strPath /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    if( strPath.empty() )
        return -EINVAL;
    return m_pRegFs->RemoveDir( strPath );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::SetGid(
    const std::string& strPath /*[ In ]*/,
    guint32 wGid /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->SetGid( strPath, wGid );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::SetUid(
    const std::string& strPath /*[ In ]*/,
    guint32 wUid /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->SetUid( strPath, wUid );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::GetGid(
    const std::string& strPath /*[ In ]*/,
    guint32& gid /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->GetGid( strPath, gid );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::GetUid(
    const std::string& strPath /*[ In ]*/,
    guint32& uid /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->GetUid( strPath, uid );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::SymLink(
    const std::string& strSrcPath /*[ In ]*/,
    const std::string& strDestPath /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->SymLink(
        strSrcPath, strDestPath );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::GetValue(
    const std::string& strPath /*[ In ]*/,
    std::string& strJson /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    Variant oVar;
    gint32 ret = m_pRegFs->GetValue(
        strPath, oVar );
    if( ERROR( ret ) )
        return ret;
    ret = VarToJson( oVar, strJson );
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::SetValue(
    const std::string& strPath /*[ In ]*/,
    const std::string& strJson /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    Variant oVar;
    gint32 ret = JsonToVar( strJson, oVar );
    if( ERROR( ret ) )
        return ret;
    ret = m_pRegFs->GetValue( strPath, oVar );
    if( ERROR( ret ) )
        return ret;
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Chmod(
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->Chmod( strPath, dwMode );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Chown(
    const std::string& strPath /*[ In ]*/,
    guint32 dwUid /*[ In ]*/,
    guint32 dwGid /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->Chown(
        strPath, dwUid, dwGid );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::ReadLink(
    const std::string& strPath /*[ In ]*/,
    BufPtr& buf /*[ Out ]*/ )
{
    gint32 ret = 0;
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    do{
        struct stat stBuf;
        ret = m_pRegFs->GetAttr( strPath, stBuf );
        if( ERROR( ret ) )
            break;
        if( stBuf.st_size == 0 ||
            stBuf.st_size > MAX_FILE_SIZE )
        {
            ret = ERROR_FAIL;
            break;
        }
        BufPtr pBuf( true );
        ret = pBuf->Resize( stBuf.st_size );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = pBuf->size();
        ret = m_pRegFs->ReadLink(
            strPath, pBuf->ptr(), dwSize );
        if( dwSize < pBuf->size() )
            pBuf->Resize( dwSize );
        buf = pBuf;
    }while( 0 );
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Rename(
    const std::string& strFrom /*[ In ]*/,
    const std::string& strTo /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->Rename( strFrom, strTo );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Flush(
    guint32 dwFlags /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->Flush( FLAG_FLUSH_CHILD );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Access(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    return m_pRegFs->Access( strPath, dwFlags );
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::GetAttr(
    const std::string& strPath /*[ In ]*/,
    FileStat& oStat /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    struct stat stBuf;
    gint32 ret = m_pRegFs->GetAttr(
        strPath, stBuf ); 
    if( ERROR( ret ) )
        return ret;
    oStat.st_dev = stBuf.st_dev;
    oStat.st_ino = stBuf.st_ino;
    oStat.st_mode = stBuf.st_mode;
    oStat.st_nlink = stBuf.st_nlink;
    oStat.st_uid = stBuf.st_uid;
    oStat.st_gid = stBuf.st_gid;
    oStat.st_rdev = stBuf.st_rdev;
    oStat.st_size = stBuf.st_size;
    oStat.st_blksize = stBuf.st_blksize;
    oStat.st_blocks = stBuf.st_blocks;

    oStat.st_atim.tv_sec = stBuf.st_atim.tv_sec;
    oStat.st_atim.tv_nsec = stBuf.st_atim.tv_nsec;
    oStat.st_mtim.tv_sec = stBuf.st_mtim.tv_sec;
    oStat.st_mtim.tv_nsec = stBuf.st_mtim.tv_nsec;
    oStat.st_ctim.tv_sec = stBuf.st_ctim.tv_sec;
    oStat.st_ctim.tv_nsec = stBuf.st_ctim.tv_nsec;
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::ReadDir(
    guint64 hDir /*[ In ]*/,
    std::vector<FileStat>& vecDirEnt /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    gint32 ret = 0;
    std::vector< KEYPTR_SLOT > vecEnt;
    ret = m_pRegFs->ReadDir( hDir, vecEnt );
    if( ERROR( ret ) )
        return ret;
    vecDirEnt.resize( vecEnt.size() );
    for( int i = 0; i < vecEnt.size(); i++ )
    {
        FileStat& st = vecDirEnt[ i ];
        auto& elem = vecEnt[ i ];
        st.st_ino = elem.oLeaf.dwInodeIdx;
        if( elem.oLeaf.byFileType == ftRegular )
            st.st_mode = S_IFREG;
        else if( elem.oLeaf.byFileType == ftDirectory )
            st.st_mode = S_IFDIR;
        else if( elem.oLeaf.byFileType == ftLink )
            st.st_mode = S_IFLNK;
        guint32 dwLen = strnlen(
            elem.szKey, sizeof( elem.szKey ) );
        st.st_name.append( elem.szKey, dwLen );    
    }
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::OpenDir(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/,
    guint64& hDir /*[ Out ]*/ )
{
    if( m_pRegFs.IsEmpty() )
        return -EFAULT;
    RFHANDLE hFile = INVALID_HANDLE;
    gint32 ret = m_pRegFs->OpenDir(
        strPath, dwFlags, hFile );
    if( ERROR( ret ) )
        return ret;
    hDir = hFile;
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::ExecBat(
    const std::string& strJson /*[ In ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

}
