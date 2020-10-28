/*
 * =====================================================================================
 *
 *       Filename:  iiddict.h
 *
 *    Description:  declaration of an Iid and Interface name dictionary
 *
 *        Version:  1.0
 *        Created:  08/17/2018 08:48:16 AM
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
#pragma once
#include <unordered_map>

namespace rpcfrmwrk
{

class CInterfIdDict
{
    typedef std::unordered_map
        < guint64, std::string > IidMap;

    typedef std::unordered_map
        < std::string, guint64 > NameMap;

    IidMap m_mapIid2Name;
    NameMap m_mapName2Iid;

    stdmutex m_oLock;

    public:

    guint64 GetIid(
        const std::string& strIfName );

    const std::string& GetName(
        guint64 Iid );

    gint32 AddIid(
        const std::string& strIfName,
        guint64 Iid );
};

}
