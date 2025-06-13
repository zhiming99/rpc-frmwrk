/*
 * =====================================================================================
 *
 *       Filename:  saproxy.cpp
 *
 *    Description:  Implementation of SimpAuth proxy for non-js client login
 *
 *        Version:  1.0
 *        Created:  05/14/2025 10:29:55 AM
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
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <pwd.h>
#include "rpc.h"
using namespace rpcf;
#include "ifhelper.h"
#include "security.h"
#include "oa2proxy.h"
#include "saproxy.h"
#include "blkalloc.h"
#include "encdec.h"

#define SIMPAUTH_DIR "simpleauth"

namespace rpcf 
{

CSimpAuthLoginProxy::CSimpAuthLoginProxy(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        CRpcServices* pSvc = nullptr;

        ret = oCfg.GetPointer(
            propRouterPtr, pSvc );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }
        m_pRouter = ObjPtr( pSvc );

        CCfgOpenerObj oIfCfg( this );
        oIfCfg.RemoveProperty(
            propRouterPtr );

    }while( 0 );
    return;
}

gint32 CSimpAuthLoginProxy::RebuildMatches()
{
    // this interface does not I/O normally
    CCfgOpenerObj oIfCfg( this );
    if( m_vecMatches.empty() )
        return -ENOENT;

    gint32 ret = super::RebuildMatches();
    if( ERROR( ret ) )
        return ret;

    MatchPtr pMatch;
    for( auto& elem : m_vecMatches )
    {
        CCfgOpenerObj oMatch(
            ( CObjBase* )elem );

        std::string strIfName;

        ret = oMatch.GetStrProp(
            propIfName, strIfName );

        if( ERROR( ret ) )
            continue;

        if( DBUS_IF_NAME( "IAuthenticate" )
            == strIfName )
        {
            pMatch = elem;
            break;
        }
    }

    if( !pMatch.IsEmpty() )
    {
        // overwrite the destination info
        oIfCfg.CopyProp(
            propDestDBusName, pMatch );

        oIfCfg.CopyProp(
            propObjPath, pMatch );
    }

    return 0;
}

gint32 CSimpAuthLoginProxy::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( IAuthenticate, false );

    ADD_PROXY_METHOD_ASYNC( 1,
        CSimpAuthLoginProxy::Login,
        SYS_METHOD( AUTH_METHOD_LOGIN ) );

    ADD_PROXY_METHOD_ASYNC( 1,
        CSimpAuthLoginProxy::MechSpecReq,
        SYS_METHOD( AUTH_METHOD_MECHSPECREQ ) );

    END_IFPROXY_MAP;
    return 0;
}

gint32 CSimpAuthLoginProxy::Login(
    IEventSink* pCallback,
    IConfigDb* pInfo, /*[ in ]*/
    CfgPtr& pResp ) /*[ out ]*/
{
    if( pInfo == nullptr )
        return -EINVAL;

    CCfgOpener oOptions;
    oOptions.SetStrProp( propIfName,
        DBUS_IF_NAME( "IAuthenticate" ) );
    oOptions.SetBoolProp( propSysMethod, true );
    return this->AsyncCall( pCallback,
        oOptions.GetCfg(), pResp,
        AUTH_METHOD_LOGIN, pInfo );
}

gint32 CSimpAuthLoginProxy::DoLogin(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        TaskletPtr pDeferTask;

        CfgPtr pResp;
        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pDeferTask, ObjPtr( this ),
            &CSimpAuthLoginProxy::StartLogin,
            nullptr );

        if( ERROR( ret ) )
            break;

        CIfRetryTask* pRetryTask = pDeferTask;
        if( pRetryTask == nullptr )
        {
            ( *pDeferTask )( eventCancelTask );
            ret = -EFAULT;
            break;
        }

        pRetryTask->SetClientNotify( pCallback );

        // run this task on a standalone thread
        CIoManager* pMgr = GetIoMgr();
        ret = pMgr->RescheduleTask(
            pDeferTask, false );

        if( ERROR( ret ) )
        {
            ( *pDeferTask )( eventCancelTask );
            break;
        }

        ret = STATUS_PENDING;

    }while( 0 );
    return ret;
}

