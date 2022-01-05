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

namespace rpcf
{

stdstr CastTo( BufPtr& pBuf )
{
    std::string strVal( pBuf->ptr() );
    return strVal;
}

template<>
BufPtr& CastTo< BufPtr >( Variant& i )
{
    return i;
}

template<>
Variant PackageTo< DMsgPtr >( const DMsgPtr& pMsg )
{
    Variant o( pMsg );
    return o;
}

template<>
Variant PackageTo< ObjPtr >( const ObjPtr& pObj )
{
    Variant o( pObj );
    return o;
}

template<>
Variant PackageTo< BufPtr >( const BufPtr& pObj )
{
    Variant o( pObj );
    return o;
}

template<>
Variant PackageTo< CObjBase >( const CObjBase* pObj )
{
    ObjPtr ptrObj( const_cast< CObjBase* >( pObj ) );
    Variant o( ptrObj );
    return o;
}

template<>
Variant PackageTo< DBusMessage >( const DBusMessage* pMsg )
{
    DMsgPtr p( const_cast< DBusMessage* >( pMsg ) );
    return Variant( p );
}

template<>
Variant PackageTo< char >( const char* pText )
{
    Variant o( pText );
    return o;
}

template<>
Variant PackageTo< stdstr >( const stdstr& str )
{
    Variant o( str );
    return o;
}

template<>
Variant PackageTo< CBuffer >( CBuffer* pObj )
{
    BufPtr p( pObj );
    return Variant( p );
}

template<>
auto VecToTuple<>( std::vector< Variant >& vec ) -> std::tuple<> 
{
    return std::tuple<>();
}

void AssignVal( DMsgPtr& rVal, Variant& oVar )
{
    rVal = ( DMsgPtr& )oVar;
}

}
