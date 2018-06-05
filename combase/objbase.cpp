/*
 * =====================================================================================
 *
 *       Filename:  objbase.cpp
 *
 *    Description:  implementation of class CObjBase and other base classes
 *
 *        Version:  1.0
 *        Created:  04/12/2016 08:57:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include <string>
#include <vector>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "defines.h"
#include "autoptr.h"
#include "buffer.h"

inline gint64 GetRandom()
{
    timeval tv;
    gettimeofday( &tv, NULL );
    gint64 iPid = getpid();
    iPid <<=32;
    // don't care if tv initialized or not
    // or gettimeofday succeeded or not
    return tv.tv_usec + iPid;
}

std::atomic< guint64 >
    CObjBase::m_atmObjCount( GetRandom() );

CObjBase::CObjBase()
    : m_atmRefCount( 0 )
{
    m_dwClsid = Clsid_Invalid;
    m_qwObjId = ++m_atmObjCount;
#ifdef DEBUG
    m_dwMagic = *( guint32* )"ObjB";
#endif
}

// copy constructor
CObjBase::CObjBase( const CObjBase& rhs )
    : m_atmRefCount( 0 )
{
    m_dwClsid = rhs.m_dwClsid;
    m_qwObjId = ++m_atmObjCount;

#ifdef DEBUG
    m_dwMagic = *( guint32* )"ObjB";
#endif
}

CObjBase::~CObjBase()
{
}

gint32 CObjBase::SetClassId(
    EnumClsid iClsid )
{
    m_dwClsid = iClsid;
    return 0;
}

gint32 CObjBase::AddRef()
{
    return ++m_atmRefCount;  
}

gint32 CObjBase::Release()
{
    gint32 iRef = --m_atmRefCount;
    if( iRef <= 0 )
        delete this;
    return iRef;
}

EnumClsid CObjBase::GetClsid() const
{
    return m_dwClsid;
}

guint64 CObjBase::GetObjId() const
{
    return m_qwObjId;
}

const char* CObjBase::GetClassName() const
{
    return CoGetClassName( m_dwClsid );
}

gint32 CObjBase::Serialize(
    CBuffer& oBuf ) const
{
    return -ENOTSUP;
}

gint32 CObjBase::Deserialize(
    const char* pBuf, guint32 dwSize )
{
    return -ENOTSUP;
}

gint32 CObjBase::Deserialize(
    const CBuffer& obuf )
{
    return -ENOTSUP;
}

gint32 CObjBase::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    vecProps.push_back( propObjId );
    vecProps.push_back( propRefCount );
    vecProps.push_back( propClsid );
    return 0;
}

gint32 CObjBase::GetProperty(
    gint32 iProp, CBuffer& oBuf ) const
{
    gint32 ret = 0;

    switch( iProp )
    {
    case propObjId:
        oBuf = GetObjId();
        break;

    case propRefCount:
        oBuf = ( guint32 )m_atmRefCount;
        break;

    case propClsid:
        oBuf = ( guint32 )GetClsid();
        break;

    default:
        ret = -ENOENT;
    }
    return ret;
}

gint32 CObjBase::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{

    gint32 ret = 0;
    switch( iProp )
    {
    case propObjId:
        iType = typeUInt64;
        break;

    case propClsid:
    case propRefCount:
        iType = typeUInt32;
        break;

    default:
        ret = -ENOENT;
    }
    return ret;
}

gint32 CObjBase::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{
    gint32 ret = 0;

    switch( iProp )
    {
    default:
        ret = -ENOENT;
    }

    return ret;
}

gint32 CObjBase::RemoveProperty(
    gint32 iProp )
{
    return -ENOTSUP;
}

gint32 CObjBase::Intitialize(
    const IConfigDb* pCfg )
{
    return 0;
}

