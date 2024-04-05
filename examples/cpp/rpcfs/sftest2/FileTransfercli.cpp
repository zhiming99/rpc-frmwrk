/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -s -O . ../../../sftest.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "sftest.h"
#include "FileTransfercli.h"

gint32 CFileTransfer_CliImpl::CreateStmSkel(
    InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = false;
        oCfg.SetPointer( propParentPtr, this );
        oCfg.CopyProp( propSkelCtx, this );
        std::string strDesc;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;
        ret = CRpcServices::LoadObjDesc(
            strDesc,"FileTransfer_SvrSkel",
            false, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj(
            clsid( CFileTransfer_CliSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CFileTransfer_CliImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CFileTransfer_ChannelCli );
        oCtx.CopyProp( propObjDescPath, this );
        oCtx.CopyProp( propSvrInstName, this );
        stdstr strInstName;
        ret = oIfCfg.GetStrProp(
            propObjName, strInstName );
        if( ERROR( ret ) )
            break;
        oCtx[ 1 ] = strInstName;
        guint32 dwHash = 0;
        ret = GenStrHash( strInstName, dwHash );
        if( ERROR( ret ) )
            break;
        char szBuf[ 16 ];
        sprintf( szBuf, "_%08X", dwHash );
        strInstName = "FileTransfer_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = false;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}

gint32 CFileTransfer_CliImpl::UploadFile(
    HANDLE hChannel, const stdstr& strFile )
{
    if( hChannel == INVALID_HANDLE ||
        strFile.empty() )
        return -EINVAL;

    if( !IsFileExist( strFile ) )
        return -ENOENT;

    gint32 ret = 0;
    do{
        TransFileContext o( strFile );
        o.m_cDirection = 'u';
        o.m_strPath = strFile;
        o.m_bServer = false;
        ret = o.OpenFile();
        if( ERROR( ret ) )
            break;
        m_oTransCtx.AddContext( hChannel, o );
        BufPtr pBuf( true );
        *pBuf = strFile;
        stdstr strName = basename( pBuf->ptr() );

        ret = this->StartUpload(
            strName, hChannel, 0, o.m_iSize );
        if( ERROR( ret ) )
            break;

        ret = m_oTransCtx.ReadFileAndSend(
            hChannel );
    }while( 0 );

    return ret; 
}

gint32 CFileTransfer_CliImpl::DownloadFile(
    HANDLE hChannel, const stdstr& strFile,
    guint64 qwOffset, guint64 qwSize  )
{
    if( hChannel == INVALID_HANDLE ||
        strFile.empty() )
        return -EINVAL;

    if( !IsFileNameValid( strFile ) )
        return - EINVAL;

    gint32 ret = 0;
    do{
        TransFileContext o( strFile );
        o.m_cDirection = 'd';
        o.m_strPath = "./" + strFile + ".1";
        o.m_bServer = false;
        o.m_iOffset = qwOffset;
        o.m_iSize = qwSize;

        ret = o.OpenFile();
        if( ERROR( ret ) )
            break;
        ret = m_oTransCtx.AddContext(
            hChannel, o );
        if( ERROR( ret ) )
            break;

        ret = StartDownload( strFile,
            hChannel, qwOffset, qwSize );
        if( ERROR( ret ) )
            break;
        BufPtr pBuf;
        ret = m_oTransCtx.WriteFileAndRecv(
            hChannel, pBuf );

    }while( 0 );
    return ret;
}
