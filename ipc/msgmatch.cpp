/*
 * =====================================================================================
 *
 *       Filename:  msgmatch.cpp
 *
 *    Description:  definitions of message match classes
 *
 *        Version:  1.0
 *        Created:  10/13/2022 08:59:18 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "msgmatch.h"
#include "reqopen.h"
namespace rpcf{

CMessageMatch::CMessageMatch( const IConfigDb* pCfg ) :
    CMessageMatch()
{
    gint32 ret = 0;

    do{
        if( pCfg == nullptr )
        {
            // maybe user want to setup the
            // properties later
            break;
        }

        CCfgOpener a( pCfg );

        ret = a.GetStrProp(
            propIfName, m_strIfName );

        if( ERROR( ret ) )
            break;

        guint32* pMatchType =
            ( guint32* )&m_iMatchType;

        ret = a.GetIntProp( propMatchType,
            *pMatchType );

        if( ERROR( ret ) )
            break;

        ret = a.GetStrProp(
            propObjPath, m_strObjPath );

    }while( 0 );

    if( ERROR( ret ) )
    {
        stdstr strMsg =
            DebugMsg( ret, "error in CMessageMatch's ctor" );

        throw std::runtime_error( strMsg );
    }
}

// if a request message this interface can handle
gint32 CMessageMatch::IsMyMsgBasic(
    DBusMessage* pMessage ) const
{
    gint32 ret = 0;

    do{
        if( pMessage == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        DMsgPtr pMsg( pMessage );

        if( m_strIfName.empty()
            || m_strObjPath.empty() )
        {
            ret = -EINVAL;
            break;
        }

        // test if the interface is the same
        stdstr strIf = std::move(
            pMsg.GetInterface() );

        if( m_strIfName != strIf )
        {
            if( strIf.empty() )
                ret = -EBADMSG;
            else
                ret = ERROR_FALSE;
            break;
        }

        // test if the object is the same
        if( m_strObjPath != pMsg.GetPath() )
        {
            ret = ERROR_FALSE;
        }

    }while( 0 );

    return ret;
}

gint32 CMessageMatch::IsMyMsgIncoming(
    DBusMessage* pMessage ) const
{
    gint32 ret = 0;

    if( pMessage == nullptr )
        return -EINVAL;

    do{
        DMsgPtr pMsg( pMessage );
        bool bClient = ( GetType() == matchClient );

        gint32 iType = pMsg.GetType();

        if( bClient )
        {
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
            case DBUS_MESSAGE_TYPE_SIGNAL:
                break;

            case DBUS_MESSAGE_TYPE_METHOD_CALL:
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
        }
        else
        {
            // a message from the server
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                break;

            case DBUS_MESSAGE_TYPE_SIGNAL:
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
        }

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    return IsMyMsgBasic( pMessage );
}

// if a event message this interface has subcribed
gint32 CMessageMatch::IsMyMsgOutgoing(
    DBusMessage* pMessage ) const
{
    gint32 ret = 0;

    if( pMessage == nullptr )
        return -EINVAL;

    do{
        DMsgPtr pMsg( pMessage );
        bool bClient = ( GetType() == matchClient );

        gint32 iType = pMsg.GetType();

        if( bClient )
        {
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                break;

            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
            case DBUS_MESSAGE_TYPE_SIGNAL:
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
        }
        else
        {
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_SIGNAL:
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
                    break;

            case DBUS_MESSAGE_TYPE_METHOD_CALL:
            default:
                ret = -ENOTSUP;
                break;
            }
        }

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    return IsMyMsgBasic( pMessage );
}

gint32 CMessageMatch::IsMyMsgBasic(
    IConfigDb* pMessage ) const
{
    gint32 ret = 0;

    do{
        if( pMessage == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CCfgOpener oMsg( pMessage );
        if( m_strIfName.empty()
            || m_strObjPath.empty() )
        {
            ret = -EINVAL;
            break;
        }

        stdstr strVal;
        ret = oMsg.GetStrProp(
            propIfName, strVal );
        if( ERROR( ret ) )
            break;

        // test if the interface is the same
        if( m_strIfName != strVal )
        {
            ret = ERROR_FALSE;
            break;
        }

        ret = oMsg.GetStrProp(
            propObjPath, strVal );
        if( ERROR( ret ) )
            break;

        // test if the object is the same
        if( m_strObjPath != strVal )
        {
            ret = ERROR_FALSE;
        }

    }while( 0 );

    return ret;
}

gint32 CMessageMatch::IsMyMsgIncoming(
    IConfigDb* pMessage ) const
{
    gint32 ret = 0;

    if( pMessage == nullptr )
        return -EINVAL;

    do{
        CCfgOpener oMsg( pMessage );
        bool bClient = ( GetType() == matchClient );

        IConfigDb* pOptions;
        ret = oMsg.GetPointer(
            propCallOptions, pOptions );
        if( ERROR( ret ) )
            break;

        guint32 dwFlags = 0;
        CCfgOpener oOptions( pOptions );
        ret = oOptions.GetIntProp(
            propCallFlags, dwFlags );
        if( ERROR( ret ) )
            break;

        gint32 iType =
            ( dwFlags & CF_MESSAGE_TYPE_MASK );

        if( bClient )
        {
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
            case DBUS_MESSAGE_TYPE_SIGNAL:
                break;

            case DBUS_MESSAGE_TYPE_METHOD_CALL:
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
        }
        else
        {
            // a message from the server
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                break;

            case DBUS_MESSAGE_TYPE_SIGNAL:
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
        }

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    return IsMyMsgBasic( pMessage );
}

// if a event message this interface has subcribed
gint32 CMessageMatch::IsMyMsgOutgoing(
    IConfigDb* pMessage ) const
{
    gint32 ret = 0;

    if( pMessage == nullptr )
        return -EINVAL;

    do{
        CCfgOpener oMsg( pMessage );
        bool bClient = ( GetType() == matchClient );

        IConfigDb* pOptions;
        ret = oMsg.GetPointer(
            propCallOptions, pOptions );
        if( ERROR( ret ) )
            break;

        guint32 dwFlags = 0;
        CCfgOpener oOptions( pOptions );
        ret = oOptions.GetIntProp(
            propCallFlags, dwFlags );
        if( ERROR( ret ) )
            break;

        gint32 iType =
            ( dwFlags & CF_MESSAGE_TYPE_MASK );


        if( bClient )
        {
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                break;

            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
            case DBUS_MESSAGE_TYPE_SIGNAL:
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
        }
        else
        {
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_SIGNAL:
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            case DBUS_MESSAGE_TYPE_ERROR:
                    break;

            case DBUS_MESSAGE_TYPE_METHOD_CALL:
            default:
                ret = -ENOTSUP;
                break;
            }
        }

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    return IsMyMsgBasic( pMessage );
}

stdstr CMessageMatch::GetDest() const
{
    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );
    stdstr strMyDest;

    gint32 ret = oCfg.GetStrProp(
        propDestDBusName, strMyDest );

    if( ERROR( ret ) )
        return stdstr( "" );

    return strMyDest;
}

bool CMessageMatch::IsMyObjPath(
    const stdstr& strObjPath ) const
{

    // the dest is a prefix of obj path
    stdstr strMyObjPath = m_strObjPath;

    std::replace( strMyObjPath.begin(),
        strMyObjPath.end(), '/', '.' );
        
    if( strObjPath == strMyObjPath )
        return true;

    return false;
}

bool CMessageMatch::IsMyDest(
    const stdstr& strDest ) const
{
    gint32 ret = 0;
    stdstr strMyDest = GetDest();
    if( !strMyDest.empty() )
    {
        ret = strncmp(
            strMyDest.c_str(),
            strDest.c_str(),
            strDest.size() );
        if( ret != 0 )
            return false;
    }
    else
    {
        // the dest is a prefix of obj path
        strMyDest = m_strObjPath;
        std::replace( strMyDest.begin(),
            strMyDest.end(), '/', '.' );
        gint32 iIdx = 0;

        if( strMyDest[ 0 ] == '.' )
            iIdx = 1;

        if( strDest != strMyDest.substr( iIdx, strDest.size() ) )
            return false;

        // FIXME: it is not a right solution.
        // we should rely on the
        // propDestDBusName for accurate match
        ret = 0;
    }

    if( ERROR( ret ) )
        return false;

    return true;
}

// a equalness comparison
bool CMessageMatch::operator==( const IMessageMatch& rhs ) const
{
    if( m_strObjPath.empty()
        || m_strIfName.empty() )
        return false;

    if( GetType() != rhs.GetType() )
        return false;

    if( ToString() != rhs.ToString() )
        return false;

    return true;
}

const IMessageMatch& CMessageMatch::operator=(
    const IMessageMatch& rhs )
{
    const CMessageMatch& oMatch =
        static_cast< const CMessageMatch& >( rhs );

    return ( *this = oMatch );
}

stdstr CMessageMatch::ToDBusRules(
    gint32 iMsgType ) const
{
    gint32 ret = 0;

    stdstr strMatch;
    stdstr strRule = "type=";

    switch( iMsgType )
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:
        {
            strRule += "'signal'";
            break;
        }
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
        {
            strRule += "'method_call'";
            break;
        }
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        {
            strRule += "'method_return'";
            break;
        }
    case DBUS_MESSAGE_TYPE_ERROR:
        {
            strRule += "'error'";
            break;
        }
    case DBUS_MESSAGE_TYPE_INVALID:
    default:
        {
            strRule = "";
            break;
        }
    }

    if( !strRule.empty() )
        strMatch = strRule;

    GetIfName( strRule );
    if( !strRule.empty() )
    {
        strMatch += stdstr( ",interface='" )
            + strRule + stdstr( "'" );
    }

    GetObjPath( strRule );
    if( !strRule.empty() )
    {
        strMatch += stdstr( ",path='" )
            + strRule + stdstr( "'" );
    }

    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );
    ret = oCfg.GetStrProp(
        propMethodName, strRule );
    if( SUCCEEDED( ret ) )
    {
        if( SUCCEEDED( ret ) && !strRule.empty() )
        {
            strMatch += stdstr( ",member='" )
                + strRule + stdstr( "'" );
        }
    }

    return strMatch;
}

stdstr CMessageMatch::ToString() const
{
    stdstr strAll =
        ToDBusRules( DBUS_MESSAGE_TYPE_INVALID );

    strAll += ",match=";
    if( GetType() == matchClient )
    {
        strAll += "matchClient";
    }
    else if( GetType() == matchServer )
    {
        strAll += "matchServer";
    }

    return strAll;
}

gint32 CMessageMatch::EnumProperties(
    std::vector< gint32 >& vecProps ) const 
{
    gint32 ret =
        super::EnumProperties( vecProps );

    if( ERROR( ret ) )
        return ret;

    vecProps.push_back( propObjPath );
    vecProps.push_back( propIfName );
    vecProps.push_back( propMatchType );

    ret = m_pCfg->EnumProperties( vecProps );

    if( ERROR( ret ) )
        return ret;

    return 0;
}

gint32 CMessageMatch::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{

    gint32 ret = 0;
    switch( iProp )
    {
    case propObjPath:
    case propIfName:
        {
            iType = typeString;
            break;
        }
    case propMatchType:
        {
            iType = typeUInt32;
            break;
        }
    default:
        {
            ret = m_pCfg->GetPropertyType(
                iProp, iType );
            break;
        }
    }
    return ret;
}

bool CMessageMatch::exist( gint32 iProp ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propObjPath:
    case propIfName:
    case propMatchType:
        {
            ret = true;
            break;
        }
    default:
        {
            ret = m_pCfg->exist( iProp );
            break;
        }
    }
    return ret;
}

gint32 CMessageMatch::GetProperty(
    gint32 iProp, Variant& oBuf ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propObjPath:
        {
            oBuf = m_strObjPath;
            break;
        }
    case propIfName:
        {
            oBuf = m_strIfName;
            break;
        }
    case propMatchType:
        {
            oBuf = GetType();
            break;
        }
    default:
        {
            ret = m_pCfg->GetProperty(
                iProp, oBuf );
            break;
        }
    }
    return ret;
}

gint32 CMessageMatch::SetProperty(
    gint32 iProp,
    const Variant& oBuf )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propObjPath:
        {
            if( !IsValidDBusPath( ( const stdstr& )oBuf ) )
            {
                ret = -EINVAL;
                break;
            }
            m_strObjPath = ( const stdstr& )oBuf;
            break;
        }
    case propIfName:
        {
            if( !IsValidDBusName( ( const stdstr& )oBuf ) )
            {
                ret = -EINVAL;
                break;
            }
            m_strIfName = ( const stdstr& )oBuf;
            break;
        }
    case propMatchType:
        {
            guint32 dwType = oBuf;
            SetType( ( EnumMatchType )dwType );
            break;
        }
    default:
        {
            ret = m_pCfg->SetProperty(
                iProp, oBuf );
            break;
        }
    }
    return ret;
}
gint32 CMessageMatch::RemoveProperty( gint32 iProp )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propObjPath:
        {
            m_strObjPath.clear();
            break;
        }
    case propIfName:
        {
            m_strIfName.clear();
            break;
        }
    case propMatchType:
        {
            SetType( matchInvalid );
            break;
        }
    default:
        {
            ret = m_pCfg->
                RemoveProperty( iProp );
            break;
        }
    }
    return ret;
}

const CMessageMatch& CMessageMatch::operator=(
    const CMessageMatch& rhs )
{
    m_strObjPath = rhs.m_strObjPath;
    m_strIfName = rhs.m_strIfName;
    SetType( rhs.GetType() );
    if( !rhs.m_pCfg.IsEmpty() )
    {
        *m_pCfg = *rhs.m_pCfg;
    }
    else
    {
        m_pCfg.Clear();
    }

    return *this;
}

gint32 CMessageMatch::Serialize(
    CBuffer& oBuf ) const
{
    gint32 ret = 0;
    CfgPtr pCfg( true );
    *pCfg = *m_pCfg;
    CCfgOpener oCfg( ( IConfigDb* )pCfg );
    oCfg[ propIfName ] = m_strIfName;
    oCfg[ propObjPath ] = m_strObjPath;

    SERI_HEADER oHeader;

    oHeader.iMatchType = GetType();

    BufPtr pCfgBuf( true );
    ret = pCfg->Serialize( *pCfgBuf );

    if( ERROR( ret ) )
        return ret;

    // NOTE: we DON'T use the 
    // clsid( CMessageMatch )
    oHeader.dwClsid = GetClsid();

    oHeader.dwSize = pCfgBuf->size() +
        sizeof( SERI_HEADER ) -
        sizeof( SERI_HEADER_BASE );

    oHeader.hton();
    ret = oBuf.Append(
        ( guint8* )&oHeader,
        sizeof( oHeader ) );

    if( ERROR( ret ) )
        return ret;

    ret = oBuf.Append(
        ( guint8* )pCfgBuf->ptr(),
        pCfgBuf->size() );

    if( ERROR( ret ) )
        return ret;

    return 0;
}

gint32 CMessageMatch::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

gint32 CMessageMatch::Deserialize(
    const char* pBuf, guint32 dwSize )
{
    gint32 ret = 0;

    do{
        if( pBuf == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        const SERI_HEADER* pHeader =
            ( const SERI_HEADER* )pBuf;
        
        SERI_HEADER oHeader;
        oHeader = *pHeader;
        oHeader.ntoh();

        // NOTE: we DON'T hardcode the clsid(
        // CMessageMatch )
        if( oHeader.dwClsid != GetClsid() )
        {
            ret = -EINVAL;
            break;
        }

        gint32 ret = m_pCfg.NewObj();
        if( ERROR( ret ) )
            break;

        ret = m_pCfg->Deserialize(
            pBuf + sizeof( oHeader ),
            oHeader.dwSize );

        if( ERROR( ret ) )
            return ret;

        CCfgOpener oCfg( ( IConfigDb* )m_pCfg );
        ret = oCfg.GetStrProp(
            propIfName, m_strIfName );
        if( ERROR( ret ) )
            break;

        ret = oCfg.GetStrProp(
            propObjPath, m_strObjPath );
        if( ERROR( ret ) )
            break;

        SetType( ( EnumMatchType )oHeader.iMatchType );

    }while( 0 );

    return ret;
}

gint32 CMessageMatch::Clone( CMessageMatch* rhs )
{
    
    m_strObjPath = rhs->m_strObjPath;
    m_strIfName = rhs->m_strIfName;
    m_iMatchType = rhs->m_iMatchType;
    if( rhs->m_pCfg.IsEmpty() )
    {
        m_pCfg.Clear();
        return STATUS_SUCCESS;
    }
    gint32 ret = m_pCfg.NewObj(
        clsid( CConfigDb2 ) );
    if( ERROR( ret ) )
        return ret;
    *m_pCfg = *rhs->m_pCfg;
    return STATUS_SUCCESS;
}

CProxyMsgMatch::CProxyMsgMatch( const IConfigDb* pCfg ) :
    CMessageMatch( pCfg )
{
    SetClassId( clsid( CProxyMsgMatch ) );

    if( pCfg == nullptr )
        return;

   CCfgOpener oCfg( ( IConfigDb* )GetCfg() );
   oCfg.CopyProp( propIpAddr, pCfg );
}

// test if a event message this match is
// listening to

gint32 CProxyMsgMatch::IsMyMsgIncoming(
    DBusMessage* pDBusMsg ) const 
{
    gint32 ret =
        super::IsMyMsgIncoming( pDBusMsg );

    if( ERROR( ret ) )
        return ret;

    DMsgPtr pMsg( pDBusMsg );
    ObjPtr pTransCtx;
    ret = pMsg.GetObjArgAt( 0, pTransCtx );
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );
    ret = oCfg.IsEqualProp( propConnHandle,
        ( CObjBase* )pTransCtx );

    if( ERROR( ret ) )
        return ret;

    stdstr strEvtPath;
    CCfgOpener oTransCtx(
        ( IConfigDb* )pTransCtx );

    ret = oTransCtx.GetStrProp(
        propRouterPath, strEvtPath );

    if( ERROR( ret ) )
        return ret;

    stdstr strExpPath;
    ret = oCfg.GetStrProp(
        propRouterPath, strExpPath );

    if( ERROR( ret ) )
        return ret;
    
    if( strEvtPath == strExpPath )
        return STATUS_SUCCESS;

    ret = ERROR_FALSE;
    if( strEvtPath.size() > strExpPath.size() )
        return ret;

    guint32 dwLen = strEvtPath.size();
    if( dwLen < strExpPath.size() )
    {
        stdstr strVal =
            strExpPath.substr( 0, dwLen );

        if( pMsg.GetMember() != 
            SYS_EVENT_RMTSVREVENT )
            return ret;

        if( strVal != strEvtPath )
            return ret;

        if( strVal[ dwLen - 1 ] == '/' )
            return STATUS_SUCCESS;

        if( strExpPath[ dwLen ] == '/' )
            return STATUS_SUCCESS;
    }
    return ERROR_FALSE;
}

stdstr CProxyMsgMatch::ToString() const
{
    stdstr strAll = super::ToString();

    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );

    guint32 dwConnHandle = 0;
    gint32 ret = oCfg.GetIntProp(
        propConnHandle, dwConnHandle );

    if( ERROR( ret ) )
        return strAll;

    strAll += ",ConnHandle=";
    strAll += std::to_string( dwConnHandle );

    stdstr strValue;
    ret = oCfg.GetStrProp(
        propRouterPath, strValue );
    if( ERROR( ret ) )
        return strAll;

    strAll += ",RouterPath=";
    strAll += strValue; 

    return strAll;
}

CDBusDisconnMatch::CDBusDisconnMatch(
    const IConfigDb* pCfg ) :
    CMessageMatch( pCfg )
{
    SetClassId( clsid( CDBusDisconnMatch ) );

    gint32 ret = 0;

    if( pCfg != nullptr )
    {
        m_pCfg = pCfg;
    }

    do{
        SetObjPath( DBUS_PATH_LOCAL );
        SetIfName( DBUS_INTERFACE_LOCAL );
        SetType( matchClient );

        CCfgOpener oCfg( ( IConfigDb* )m_pCfg );
        ret = oCfg.SetStrProp(
            propMethodName, "Disconnected" );

    }while( 0 );

    if( ERROR( ret ) )
    {
        stdstr strInfo = DebugMsg(
            ret, "Error in CDBusDisconnMatch ctor" );

        throw std::runtime_error( strInfo );
    }
    return;
}

gint32 CDBusDisconnMatch::IsMyMsgIncoming(
    DBusMessage* pMessage ) const
{
    DMsgPtr pMsg( pMessage );

    gint32 ret = super::IsMyMsgIncoming( pMsg );

    if( ERROR( ret ) )
        return ret;

    stdstr strMsgMember;

    strMsgMember = pMsg.GetMember();

    if( strMsgMember.empty() )
        return ERROR_FALSE;

    CCfgOpener oCfg( ( IConfigDb* )m_pCfg );
    stdstr strMember;

    ret = oCfg.GetStrProp(
        propMethodName, strMember );

    if( ERROR( ret ) )
        return ret;

    if( strMember != strMsgMember )
        return ERROR_FALSE;

    return 0;
}

stdstr CDBusDisconnMatch::ToString() const
{
    stdstr strAll = super::ToString();
    strAll += ",Member=";
    strAll += "Disconnected"; 
    return strAll;
}

CDBusSysMatch::CDBusSysMatch(
    const IConfigDb* pCfg ) :
    CMessageMatch( nullptr )
{
    SetClassId( clsid( CDBusSysMatch ) );

    gint32 ret = 0;
    do{
        CCfgOpener matchCfg(
            ( IConfigDb* )m_pCfg );

        SetObjPath( DBUS_SYS_OBJPATH );
        SetIfName( DBUS_SYS_INTERFACE );
        SetType( matchClient );

        ret = matchCfg.SetStrProp(
            propDestDBusName, 
            DBUS_SERVICE_DBUS );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "error in CDBusSysMatch's ctor" );

        throw std::runtime_error( strMsg );
    }
}

CRouterRemoteMatch::CRouterRemoteMatch(
    const IConfigDb* pCfg ) :
    CMessageMatch( pCfg )
{
    SetClassId( clsid( CRouterRemoteMatch ) );
    SetType( matchClient );
}

gint32 CRouterRemoteMatch::CopyMatch(
    IMessageMatch* pMatch )
{
    CCfgOpenerObj oCfg( this );
    gint32 ret = 0;

    do{
        ret = oCfg.CopyProp( propObjPath, pMatch );
        if( ERROR( ret ) )
            break;

        ret = oCfg.CopyProp( propIfName, pMatch );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg2( ( IConfigDb* )GetCfg() );
        IConfigDb* prhs = pMatch->GetCfg();
        CCfgOpener orhs( prhs );

        ret = oCfg2.CopyProp(
            propDestDBusName, prhs );

        if( ERROR( ret ) )
            break;

        ret = oCfg2.CopyProp( propPortId, prhs );
        if( ERROR( ret ) )
            break;

        stdstr strPath;
        ret = orhs.GetStrProp(
            propRouterPath, strPath );
        if( ERROR( ret ) )
            break;

        oCfg2.SetStrProp( propRouterPath, strPath );
        if( strPath == "/" )
            break;

        // optional
        oCfg.CopyProp( propPrxyPortId, pMatch );

    }while( 0 );

    return ret;
}

stdstr CRouterRemoteMatch::ToString() const
{
    stdstr strAll = super::ToString();

    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );

    guint32 dwPortId = 0;
    gint32 ret = oCfg.GetIntProp(
        propPortId, dwPortId );

    if( ERROR( ret ) )
        return strAll;

    strAll += ",PortId=";
    strAll += std::to_string( dwPortId );

    stdstr strValue;
    oCfg.GetStrProp(
        propDestDBusName, strValue );

    strAll += ",Destination=";
    strAll += strValue;

    ret = oCfg.GetStrProp(
        propRouterPath, strValue );
    if( ERROR( ret ) )
        return strAll;

    strAll += ",RouterPath=";
    strAll += strValue; 

    return strAll;
}

gint32 CRouterRemoteMatch::IsMyReqToRelay(
    IConfigDb* pReqCtx,
    DBusMessage* pReqMsg ) const
{
    gint32 ret =
        super::IsMyMsgOutgoing( pReqMsg );

    if( ERROR( ret ) )
        return ret;
    do{
        DMsgPtr pMsg( pReqMsg );
        CCfgOpener oCfg(
            ( IConfigDb* )GetCfg() );

        stdstr strDest =
            pMsg.GetDestination();

        ret = oCfg.IsEqual(
            propDestDBusName, strDest );

        if( ERROR( ret ) )
            break;

        ret = oCfg.IsEqualProp(
            propRouterPath, pReqCtx );

        if( ERROR( ret ) )
            break;

        guint32 dwPortId = 0;
        CCfgOpener oReqCtx( pReqCtx );
        ret = oReqCtx.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        ret = oCfg.IsEqual(
            propPortId, dwPortId );

        break;

    }while( 0 );

    return ret;
}

gint32 CRouterRemoteMatch::IsMyModEvent(
    DBusMessage* pEvtMsg ) const
{
    DMsgPtr pMsg( pEvtMsg );
    if( pMsg.GetMember() !=
        "NameOwnerChanged" )
        return ERROR_FALSE;

    stdstr strModName;
    gint32 ret = pMsg.GetStrArgAt(
        0, strModName );

    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg(
        ( IConfigDb* )GetCfg() );

    return oCfg.IsEqual(
        propDestDBusName, strModName );
}

gint32 CRouterRemoteMatch::IsMyEvtToForward(
    IConfigDb* pEvtCtx,
    DBusMessage* pEvtMsg ) const
{
    CCfgOpener oCfg(
        ( IConfigDb* )GetCfg() );

    gint32 ret = oCfg.IsEqualProp(
        propRouterPath, pEvtCtx );

    if( ERROR( ret ) )
        return ERROR_FALSE;
    
    ret = super::IsMyMsgIncoming( pEvtMsg );

    if( ERROR( ret ) )
    {
        ret = IsMyModEvent( pEvtMsg );
        if( ERROR( ret ) )
            return ERROR_FALSE;
    }

    return ret;
}

gint32 CRouterRemoteMatch::IsMyModule(
    const stdstr strModName )
{
    if( strModName.empty() )
        return -EINVAL;

    stdstr strModPath =
        DBUS_DESTINATION( strModName );

    if( strModPath.size() >=
        m_strObjPath.size() )
        return ERROR_FALSE;

    std::replace( strModPath.begin(),
        strModPath.end(), '.', '/');

    stdstr strPrefix;
    strPrefix = m_strObjPath.substr(
        0, strModPath.size() );
    
    if( strPrefix != strModPath )
        return ERROR_FALSE;

    stdstr strObjName;
    strObjName = m_strObjPath.substr(
        strModPath.size(),
        m_strObjPath.size() );

    if( strObjName.empty() )
        return ERROR_FALSE;

    if( strObjName[ 0 ] != '/' )
        return ERROR_FALSE;

    strObjName[ 0 ] = 'O';

    if( strObjName.find( '/' ) !=
        stdstr::npos )
        return ERROR_FALSE;

    return 0;
}

guint32 CRouterRemoteMatch::GetPortId()
{
    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );
    guint32 dwPortId = 0;
    oCfg.GetIntProp( propPortId, dwPortId );
    return dwPortId;
}

CRouterLocalMatch::CRouterLocalMatch(
    const IConfigDb* pCfg ) :
    CRouterRemoteMatch( pCfg )
{
    SetClassId( clsid( CRouterLocalMatch ) );
    SetType( matchClient );
}

gint32 CRouterLocalMatch::CopyMatch(
    IMessageMatch* pMatch )
{
    CCfgOpenerObj oCfg( this );
    gint32 ret = 0;

    do{
        ret = oCfg.CopyProp(
            propObjPath, pMatch );
        if( ERROR( ret ) )
            break;

        ret = oCfg.CopyProp(
            propIfName, pMatch );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg2(
            ( IConfigDb* )GetCfg() );
        IConfigDb* prhs = pMatch->GetCfg();

        ret = oCfg2.CopyProp(
            propDestDBusName, prhs );

        if( ERROR( ret ) )
            break;

        ret = oCfg2.CopyProp(
            propRouterPath, prhs );
        if( ERROR( ret ) )
            break;

        ret = oCfg2.CopyProp(
            propSrcDBusName, prhs );
        if( ERROR( ret ) )
            break;

        ret = oCfg2.CopyProp(
            propSrcUniqName, prhs );
        if( ERROR( ret ) )
            break;

        oCfg2.CopyProp(
            propPrxyPortId, prhs );

    }while( 0 );

    return ret;
}

stdstr CRouterLocalMatch::ToString() const
{
    stdstr strAll =
        super::super::ToString();

    stdstr strValue;
    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );

    oCfg.GetStrProp(
        propSrcUniqName, strValue );

    strAll += ",UniqName=";
    strAll += strValue;

    guint32 dwPortId = 0;
    oCfg.GetIntProp(
        propPrxyPortId, dwPortId );

    strAll += ",PrxyPortId=";
    strAll += std::to_string( dwPortId );

    oCfg.GetStrProp(
        propRouterPath, strValue );

    strAll += ",RouterPath=";
    strAll += strValue;

    return strAll;
}

gint32 CRouterLocalMatch::IsMyReqToForward(
    IConfigDb* pReqCtx,
    DBusMessage* pReqMsg ) const
{
    gint32 ret =
        super::IsMyMsgOutgoing( pReqMsg );

    if( ERROR( ret ) )
        return ret;

    CCfgOpener oReqCtx( pReqCtx );
    CCfgOpener oCfg(
        ( IConfigDb* )GetCfg() );

    ret = oCfg.IsEqualProp(
        propRouterPath, pReqCtx );
    if( ERROR( ret ) )
        return ret;
    
    DMsgPtr pMsg( pReqMsg );
    stdstr strVal = pMsg.GetSender();

    ret = oCfg.IsEqual(
        propSrcDBusName, strVal );

    if( ERROR( ret ) )
    {
        ret = oCfg.IsEqual(
            propSrcUniqName, strVal );
    }

    if( ERROR( ret ) )
        return ret;

    return 0;
}

gint32 CRouterLocalMatch::IsMyEvtToRelay(
    IConfigDb* pEvtCtx,
    DBusMessage* pEvtMsg ) const
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetCfg() );

        ret = oCfg.IsEqualProp(
            propRouterPath, pEvtCtx );

        if( ERROR( ret ) )
            break;

        ret = super::IsMyMsgIncoming(
            pEvtMsg );

        if( ERROR( ret ) )
        {
            ret = IsMyModEvent( pEvtMsg );
            if( ERROR( ret ) )
                break;
        }

        CCfgOpener oEvtCtx( pEvtCtx );

        guint32 dwPortId = 0;
        ret = oEvtCtx.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        ret = oCfg.IsEqual(
            propPrxyPortId, dwPortId );

    }while( 0 );

    return ret;
}

gint32 CRouterLocalMatch::IsMyModule(
    const stdstr strModName )
{
    if( strModName.empty() )
        return -EINVAL;

    gint32 ret =
        super::IsMyModule( strModName );

    if( SUCCEEDED( ret ) )
        return ret;

    stdstr strObjPath = strModName;
    std::replace( strObjPath.begin(),
        strObjPath.end(), '.', '/');

    CCfgOpener oCfg( ( IConfigDb* )GetCfg() );
    ret = oCfg.IsEqual( propSrcDBusName,
        strModName );

    if( SUCCEEDED( ret ) )
         return ret;

    ret = oCfg.IsEqual( propDestDBusName,
        strModName );

    if( SUCCEEDED( ret ) )
         return ret;

    return ERROR_FALSE;
}

}
