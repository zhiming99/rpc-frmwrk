/*
 * =====================================================================================
 *
 *       Filename:  portdrv.h
 *
 *    Description:  declaration of the class for port driver
 *
 *        Version:  1.0
 *        Created:  10/04/2016 03:45:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#pragma once

#include <vector>
#include "defines.h"
#include "frmwrk.h"
#include "port.h"

#define DRV_STATE_READY     0x01
#define DRV_STATE_STOPPED   0x02
#define DRV_STATE_MASK      0x03

#define DRV_TYPE_BUSDRV     0x04
#define DRV_TYPE_FUNCDRV    0x08
#define DRV_TYPE_MASK       0x0c

#define SetDrvState( stat_ ) \
    m_dwFlags = ( ( m_dwFlags & ~DRV_STATE_MASK ) | ( ( stat_ ) & DRV_STATE_MASK ) )

#define GetDrvState( stat_ ) \
    ( m_dwFlags & ~DRV_STATE_MASK )

#define SetDrvType( type_ ) \
    m_dwFlags = ( ( m_dwFlags & ~DRV_TYPE_MASK ) | ( ( type_ ) & DRV_TYPE_MASK ) )

#define GetDrvType( type_ ) \
    ( m_dwFlags & ~DRV_TYPE_MASK )

class IPortDriver : public IService
{
	public:

	// PnP Events;
    // actively find the driver and
    // creating a device with the driver
	virtual gint32 Probe(
            IPort* pLowerPort,
            PortPtr& pNewPort,
            const IConfigDb* pConfig = NULL ) = 0;

    virtual gint32 AddPort( IPort* pPort ) = 0;

	virtual gint32 RemovePort( IPort* pPort ) = 0;

    // create a new port from nowhere
    virtual gint32 CreatePort(
            PortPtr& pNewPort,
            const IConfigDb* pConfig = NULL) = 0;

    virtual gint32 GetDrvName(
            std::string& strName ) const = 0;

    virtual gint32 EnumPorts(
            std::vector<PortPtr>& vecPorts ) const = 0;
};

typedef CAutoPtr< Clsid_Invalid, IPortDriver > PortDrvPtr;

class CPortDriver : public IPortDriver
{
    protected:

    // auto-increment id counter
    std::atomic<guint32>            m_atmNewPortId;

    // ports belongs to this driver
    // the refernce count held by this
    // PortPtr should be the last before
    // the port get removed
    std::map< guint32, PortPtr >    m_mapIdToPort;
    CfgPtr                          m_pCfgDb;
    CIoManager                      *m_pIoMgr;

    stdrmutex                       m_oLock;
    guint32                         m_dwFlags;

    public:

    typedef  IPortDriver super;

    CPortDriver( const IConfigDb* pCfg = nullptr );
    ~CPortDriver();

    inline guint32 NewPortId()
    { return m_atmNewPortId++; }

    stdrmutex& GetLock()
    { return m_oLock; }

    gint32 GetProperty(
            gint32 iProp, CBuffer& oBuf ) const;

    gint32 SetProperty(
            gint32 iProp, const CBuffer& oBuf );

    gint32 EnumPorts(
            std::vector<PortPtr>& vecPorts ) const;

    gint32 GetDrvName(
            std::string& strName ) const;

    gint32 RemovePort( IPort* pPort );

	gint32 AddPort( IPort* pNewPort );

    inline CIoManager* GetIoMgr() const
    { return m_pIoMgr; }

    gint32 Start();

    gint32 Stop();
};

class IBusDriver : public CPortDriver
{
    public:

    typedef CPortDriver super;
    IBusDriver( const IConfigDb* pCfg = nullptr )
    : super( pCfg )
    { }
};

typedef CAutoPtr< Clsid_Invalid, IBusDriver > BusDrvPtr;

