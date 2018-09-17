/*
 * =====================================================================================
 *
 *       Filename:  creatins.cpp
 *
 *    Description:  implementation of some global functions
 *
 *        Version:  1.0
 *        Created:  06/10/2016 08:53:22 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */
#include <deque>
#include <map>
#include "stlcont.h"
#include "objfctry.h"

FctryVecPtr g_pFactories;

/**
* @name CoCreateInstance create an instance
* of the given clsid, and store the pointer to
* pObj, with the given parameters from pCfg
* @{ it can catch exceptions thrown from the
* object constructor and make proper handling. it
* is OK if some of the object don't need pCfg in
* there constructors */
/** clsid: the class id of the object to create
 *  pObj: to hold the created object
 *  pCfg: more parameters to pass to the object's
 *  constructor.
 *  return value: return -ENOTSUP if cannot find
 *  the given class id. 0 for successful creation,
 *  and negative number for otherwise errors @} */

gint32 CoCreateInstance(
    EnumClsid iClsid, CObjBase*& pObj, const IConfigDb* pCfg ) 
{
    gint32 ret = 0;

    if( iClsid == clsid( Invalid ) )
        return -EINVAL;

    try
    {
        ret = g_pFactories->CreateInstance(
            iClsid, pObj, pCfg );
    }
    catch( const std::out_of_range& e )
    {
        ret = -ENOENT;
    }
    catch( const std::bad_alloc& e )
    {
        ret = -ENOMEM;
    }
    catch( const std::runtime_error& e )
    {
        ret = -EFAULT;
    }
    catch( const std::invalid_argument& e )
    {
        ret = -EINVAL;
    }

    return ret;
}