static gint32 ReadRegFile( RegFsPtr& pfs,
    const stdstr& strPath, BufPtr& pData )
{
    RFHANDLE hFile = INVALID_HANDLE;
    gint32 ret = 0;
    do{
        ret = pfs->OpenFile(
            strPath, O_RDONLY, hFile );
        if( ERROR( ret ) )
            break;
        struct stat st;
        ret = pfs->GetAttr( hFile, st );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = st.st_size;
        if( dwSize == 0 )
            break;
        BufPtr pBuf( true );
        pBuf->Resize( dwSize );
        ret = pfs->ReadFile(
            hFile, pBuf->ptr(), dwSize, 0 );
        if( ERROR( ret ) )
            break;
        pData = pBuf;
    }while( 0 );
    if( hFile != INVALID_HANDLE )
        pfs->CloseFile( hFile );
    return ret;
}

static gint32 GetDefaultUser(
    RegFsPtr& pfs, stdstr& strDefault )
{
    RFHANDLE hFile = INVALID_HANDLE;
    gint32 ret = 0;

    const stdstr strDefCred = "/" SIMPAUTH_DIR
        "/credentials/.default";
    do{
        BufPtr pBuf;
        ret = ReadRegFile(
            pfs, strDefCred, pBuf );
        if( ERROR( ret ) )
            break;
        if( pBuf.IsEmpty() || pBuf->empty() )
        {
            ret = -ENODATA;
            break;
        }
        strDefault.append(
            pBuf->ptr(), pBuf->size() );
    }while( 0 );
    return ret;
}

static std::string GetClientRegPath()
{
    stdstr strCliReg = GetHomeDir();
    strCliReg += "/.rpcf/clientreg.dat";
    return strCliReg;
}

