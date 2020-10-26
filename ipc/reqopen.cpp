/*
 * =====================================================================================
 *
 *       Filename:  reqopen.cpp
 *
 *    Description:  implementation of CReqOpener and CReqBuilder
 *
 *        Version:  1.0
 *        Created:  08/25/2018 05:18:25 PM
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
#include "frmwrk.h"
#include "proxy.h"

namespace rpcfrmwrk
{

using namespace std;

CReqBuilder::CReqBuilder(
    CRpcBaseOperations* pIf )
    : CParamList()
{
    if( pIf == nullptr )
        return;

    gint32 ret = 0;
    do{
        m_pIf = pIf;

        ObjPtr pObj = pIf;

        CopyProp( propIfName, pIf );
        CopyProp( propObjPath, pIf );
        CopyProp( propSrcDBusName, pIf );

        if( !pIf->IsServer() )
        {
            // a mandatory parameter for proxy
            ret = CopyProp( propDestDBusName, pIf );
            if( ERROR( ret ) )
                break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        string strModName = pMgr->GetModName();

        CCfgOpenerObj oSrcCfg( pIf );
        guint32 dwVal;

        ret = oSrcCfg.GetIntProp(
            propTimeoutSec, dwVal );

        if( SUCCEEDED( ret ) )
            SetTimeoutSec( dwVal );

        ret = oSrcCfg.GetIntProp(
            propKeepAliveSec, dwVal );

        if( SUCCEEDED( ret ) )
            SetKeepAliveSec( dwVal );

    }while( 0 );
}

CReqBuilder::CReqBuilder(
    const IConfigDb* pCfg )
    : CParamList()
{
    if( pCfg == nullptr )
        return;

    try{
        CopyProp( propIfName, pCfg );
        CopyProp( propObjPath, pCfg );
        CopyProp( propDestDBusName, pCfg );
        CopyProp( propSrcDBusName, pCfg );

        CopyProp( propMethodName, pCfg );
        CopyProp( propCallOptions, pCfg );
        CopyProp( propTaskId, pCfg );

        CCfgOpener oSrcCfg( pCfg );
        guint32 dwVal;

        if( oSrcCfg.exist( propTimeoutSec ) )
        {
            dwVal = oSrcCfg[ propTimeoutSec ];
            SetTimeoutSec( dwVal );
        }
        if( oSrcCfg.exist( propKeepAliveSec ) )
        {
            dwVal = oSrcCfg[ propKeepAliveSec ];
            SetKeepAliveSec( dwVal );
        }

        CopyParams( pCfg );
    }
    catch( std::invalid_argument& e )
    {

    }
    return;
}

CReqBuilder::CReqBuilder(
    IConfigDb* pCfg )
    : CParamList( pCfg )
{ return; }

gint32 CReqBuilder::SetIfName(
    const std::string& strIfName )
{
    return SetStrProp(
        propIfName, strIfName );
}

gint32 CReqBuilder::SetObjPath(
    const std::string& strObjPath ) 
{
    return SetStrProp(
        propObjPath, strObjPath );
}
gint32 CReqBuilder::SetDestination(
    const std::string& strDestination ) 
{
    return SetStrProp(
        propDestDBusName, strDestination );
}

gint32 CReqBuilder::SetSender(
    const std::string& strSender ) 
{
    return SetStrProp(
        propSrcDBusName, strSender );
}

gint32 CReqBuilder::SetMethodName(
    const std::string& strMethodName ) 
{
    return SetStrProp(
        propMethodName, strMethodName );
}

gint32 CReqBuilder::SetKeepAliveSec(
    guint32 dwTimeoutSec )
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    return oCfg.SetIntProp(
        propKeepAliveSec, dwTimeoutSec );
}

gint32 CReqBuilder::SetTimeoutSec(
    guint32 dwTimeoutSec)
{

    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    return oCfg.SetIntProp(
        propTimeoutSec, dwTimeoutSec );
}

gint32 CReqBuilder::SetCallFlags(
    guint32 dwFlags )
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    return oCfg.SetIntProp(
        propCallFlags, dwFlags );
}

gint32 CReqBuilder::SetReturnValue(
    gint32 iRet )
{
    return SetIntProp(
        propReturnValue, iRet );
}

gint32 CReqBuilder::GetCallOptions(
    CfgPtr& pCfg )
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = GetObjPtr(
            propCallOptions, pObj );

        if( ERROR( ret ) )
        {
            ret = pCfg.NewObj();
            if( ERROR( ret ) )
                break;

            pObj = pCfg;
            if( pObj.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }
            ret = SetObjPtr(
                propCallOptions, pObj );

            if( ERROR( ret ) )
                break;

            break;
        }

        CfgPtr pSrcCfg = pObj;
        if( pSrcCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        pCfg = pSrcCfg;

    }while( 0 );

    return ret;
}

gint32 CReqBuilder::SetTaskId(
    guint64 dwTaskId )
{
    return SetQwordProp(
        propTaskId, dwTaskId );
}

gint32 CReqOpener::GetIfName(
    std::string& strIfName ) const
{
    return GetStrProp(
        propIfName, strIfName );
}
gint32 CReqOpener::GetObjPath(
    std::string& strObjPath ) const 
{
    return GetStrProp(
        propObjPath, strObjPath );
}

gint32 CReqOpener::GetDestination(
    std::string& strDestination ) const 
{
    return GetStrProp(
        propDestDBusName, strDestination );
}

gint32 CReqOpener::GetMethodName(
    std::string& strMethodName ) const
{
    return GetStrProp(
        propMethodName, strMethodName );
}

gint32 CReqOpener::GetSender(
    std::string& strSender ) const
{
    return GetStrProp(
        propSrcDBusName, strSender );
}

gint32 CReqOpener::GetKeepAliveSec(
    guint32& dwTimeoutSec ) const
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg ); 
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCallOption(
        ( IConfigDb* ) pCfg );

    return oCallOption.GetIntProp(
        propKeepAliveSec, dwTimeoutSec );

}

gint32 CReqOpener::GetTimeoutSec(
    guint32& dwTimeoutSec) const 
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg ); 
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCallOption(
        ( IConfigDb* ) pCfg );

    return oCallOption.GetIntProp(
        propTimeoutSec, dwTimeoutSec );
}

gint32 CReqOpener::GetCallFlags(
    guint32& dwFlags ) const
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg ); 
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCallOption(
        ( IConfigDb* ) pCfg );

    return oCallOption.GetIntProp(
        propCallFlags, dwFlags );
}

gint32 CReqOpener::GetReturnValue(
    gint32& iRet ) const
{
    guint32 dwRet = ( guint32 ) iRet;
    return GetIntProp(
        propReturnValue, dwRet );
}

bool CReqOpener::HasReply() const
{
    guint32 dwFlags;
    gint32 ret = GetCallFlags( dwFlags );
    if( ERROR( ret ) )
    {
        // default to have a replay
        return true;
    }
    bool bRet =
        ( ( dwFlags & CF_WITH_REPLY ) != 0 );

    return bRet;
}

bool CReqOpener::IsKeepAlive() const
{
    guint32 dwFlags;
    gint32 ret = GetCallFlags( dwFlags );
    if( ERROR( ret ) )
    {
        // default to no keep alive
        return false;
    }
    bool bRet =
        ( ( dwFlags & CF_KEEP_ALIVE ) != 0 );

    return bRet;
}

gint32 CReqOpener::GetReqType(
    guint32& dwType ) const
{
    guint32 dwFlags;
    gint32 ret = GetCallFlags( dwFlags );
    if( ERROR( ret ) )
        return ret;

    dwType = ( dwFlags & 7 );
    return 0;
}

gint32 CReqOpener::GetCallOptions(
    CfgPtr& pCfg ) const
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = GetObjPtr(
            propCallOptions, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pSrcCfg = pObj;
        if( pSrcCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        pCfg = pSrcCfg;

    }while( 0 );

    return ret;
}

gint32 CReqOpener::GetTaskId(
    guint64& dwTaskId ) const
{
    return GetQwordProp(
        propTaskId, dwTaskId );
}

}
