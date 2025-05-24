/*
 * =====================================================================================
 *
 *       Filename:  oa2proxy.cpp
 *
 *    Description:  implementation of OAuth2 proxy for non-js client login
 *
 *        Version:  1.0
 *        Created:  12/02/2024 02:59:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2020 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 * =====================================================================================
 */

#include "rpc.h"
#include "ifhelper.h"
#include "security.h"
#include "oa2proxy.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <pwd.h>

namespace rpcf 
{
COAuth2LoginProxy::COAuth2LoginProxy(
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

gint32 COAuth2LoginProxy::RebuildMatches()
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

gint32 COAuth2LoginProxy::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( IAuthenticate, false );

    ADD_PROXY_METHOD_ASYNC( 1,
        COAuth2LoginProxy::Login,
        SYS_METHOD( AUTH_METHOD_LOGIN ) );

    ADD_PROXY_METHOD_ASYNC( 1,
        COAuth2LoginProxy::MechSpecReq,
        SYS_METHOD( AUTH_METHOD_MECHSPECREQ ) );

    END_IFPROXY_MAP;
    return 0;
}

gint32 COAuth2LoginProxy::Login(
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

gint32 COAuth2LoginProxy::DoLogin(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        TaskletPtr pDeferTask;

        CfgPtr pResp;
        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pDeferTask, ObjPtr( this ),
            &COAuth2LoginProxy::StartLogin,
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
            pDeferTask, true );

        if( ERROR( ret ) )
        {
            ( *pDeferTask )( eventCancelTask );
            break;
        }

        ret = STATUS_PENDING;

    }while( 0 );
    return ret;
}

gint32 COAuth2LoginProxy::DecryptCookie(
    const stdstr& strHex,
    stdstr& strCookie )
{
    gint32 ret = 0;
    do{
        BufPtr pEnc( true );
        ret = pEnc->Resize(
            strHex.size() / 2 );
        if( ERROR( ret ) )
            break;

        ret = HexStringToBytes(
            strHex.c_str(), strHex.size(),
            ( guint8* )pEnc->ptr() );
        if( ERROR( ret ) )
            break;

        stdstr strHome = GetHomeDir();
        strHome += "/.rpcf/openssl/";
        std::vector< stdstr > vecKeys = {
             "clientkey.pem", "signkey.pem" };

        BufPtr pDec( true );
        size_t dec_len = 0;
        for( auto& elem : vecKeys )
        {
            stdstr strKeyPath = strHome + elem;
            FILE *pri_fp = fopen(
                strKeyPath.c_str(), "r");

            if( pri_fp == nullptr )
                continue;

            EVP_PKEY *pri_pkey =
                PEM_read_PrivateKey(
                    pri_fp, NULL, NULL, NULL);

            fclose(pri_fp);

            EVP_PKEY_CTX *dctx =
                EVP_PKEY_CTX_new(pri_pkey, NULL);
            EVP_PKEY_decrypt_init(dctx);
            ret = EVP_PKEY_CTX_set_rsa_padding(
                dctx, RSA_PKCS1_OAEP_PADDING );
            if( ret <= 0 )
            {
                ret = -EFAULT;
                continue;
            }
            ret = EVP_PKEY_CTX_set_rsa_oaep_md(
                dctx, EVP_sha256() );
            if( ret <= 0 )
            {
                ret = -EFAULT;
                continue;
            }

            ret = EVP_PKEY_decrypt(dctx, NULL,
                &dec_len, ( guint8* )pEnc->ptr(),
                pEnc->size() );
            if( ret <= 0 )
                continue;

            std::vector<unsigned char>
                dec_data(dec_len);
            pDec->Resize( dec_len );

            ret = EVP_PKEY_decrypt(
                dctx, ( guint8* )pDec->ptr(),
                &dec_len, ( guint8* )pEnc->ptr(),
                pEnc->size());
            if( ret <= 0 )
            {
                unsigned long error = ERR_get_error(); 
                // Get a human-readable error string
                char error_string[256];
                ERR_error_string(error, error_string);
                OutputMsg(0, "Error message: %s\n", error_string);
            }
            EVP_PKEY_CTX_free(dctx);
            if( ret > 0 )
                break;
        }

        if( ret > 0 )
            strCookie.append(
                pDec->ptr(), dec_len );
        else
            ret = -EACCES;
    }while( 0 );
    return ret;
}

gint32 COAuth2LoginProxy::OAuth2Login(
    IEventSink* pCallback,
    IConfigDb* pInfo )
{
    gint32 ret = 0;
    do{
        CCfgOpener oResp;
        gint32 (*func)(CTasklet*,
            IEventSink*, COAuth2LoginProxy* ) =
        ([]( CTasklet* pCb,
            IEventSink* pLoginCb,
            COAuth2LoginProxy* pIf )->gint32
        {
            if( pCb == nullptr )
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
                stdstr strSess;
                ret = oResp.GetStrProp(
                    propSessHash, strSess );
                if( ERROR( ret ) )
                    break;
                pIf->SetSess( strSess );
                DebugPrint( 0, "Sess hash is %s",
                    strSess.c_str() );
                Variant oVar( false );
                pIf->SetProperty( propNoEnc, oVar );
            }while( 0 );
            pLoginCb->OnEvent( eventTaskComp,
                ret, 0, nullptr );
            return 0;
        });

        TaskletPtr pLoginCb;
        ret = NEW_COMPLETE_FUNCALL( 0,
            pLoginCb, this->GetIoMgr(),
            func, nullptr, pCallback, this );
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

gint32 COAuth2LoginProxy::StartLogin(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    bool bHooked = false;
    do{
        std::string strMech = GET_MECH( this );
        if( strMech != "OAuth2" )
        {
            ret = -ENOTSUP;
            break;
        }

        CCfgOpener oAuth(
            ( IConfigDb* )GET_AUTH( this ) );

        stdstr strCookie;
        stdstr strEncCookie;
        ret = oAuth.GetStrProp(
            propCookie, strCookie );

        if( ERROR( ret ) )
        {
            ret = oAuth.GetStrProp(
                propEncCookie, strEncCookie );
            if( ERROR( ret ) )
                break;
        }

        if( strEncCookie.size() )
        {
            ret = DecryptCookie(
                strEncCookie, strCookie );
            if( ERROR( ret ) )
            {
                ret = -EACCES;
                break;
            }
        }

        CParamList oParams;
        CCfgOpener oResp;
        oParams.Push( strCookie );
        ret = OAuth2Login( pCallback,
            oParams.GetCfg() );

    }while( 0 );
    return ret;
}

}
