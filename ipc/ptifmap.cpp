/*
 * =====================================================================================
 *
 *       Filename:  ptifmap.cpp
 *
 *    Description:  implementation of CPortInterfaceMap
 *
 *        Version:  1.0
 *        Created:  10/08/2016 05:55:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include <vector>
#include <string>
#include "defines.h"
#include "port.h"
#include "frmwrk.h"

using namespace std;

CPortInterfaceMap::CPortInterfaceMap()
{
    SetClassId( clsid( CPortInterfaceMap ) );
}

CPortInterfaceMap::~CPortInterfaceMap()
{

}

gint32 CPortInterfaceMap::AddToHandleMap(
    IPort* pPort, IEventSink* pEvent )
{
    gint32 ret = 0;

    do{
        // NOTE: we allow empty pEvent
        // so that the port can register
        // itself to this map
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CStdRMutex a( GetLock() );

        PortPtr portPtr( pPort );
        EventPtr evtPtr( pEvent );

        if( pEvent != nullptr )
        {
            // we need to make sure the port has
            // already registered at least with <
            // PortPtr, nullptr >
            if( m_mapPort2If.find( portPtr ) == m_mapPort2If.end() )
            {
                // the port is not registered yet,
                // and cannot be used
                ret = -ENOENT;
                break;
            }
        }

        m_mapPort2If.insert(
            pair< PortPtr, EventPtr >( portPtr, evtPtr ) );

        m_mapIf2Port.insert(
            pair< EventPtr, PortPtr >( evtPtr, portPtr ) );

    }while( 0 );

    return ret;
}

gint32 CPortInterfaceMap::RemoveFromHandleMap(
    IPort* pPort, IEventSink* pEvent )
{

    gint32 ret = -ENOENT;
    bool bExist1 = false;
    bool bExist2 = false;
    do{
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CStdRMutex a( GetLock() );
        PortPtr portPtr( pPort );
        EventPtr evtPtr( pEvent );

        PEPAIR result = m_mapPort2If.equal_range( portPtr );
        multimap< PortPtr, EventPtr >::iterator itr1;
        for( itr1 = result.first; itr1 != result.second; ++itr1 )
        {
            if( ( ( IEventSink* )itr1->second ) == pEvent )
            {
                bExist1 = true;
                break;
            }
        }
        if( bExist1 )
        {
            m_mapPort2If.erase( itr1 );
            EPPAIR result2 = m_mapIf2Port.equal_range( evtPtr );
            multimap< EventPtr, PortPtr >::iterator itr2;
            for( itr2 = result2.first; itr2 != result2.second; ++itr2 )
            {
                if( pPort == itr2->second )
                {
                    bExist2 = true;
                    break;
                }
            }
            if( bExist2 )
            {
                m_mapIf2Port.erase( itr2 );
                ret = 0;
            }
            else
            {
                // bad integrity
                ret = ERROR_STATE;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CPortInterfaceMap::RemoveFromHandleMap(
    IEventSink* pEvent, IPort* pPort )
{

    gint32 ret = 0;
    bool bExist1 = false;
    bool bExist2 = false;
    do{
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CStdRMutex a( GetLock() );
        PortPtr portPtr( pPort );
        EventPtr evtPtr( pEvent );

        EPPAIR result = m_mapIf2Port.equal_range( evtPtr );
        multimap< EventPtr, PortPtr >::iterator itr1;
        for( itr1 = result.first; itr1 != result.second; ++itr1 )
        {
            if( pPort == ( IPort* )itr1->second )
            {
                bExist1 = true;
                break;
            }
        }
        if( bExist1 )
        {
            m_mapIf2Port.erase( itr1 );
            PEPAIR result2 = m_mapPort2If.equal_range( portPtr );
            multimap< PortPtr, EventPtr >::iterator itr2;
            for( itr2 = result2.first; itr2 != result2.second; ++itr2 )
            {
                // we cannot swap the two operand, because the
                // implicit conversion won't happen if `itr2->second'
                // comes first, otherwise, we need an explicit 
                // type casting for `itr2->second'
                if( pEvent == itr2->second)
                {
                    bExist2 = true;
                    break;
                }
            }
            if( bExist2 )
            {
                m_mapPort2If.erase( itr2 );
                ret = 0;
            }
            else
            {
                // bad integrity
                ret = ERROR_STATE;
            }
        }

    }while( 0 );

    return ret;
}
gint32 CPortInterfaceMap::PortExist(
    IPort* pPort, vector< EventPtr >* pvecVals ) const
{

    gint32 ret = 0;
    do{
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CStdRMutex a( GetLock() );
        PortPtr portPtr( pPort );
        if( m_mapPort2If.find( portPtr ) == m_mapPort2If.end() )
        {
            ret = -ENOENT;
            break;
        }
        CPEPAIR result = m_mapPort2If.equal_range( portPtr );
        if( pvecVals != nullptr )
        {
            multimap< PortPtr, EventPtr >::const_iterator itr;
            for( itr = result.first; itr != result.second; ++itr )
            {
                pvecVals->push_back( itr->second );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CPortInterfaceMap::InterfaceExist(
    IEventSink* pEvent, vector< PortPtr >* pvecVals ) const
{

    gint32 ret = 0;
    do{
        CStdRMutex a( GetLock() );
        EventPtr evtPtr( pEvent );

        if( m_mapIf2Port.find( evtPtr ) == m_mapIf2Port.end() ) 
        {
            ret = -ENOENT;
            break;
        }

        CEPPAIR result = m_mapIf2Port.equal_range( evtPtr );
        if( pvecVals != nullptr )
        {
            multimap< EventPtr, PortPtr >::const_iterator itr;
            for( itr = result.first; itr != result.second; ++itr )
            {
                pvecVals->push_back( itr->second );
            }
        }
    }while( 0 );

    return ret;
}