gint32 GetPassHash( stdstr& strUser,
    BufPtr& pPassHash,
    const stdstr& strSalt,
    bool& bGmSSL )
{
    gint32 ret = 0;
    RegFsPtr pfs;
    stdstr strCliReg = GetClientRegPath();

    try{
        CProcessLock oProcLock( strCliReg );
        do{
            const stdstr strCredDir =
                "/" SIMPAUTH_DIR "/credentials";

            CParamList oParams;
            oParams.SetStrProp(
                propConfigPath, strCliReg );
            ret = pfs.NewObj(
                clsid( CRegistryFs ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = pfs->Start();
            if( ERROR( ret ) )
            {
                OutputMsg( ret, "Error start the registry "
                "and you may want to format the registry "
                "with command 'regfsmnt -i %s' first",
                strCliReg.c_str() );
                break;
            }

            if( strUser.empty() )
            {
                ret = GetDefaultUser( pfs, strUser );
                if( ERROR( ret ) )
                {
                    OutputMsg( ret, "Error cannot find "
                        "default user to login with" );
                    break;
                }
            }
            stdstr strPath =
                strCredDir + "/" + strUser;

            BufPtr pEncKey;
            ret = ReadRegFile( pfs,
                strPath + ".cred", pEncKey );

            if( ERROR( ret ) )
            {
                OutputMsg( ret,
                    "Error read the key file %s",
                    ( strPath + ".cred" ).c_str() );
                break;
            }

            BufPtr pEmpty;
            ret = ReadRegFile( pfs,
                strPath + ".gmssl", pEmpty );
            if( SUCCEEDED( ret ) )
                bGmSSL = true;
            else
                bGmSSL = false;

            BufPtr pTextHash;
            ret = DecryptWithPrivKey(
                pEncKey, pTextHash, bGmSSL );
            if( ERROR( ret ) )
            {
                OutputMsg( ret,
                    "Error decrypt the key file %s",
                    ( strPath + ".cred" ).c_str() );
                break;
            }

            stdstr strHash( pTextHash->ptr(),
                pTextHash->size() );

            strHash += strSalt;
            stdstr strKey;
            ret = GenSha2Hash(
                strHash, strKey, bGmSSL );
            if( ERROR( ret ) )
                break;

            BufPtr pBinHash( true );
            ret = pBinHash->Resize(
                strKey.size() / 2 );
            if( ERROR( ret ) )
                break;
            ret = HexStringToBytes(
                strKey.c_str(), strKey.size(),
                ( guint8* )pBinHash->ptr() );
            if( ERROR( ret ) )
                break;
            if( pBinHash->size() != 32 )
            {
                ret = -EINVAL;
                break;
            }
            pPassHash = pBinHash;

        }while( 0 );
        if( !pfs.IsEmpty() )
            pfs->Stop();
    }
    catch( std::runtime_error& e )
    {
        if( ret >= 0 )
            ret = -EFAULT;
    }
    return ret;
}

gint32 CSimpAuthLoginProxy::BuildLoginInfo(
    const stdstr& strUser, CfgPtr& pInfo )
{
    gint32 ret = 0;
    do{
        Variant oVar;
        ret = this->GetProperty(
            propConnHandle, oVar );
        if( ERROR( ret ) )
            break;
        guint32 dwPortId = oVar;
        InterfPtr pIf;
        ret = m_pRouter->GetBridgeProxy(
            dwPortId, pIf );
        if( ERROR( ret ) )
            break;
        ret = pIf->GetProperty(
            propSessHash, oVar );
        if( ERROR( ret ) )
            break;

        BufPtr pKey;
        bool bGmSSL = false;
        stdstr strUser2 = strUser;
        ret = GetPassHash(
            strUser2, pKey, oVar, bGmSSL );
        if( ERROR( ret ) )
            break;

        CRpcTcpBridgeProxy* pbp = pIf;
        guint64 qwTs = pbp->GetPeerStartSec();

        CCfgOpener oLogin;
        oLogin.SetProperty( propSessHash, oVar );

        oLogin.SetStrProp(
            propUserName, strUser2 );

        oLogin.SetQwordProp(
            propTimestamp, qwTs );

        BufPtr pToken( true );
        ret = oLogin.Serialize( pToken );
        if( ERROR( ret ) )
            break;

        BufPtr pEncrypted;
        ret = EncryptAesGcmBlock( pToken,
            pKey, pEncrypted, bGmSSL );

        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error encrypting the token" );
            break;
        }
        CParamList oInfo( ( IConfigDb* )pInfo );
        oInfo.SetStrProp( propUserName, strUser2 );
        oInfo.Push( pEncrypted );
        oInfo.SetBoolProp( propGmSSL, bGmSSL );

        CCfgOpener oCtx;
        oCtx.SetStrProp( propUserName, strUser2 );
        oCtx.SetProperty( propSessHash, oVar );
        oCtx.SetQwordProp( propTimestamp, qwTs );
        oInfo.SetPointer( propContext,
            ( IConfigDb* )oCtx.GetCfg() );

    }while( 0 );
    return ret;
}

static gint32 CheckServerToken( IConfigDb* pReq,
    BufPtr pSvrToken, bool bGmSSL )
{
    gint32 ret = 0;
    if( pReq == nullptr || pSvrToken.IsEmpty() )
        return -EINVAL;
    do{
        CCfgOpener oReq( pReq );
        stdstr strUser;
        ret = oReq.GetStrProp(
            propUserName, strUser );
        if( ERROR( ret ) )
            break;

        stdstr strSess;
        ret = oReq.GetStrProp(
            propSessHash, strSess );
        if( ERROR( ret ) )
            break;

        guint64 qwTs; 
        ret = oReq.GetQwordProp(
            propTimestamp, qwTs );
        if( ERROR( ret ) )
            break;

        qwTs = htonll( qwTs + 1 );
        BufPtr pOrig( true );
        pOrig->Append(
            ( guint8* )&qwTs, sizeof( qwTs ) );
        pOrig->Append(
            strUser.c_str(), strUser.size() );
        pOrig->Append(
            strSess.c_str(), strSess.size() );

        bool bGmSSL2 = false;
        BufPtr pPass;
        ret = GetPassHash( strUser,
            pPass, strSess, bGmSSL2 );
        if( ERROR( ret ) )
            break;

        BufPtr pText;
        ret = DecryptAesGcmBlock(
            pPass, pSvrToken, pText, bGmSSL );
        if( ERROR( ret ) )
            break;

        if( pText->size() != pOrig->size() )
        {
            ret = -EACCES;
            break;
        }
        ret = memcmp( pText->ptr(),
            pOrig->ptr(), pText->size() );
        if( ret != 0 )
            ret = -EACCES;

    }while( 0 );
    return ret;
}

gint32 CSimpAuthLoginProxy::SimpAuthLogin(
    IEventSink* pCallback,
    IConfigDb* pInfo )
{
    gint32 ret = 0;
    do{
        CCfgOpener oResp;
        gint32 (*func)(CTasklet*, IEventSink*,
            CSimpAuthLoginProxy*, IConfigDb* ) =
        ([]( CTasklet* pCb,
            IEventSink* pLoginCb,
            CSimpAuthLoginProxy* pIf,
            IConfigDb* pReq )->gint32
        {
            if( pCb == nullptr || pReq == nullptr )
                return -EINVAL;

            gint32 ret = 0;
            do{
                CCfgOpener oCfg( ( IConfigDb* )
                    pCb->GetConfig() );
                IConfigDb* pResp = nullptr;
                ret = oCfg.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oResp( pResp );
                gint32 iRet = 0;
                ret = oResp.GetIntProp(
                    propReturnValue, ( guint32& )iRet );
                if( ERROR( ret ) )
                    break;

                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }

                IConfigDb* pInfo;
                ret = oResp.GetPointer( 0, pInfo );
                if( ERROR( ret ) )
                {
                    ret = -EACCES;
                    break;
                }

                Variant oVar;

                bool bGmSSL = false;
                ret = pInfo->GetProperty( propGmSSL, oVar );
                if( SUCCEEDED( ret ) )
                    bGmSSL = ( bool& )oVar;

                ret = pInfo->GetProperty( 0, oVar );
                if( ERROR( ret ) )
                {
                    ret = -EACCES;
                    break;
                }

                BufPtr pSvrToken = ( BufPtr& )oVar;
                if( pSvrToken.IsEmpty() )
                {
                    ret = -EACCES;
                    break;
                }

                ret = CheckServerToken(
                    pReq, pSvrToken, bGmSSL );
                if( ERROR( ret ) )
                    break;

                stdstr strSess;
                ret = oResp.GetStrProp(
                    propSessHash, strSess );
                if( ERROR( ret ) )
                    break;
                pIf->SetSess( strSess );
                DebugPrint( 0, "Sess hash is %s",
                    strSess.c_str() );
                oVar = ( bool )false;
                pIf->SetProperty( propNoEnc, oVar );
            }while( 0 );
            pLoginCb->OnEvent( eventTaskComp,
                ret, 0, nullptr );
            return 0;
        });

        ObjPtr pCtx;
        CCfgOpener oInfo( pInfo );
        oInfo.GetObjPtr( propContext, pCtx );
        oInfo.RemoveProperty( propContext );
        
        TaskletPtr pLoginCb;
        ret = NEW_COMPLETE_FUNCALL( 0,
            pLoginCb, this->GetIoMgr(), func,
            nullptr, pCallback, this, pCtx );
        if( ERROR( ret ) )
            break;

        ret = this->Login( pLoginCb,
            pInfo, oResp.GetCfg() );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            ( *pLoginCb )( eventCancelTask );
            break;
        }

        oResp.GetIntProp(
            propReturnValue, ( guint32& )ret );
        if( ERROR( ret ) )
            break;

        std::string strSess;
        ret = oResp.GetStrProp(
            propSessHash, strSess );
        DebugPrint( 0, "Sess hash is %s",
            strSess.c_str() );
        SetSess( strSess );

    }while( 0 );
    return ret;
}

gint32 CSimpAuthLoginProxy::StartLogin(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    bool bHooked = false;
    do{
        std::string strMech = GET_MECH( this );
        if( strMech != "SimpAuth" )
        {
            ret = -ENOTSUP;
            break;
        }

        CCfgOpener oAuth(
            ( IConfigDb* )GET_AUTH( this ) );

        stdstr strUser;
        oAuth.GetStrProp(
            propUserName, strUser );

        CParamList oParams;
        ret = BuildLoginInfo(
            strUser, oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = this->SimpAuthLogin(
            pCallback, oParams.GetCfg() );

    }while( 0 );
    return ret;
}
}
