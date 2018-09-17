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
#include "dmsgptr.h"
#include "ifhelper.h"

stdstr CastTo( BufPtr& pBuf )
{
    std::string strVal( pBuf->ptr() );
    return strVal;
}

template<>
BufPtr& CastTo< BufPtr >( BufPtr& i )
{
    return i;
}

template<>
BufPtr PackageTo< DMsgPtr >( const DMsgPtr& pMsg )
{
    BufPtr pBuf( true );
    *pBuf = pMsg;
    return pBuf;
}

template<>
BufPtr PackageTo< ObjPtr >( const ObjPtr& pObj )
{
    BufPtr pBuf( true );
    *pBuf = pObj;
    return pBuf;
}

template<>
BufPtr PackageTo< BufPtr >( const BufPtr& pObj )
{
    return const_cast<BufPtr&>( pObj );
}

template<>
BufPtr PackageTo< CObjBase >( const CObjBase* pObj )
{
    BufPtr pBuf( true );
    ObjPtr ptrObj( const_cast< CObjBase* >( pObj ) );
    *pBuf = ptrObj;
    return pBuf;
}

template<>
BufPtr PackageTo< DBusMessage >( const DBusMessage* pMsg )
{
    BufPtr pBuf( true );
    *pBuf = DMsgPtr( const_cast< DBusMessage*>( pMsg ) );
    return pBuf;
}

template<>
BufPtr PackageTo< char >( const char* pText )
{
    BufPtr pBuf( true );
    *pBuf = stdstr( pText );
    return pBuf;
}

template<>
BufPtr PackageTo< stdstr >( const stdstr& str )
{
    BufPtr pBuf( true );
    *pBuf = str;
    return pBuf;
}

template<>
auto VecToTuple<>( std::vector< BufPtr >& vec ) -> std::tuple<> 
{
    return std::tuple<>();
}

