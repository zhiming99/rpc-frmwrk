/*
 * =====================================================================================
 *
 *       Filename:  portdrv.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/04/2016 04:26:33 PM
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
#include <vector>
#include <string>
#include "defines.h"
#include "frmwrk.h"
#include "portdrv.h"
#include "emaphelp.h"
#include "iftasks.h"

namespace rpcf
{

using namespace std;

// driver method
gint32 CPortDriver::GetProperty(
    gint32 iProp,
    Variant& oVar ) const
{
    CStdRMutex oDrvLock(
         const_cast< CPortDriver* >( this )->GetLock() );

    return m_pCfgDb->GetProperty( iProp, oVar );
}

gint32 CPortDriver::SetProperty(
    gint32 iProp,
    const Variant& oVar )
{

    CStdRMutex oDrvLock( GetLock() );
    return m_pCfgDb->SetProperty( iProp, oVar );
}

gint32 CPortDriver::GetDrvName(
    string& strDrvName ) const
{

    CStdRMutex oDrvLock(
         const_cast< CPortDriver* >( this )->GetLock() );

    gint32 ret = 0;
    CCfgOpener a( ( IConfigDb* )m_pCfgDb );
    ret = a.GetStrProp( propDrvName, strDrvName );

    if( ERROR( ret ) )
        return ret;

    return ret;
}

gint32 CPortDriver::EnumPorts(
        vector<PortPtr>& vecPorts ) const
{
    CStdRMutex oDrvLock(
         const_cast< CPortDriver* >( this )->GetLock() );

    map< guint32, PortPtr >::const_iterator itr =
        m_mapIdToPort.begin();

    while( itr != m_mapIdToPort.end() )
    {
        vecPorts.push_back( itr->second );
        ++itr;
    }
    return 0;
}

CPortDriver::CPortDriver( const IConfigDb* pCfg ) :
    m_atmNewPortId( 0 ),
    m_pCfgDb( true ),
    m_dwFlags( DRV_STATE_STOPPED | DRV_TYPE_FUNCDRV )
{
    *m_pCfgDb = *pCfg;
    CCfgOpener oCfg( pCfg );

    gint32 ret = oCfg.GetPointer(
        propIoMgr, m_pIoMgr );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( -EINVAL,
            "IoMgr ptr is nullptr in CPortDriver" );

        throw std::invalid_argument( strMsg );
    }
}

CPortDriver::~CPortDriver()
{
}

gint32 CPortDriver::RemovePort( IPort* pPort )
{
    if( pPort == nullptr )
        return -EINVAL;

    CStdRMutex oDrvLock( GetLock() );
    gint32 ret = -ENOENT;
    CCfgOpenerObj a( pPort );

    guint32 dwPortId = 0;
    ret = a.GetIntProp( propPortId, dwPortId );
    if( ERROR( ret ) )
        return ret;

    m_mapIdToPort.erase( dwPortId );
    return ret;
}

gint32 CPortDriver::AddPort( IPort* pPort )
{
    if( pPort == nullptr )
        return -EINVAL;

    CStdRMutex oDrvLock( GetLock() );
    gint32 ret = -ENOENT;
    CCfgOpenerObj a( pPort );

    guint32 dwPortId = 0;
    ret = a.GetIntProp( propPortId, dwPortId );
    if( ERROR( ret ) )
        return ret;

    if( m_mapIdToPort.find( dwPortId )
        != m_mapIdToPort.end() )
    {
        return -EEXIST;
    }

    m_mapIdToPort[ dwPortId ] = PortPtr( pPort );

    return ret;
}

gint32 CPortDriver::Start()
{
    gint32 ret = 0;

    do{
        // register the pnp manager as the event
        // subscriber make sure the driver has
        // already successfully registered
        if( GetIoMgr() == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // register with the DriverManager
        CDriverManager& oDrvMgr =
            GetIoMgr()->GetDrvMgr();

        // test if the driver is a bus driver
        IBusDriver* pBusDrv =
            dynamic_cast< IBusDriver* >( this );

        if( pBusDrv == nullptr )
            ret = oDrvMgr.RegPortDrv( this );
        else
            ret = oDrvMgr.RegBusDrv( pBusDrv );

        if( ERROR( ret ) )
            break;

        CEventMapHelper< CPortDriver >
            oEvtHelper( ( CPortDriver* )this );

        // add pnp manager as the event subscriber
        EventPtr pEventSink( &GetIoMgr()->GetPnpMgr() );
        ret = oEvtHelper.SubscribeEvent( pEventSink );

        if( ERROR( ret ) )
        {
            if( pBusDrv != nullptr )
                oDrvMgr.UnRegBusDrv( pBusDrv );
            else
                oDrvMgr.UnRegPortDrv( this );
        }

        SetDrvState( DRV_STATE_READY );

    }while( 0 );

    return ret;
}

gint32 CPortDriver::Stop()
{
    gint32 ret = 0;

    do{
        // stopping the child ports is done
        // in CGenericBusPort
        //
        // unregister with the DriverManager
        CDriverManager& oDrvMgr =
            GetIoMgr()->GetDrvMgr();

        IBusDriver* pBusDrv =
            dynamic_cast< IBusDriver* >( this );

        SetDrvState( DRV_STATE_STOPPED );

        if( pBusDrv == nullptr )
            ret = oDrvMgr.UnRegPortDrv( this );
        else
            ret = oDrvMgr.UnRegBusDrv( pBusDrv );

        if( ERROR( ret ) )
            break;

        if( ret == -ENOENT )
        {
            // fine
            ret = 0;
            break;
        }
        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

CGenBusDriver::CGenBusDriver(
    const IConfigDb* pCfg )
    : super( pCfg )
{

    gint32 ret = 0;

    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        string strDrvName;

        CCfgOpener a( pCfg ), b( ( IConfigDb* )m_pCfgDb );

        ret = a.GetPointer( propIoMgr, m_pIoMgr );
        if( ERROR( ret ) )
            break;

        // we don't have a standalone variable
        // for the driver name, put it in the
        // config db
        ret = b.CopyProp( propDrvName, pCfg );

        if( ERROR( ret ) )
            break;

        SetDrvType( DRV_TYPE_BUSDRV );

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret, 
            "error happens in constructor" );

        throw std::runtime_error( strMsg );
    }
}

gint32 CGenBusDriver::Start()
{
    gint32 ret = 0;

    do
    {
        ret = super::Start();

        if( ERROR( ret ) )
            break;

        // create all the static ports
        // specified in the config file, in our
        // case, just one.
        PortPtr pNewPort;

        // this is the local dbus port
        ret = Probe( nullptr, pNewPort, nullptr );
        if( ERROR( ret ) )
            break;

        // send a message th pnp manager to
        // start building the port stack
        
    }while( 0 );
    return ret;
}

gint32 CGenBusDriver::CreatePort(
    PortPtr& pNewPort,
    const IConfigDb* pConfig )
{

    gint32 ret = 0;
    CfgPtr pCfg( true );
    do{
        
        if( pCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        if( pConfig != nullptr )
            *pCfg = *pConfig;

        CCfgOpener a( ( IConfigDb* )pCfg );
        ObjPtr objPtr( GetIoMgr() );

        // a pointer to iomanager
        ret = a.SetPointer( propIoMgr, GetIoMgr() );
        if( ERROR( ret ) )
            break;

        // assign the port id
        guint32 dwPortId = 0;
        if( pCfg->exist( propPortId ) )
        {
            ret = a.GetIntProp(
                propPortId, dwPortId );
        }
        else
        {
            dwPortId = NewPortId();
            ret = a.SetIntProp(
                propPortId, dwPortId );
        }

        if( ERROR( ret ) )
            break;

        string strClass;

        ret = a.GetStrProp(
            propPortClass, strClass );

        if( ERROR( ret ) )
            break;

        // c++11 required to_string
        string strPortName =
            strClass
            + string( "_" )
            + std::to_string( dwPortId );

        // the port name to pass
        ret = a.SetStrProp(
            propPortName, strPortName );

        if( ERROR( ret ) )
            break;

        // the pointer to the driver
        objPtr = this;
        ret = a.SetObjPtr( propDrvPtr, objPtr );
        if( ERROR( ret ) )
            break;

        guint32 dwClsid = 0;
        ret = a.GetIntProp(
            propClsid, dwClsid );

        if( ERROR( ret ) )
            break;

        // create the bus port
        PortPtr pBusFdo;
        ret = pBusFdo.NewObj(
            ( EnumClsid )dwClsid, pCfg );

        if( ERROR( ret ) )
            break;

        ret = AddPort( pBusFdo );
        if( ERROR( ret ) )
            break;

        pNewPort = pBusFdo;

        // NOTE: the port `Start' will be called
        // when the port stack is built

        // we are done here

    }while( 0 );

    return ret;
}

gint32 CGenBusDriver::RemovePort(
    IPort* pChildPort )
{
    if( pChildPort == nullptr )
        return -EINVAL;

    CStdRMutex oDrvLock( GetLock() );
    gint32 ret = super::RemovePort( pChildPort );
    if( ERROR( ret ) )
        return ret;

    PortPtr pPort( pChildPort );
    auto itr = m_mapPort2TaskGrp.find( pPort );
    if( itr == m_mapPort2TaskGrp.end() )
        return -ENOENT;

    // NOTE: don't cancel the taskgroup, and let
    // it complete
    m_mapPort2TaskGrp.erase( itr );
    return ret;
}

gint32 CGenBusDriver::AddPort(
    IPort* pNewPort )
{
    if( pNewPort == nullptr )
        return -EINVAL;
    CStdRMutex oDrvLock( GetLock() );
    gint32 ret = super::AddPort( pNewPort );
    if( ERROR( ret ) )
        return ret;

    TaskletPtr pTask;
    PortPtr pPort( pNewPort );
    m_mapPort2TaskGrp[ pPort ] = pTask;
    return ret;
}

}
