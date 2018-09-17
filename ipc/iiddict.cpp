/*
 * =====================================================================================
 *
 *       Filename:  iiddict.cpp
 *
 *    Description:  implementation of an Iid and Interface name dictionary
 *
 *        Version:  1.0
 *        Created:  08/17/2018 08:49:44 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */
#include "rpc.h"
#include "iiddict.h"

EnumClsid CInterfIdDict::GetIid(
    const std::string& strIfName )
{
    CStdMutex oLock( m_oLock );
    NameMap::iterator itr =
        m_mapName2Iid.find( strIfName );
    if( itr == m_mapName2Iid.end() )
        return clsid( Invalid );

    return itr->second;
}

const std::string& CInterfIdDict::GetName(
    EnumClsid Iid )
{
    static std::string strNull( "" );
    CStdMutex oLock( m_oLock );
    IidMap::iterator itr =
        m_mapIid2Name.find( Iid );
    if( itr == m_mapIid2Name.end() )
        return strNull;

    return itr->second;
}

gint32 CInterfIdDict::AddIid(
    const std::string& strIfName,
    EnumClsid Iid )
{
    if( strIfName.empty() ||
        Iid == clsid( Invalid ) )
        return -EINVAL;

    CStdMutex oLock( m_oLock );
    m_mapIid2Name[ Iid ] = strIfName;
    m_mapName2Iid[ strIfName ] = Iid;
    return 0;
}

