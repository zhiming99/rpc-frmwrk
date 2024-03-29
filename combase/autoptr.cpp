/*
 * =====================================================================================
 *
 *       Filename:  autoptr.cpp
 *
 *    Description:  implementation of part of the autoptr methods
 *
 *        Version:  1.0
 *        Created:  12/07/2016 08:22:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
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

#include <dbus/dbus.h>
#include <vector>
#include <string>
#include <map>
#include "defines.h"
#include "autoptr.h"
#include "buffer.h"
#include "configdb.h"

namespace rpcf
{

gint32 CreateObjFast( EnumClsid iClsid,
    CObjBase*& pObj, const IConfigDb* pCfg ) 
{
    if( iClsid == clsid( CBuffer ) )
    {
        pObj = new( std::nothrow ) CBuffer();
    }
    else if( iClsid == clsid( CConfigDb2 ) )
    {
        pObj = new( std::nothrow ) CConfigDb2( pCfg );    
    }
    else
    {
        return  -ENOTSUP;
    }

    // NOTE: we have a reference count already, no need
    // to AddRef again
    if( unlikely( pObj == nullptr ) )
        return -ENOMEM;

    return STATUS_SUCCESS;
}

gint32 DeserializeObj( const CBuffer& oBuf, ObjPtr& pObj )
{
    const SERI_HEADER_BASE* pHeader = oBuf;

    SERI_HEADER_BASE oHeader( *pHeader );
    oHeader.ntoh();

    gint32 ret = 0;

    switch( oHeader.dwClsid )
    {
    case clsid( CBuffer ): 
        {
            BufPtr pObjBuf( true );
            ret = pObjBuf->Deserialize( oBuf );
            if( SUCCEEDED( ret ) )
            {
                if( pObjBuf->type() != DataTypeObjPtr )
                    ret = -EFAULT;
                pObj = ( ObjPtr& )*pObjBuf;
            }
            break;
        }
    case clsid( CConfigDb2 ):
        {
            CfgPtr pCfg( true );
            ret = pCfg->Deserialize( oBuf );
            if( ERROR( ret ) )
                break;

            pObj = pCfg;
            break;
        }
    default:
        {
            ObjPtr pObjTemp;
            ret = pObjTemp.NewObj(
                ( EnumClsid )oHeader.dwClsid );

            if( ERROR( ret ) )
                break;

            ret = pObjTemp->Deserialize( oBuf );
            if( ERROR( ret ) )
                break;

            pObj = pObjTemp;
            break;
        }
    }

    return ret;
}

}
