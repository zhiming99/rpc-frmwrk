/*
 * =====================================================================================
 *
 *       Filename:  dbusdrv.cpp
 *
 *    Description:  dbus bus driver implementation
 *
 *        Version:  1.0
 *        Created:  08/21/2016 01:22:49 PM
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
#include "dbusport.h"
#include "emaphelp.h"
// #include "iftasks.h"


using namespace std;

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

gint32 CGenBusDriver::Stop()
{
    gint32 ret = 0;

    do
    {
        if( GetIoMgr() == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( GetIoMgr()->RunningOnMainThread() )
        {
            ret = ERROR_STATE;
            break;
        }

        // Stop all the ports
        vector<PortPtr> vecPorts;
        ret = EnumPorts( vecPorts );
        if( ERROR( ret ) )
            break;

        CPnpManager& oPnpMgr =
            GetIoMgr()->GetPnpMgr();

        for( guint32 i = 0; i < vecPorts.size(); i++ )
        {
            ret = oPnpMgr.StopPortStack(
                vecPorts[ i ] );

            if( SUCCEEDED( ret ) )
            {
                ret = oPnpMgr.DestroyPortStack(
                    vecPorts[ i ] );
            }
        }

        // stopping the child ports is done
        // in CGenericBusPort

        // let the base class to unreg the driver
        ret = super::Stop();

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

CDBusBusDriver::CDBusBusDriver(
    const IConfigDb* pCfg ): 
    super( pCfg )
{
    SetClassId( clsid( CDBusBusDriver ) );
}

gint32 CDBusBusDriver::Probe(
        IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig )
{
    gint32 ret = 0;

    do{
        // we don't have dynamic bus yet
        CfgPtr pCfg( true );

        // make a copy of the input args
        if( pConfig != nullptr )
            *pCfg = *pConfig;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        ret = oCfg.SetStrProp( propPortClass,
            PORT_CLASS_LOCALDBUS );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetIntProp( propClsid,
            clsid( CDBusBusPort ) );

        // FIXME: we need a better task to receive the
        // notification of the port start events
        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;

        // we don't care the event sink to bind
        ret = oCfg.SetObjPtr( propEventSink,
            ObjPtr( pTask ) );

        if( ERROR( ret ) )
            break;

        ret = CreatePort( pNewPort, pCfg );
        if( ERROR( ret ) )
            break;

        if( pLowerPort != nullptr )
        {
            ret = pNewPort->AttachToPort( pLowerPort );
        }

        if( SUCCEEDED( ret ) )
        {
            CEventMapHelper< CPortDriver > a( this );
            a.BroadcastEvent(
                eventPnp,
                eventPortAttached,
                PortToHandle( pNewPort ),
                nullptr );
        }

        // waiting for the start to complete
        CSyncCallback* pSyncTask = pTask;
        if( pSyncTask != nullptr )
        {
            pSyncTask->WaitForComplete();
        }

    }while( 0 );

    return ret;
}

