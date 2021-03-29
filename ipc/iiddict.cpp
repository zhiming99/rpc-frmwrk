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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "rpc.h"
#include "iiddict.h"

namespace rpcf
{

guint64 CInterfIdDict::GetIid(
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
    guint64 Iid )
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
    guint64 Iid )
{
    if( strIfName.empty() ||
        Iid == clsid( Invalid ) )
        return -EINVAL;

    CStdMutex oLock( m_oLock );
    m_mapIid2Name[ Iid ] = strIfName;
    m_mapName2Iid[ strIfName ] = Iid;
    return 0;
}

}
