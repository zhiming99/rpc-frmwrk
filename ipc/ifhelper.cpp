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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
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
BufPtr PackageTo< CBuffer >( CBuffer* pObj )
{
    BufPtr pBuf( pObj );
    return pBuf;
}

template<>
auto VecToTuple<>( std::vector< BufPtr >& vec ) -> std::tuple<> 
{
    return std::tuple<>();
}

void AssignVal( DMsgPtr& rVal, CBuffer& rBuf )
{
    rVal = ( DMsgPtr& )rBuf;
}

template< int iInputNum, typename...Types >
inline void PackOutput(
    std::vector< BufPtr >& vec, Types&&...args )
{
    // note that the last arg is inserted first
    gint32 iOutputNum = sizeof...( args ) - iInputNum;
    _DummyClass_::PackExp(
        ( iOutputNum > vec.size() ? ( vec.insert( vec.begin(), PackageTo( args ) ), 1 ) : 0 ) ... );
}

template< int iInputNum, typename...Types >
inline void PackInput(
    std::vector< BufPtr >& vec, Types&&...args )
{
    // note that the last arg is inserted first
    int iCount = 0;
    gint32 iOutputNum = sizeof...( args ) - iInputNum;
    _DummyClass_::PackExp(
        ( iOutputNum >= iCount++ ? ( vec.insert( vec.begin(), PackageTo( args ) ), 1 ) : 0 ) ... );
}
