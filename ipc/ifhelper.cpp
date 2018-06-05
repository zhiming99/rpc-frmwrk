/*
 * =====================================================================================
 *
 *       Filename:  ifhelper.cpp
 *
 *    Description:  implementation of the helper methods for building proxy/server interface
 *
 *        Version:  1.0
 *        Created:  01/13/2018 07:30:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */


#include <deque>
#include <vector>
#include <string>
#include "defines.h"
#include "frmwrk.h"
#include "proxy.h"
#include "dmsgptr.h"

template<>
CObjBase* CastTo< CObjBase* >( BufPtr& i )
{
    return ( CObjBase* )*i;
}

template<>
DBusMessage* CastTo< DBusMessage* >( BufPtr& i )
{
    return ( ( DMsgPtr& )*i );
}

template<>
BufPtr CastTo< BufPtr >( BufPtr& i )
{
    return i;
}

template<>
BufPtr PackageTo< DMsgPtr >( DMsgPtr& pMsg )
{
    BufPtr pBuf( true );
    if( !pMsg.IsEmpty() )
        *pBuf = pMsg;
    return pBuf;
}

template<>
BufPtr PackageTo< ObjPtr >( ObjPtr& pObj )
{
    BufPtr pBuf( true );
    if( !pObj.IsEmpty() )
        *pBuf = pObj;
    return pBuf;
}

template<>
BufPtr PackageTo< CObjBase* >( CObjBase*& pObj )
{
    BufPtr pBuf( true );
    if( pObj != nullptr )
        *pBuf = ObjPtr( pObj );
    return pBuf;
}

template<>
BufPtr PackageTo< DBusMessage* >( DBusMessage*& pMsg )
{
    BufPtr pBuf( true );
    if( pMsg != nullptr )
        *pBuf = DMsgPtr( pMsg );
    return pBuf;
}

template<>
BufPtr PackageTo< stdstr >( stdstr& str )
{
    BufPtr pBuf( true );
    if( !str.empty() )
        *pBuf = str;
    return pBuf;
}

template<>
BufPtr PackageTo< float >( float& fVal )
{
    BufPtr pBuf( true );
    guint64 val = 0;
    guint32* pSrcVal = ( guint32* )&fVal;
    guint32* pDestVal = ( guint32* )&val;
    *pDestVal = *pSrcVal;
    *pBuf = val;
    return pBuf;
}

template<>
auto VecToTuple<>( std::vector< BufPtr >& vec ) -> std::tuple<> 
{
    return std::tuple<>();
}

