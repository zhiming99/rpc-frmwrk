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
 * =====================================================================================
 */
#include <vector>
#include <string>
#include "defines.h"
#include "frmwrk.h"
#include "portdrv.h"
#include "emaphelp.h"

using namespace std;

// driver method
gint32 CPortDriver::GetProperty(
    gint32 iProp, CBuffer& oBuf ) const
{
    CStdRMutex oDrvLock(
         const_cast< CPortDriver* >( this )->GetLock() );

    return m_pCfgDb->GetProperty( iProp, oBuf );
}

gint32 CPortDriver::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{

    CStdRMutex oDrvLock( GetLock() );
    return m_pCfgDb->SetProperty( iProp, oBuf );
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

    // lable the port is removed
    a.SetIntProp(
        propPortState, PORT_STATE_REMOVED );

    m_mapIdToPort.erase( propPortId );
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
