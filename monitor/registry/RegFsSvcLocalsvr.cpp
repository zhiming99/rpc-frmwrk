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
{ return ERROR_NOT_IMPL; }

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
    return ret;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::SetValue(
    const std::string& strPath /*[ In ]*/,
    const std::string& strJson /*[ In ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Chmod(
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Chown(
    const std::string& strPath /*[ In ]*/,
    guint32 dwUid /*[ In ]*/,
    guint32 dwGid /*[ In ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::ReadLink(
    const std::string& strPath /*[ In ]*/,
    BufPtr& buf /*[ Out ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Rename(
    const std::string& szFrom /*[ In ]*/,
    const std::string& szTo /*[ In ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Flush(
    guint32 dwFlags /*[ In ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::Access(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::GetAttr(
    const std::string& strPath /*[ In ]*/,
    FileStat& oStat /*[ Out ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::ReadDir(
    guint64 hDir /*[ In ]*/,
    std::vector<FileStat>& vecDirEnt /*[ Out ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}

/* Sync Req Handler*/
gint32 CRegFsSvcLocal_SvrImpl::OpenDir(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/,
    guint64& hDir /*[ Out ]*/ )
{
    // TODO: Process the sync request here 
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
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
