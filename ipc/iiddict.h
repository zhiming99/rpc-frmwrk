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
 * =====================================================================================
 */
#pragma once
#include <unordered_map>

class CInterfIdDict
{
    typedef std::unordered_map
        < EnumClsid, std::string > IidMap;

    typedef std::unordered_map
        < std::string, EnumClsid > NameMap;

    IidMap m_mapIid2Name;
    NameMap m_mapName2Iid;

    stdmutex m_oLock;

    public:

    EnumClsid GetIid(
        const std::string& strIfName );

    const std::string& GetName(
        EnumClsid Iid );

    gint32 AddIid(
        const std::string& strIfName,
        EnumClsid Iid );
};


