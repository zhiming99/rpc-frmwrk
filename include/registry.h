/*
 * =====================================================================================
 *
 *       Filename:  regstry.h
 *
 *    Description:  a tree-like registry implementation
 *
 *        Version:  1.0
 *        Created:  03/10/2016 07:35:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
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

#include "defines.h"
#include "autoptr.h"
#include "configdb.h"

namespace rpcf
{

class CDirEntry : public CObjBase
{
    friend class CRegistry;
    protected:
    std::map<std::string, std::shared_ptr< CDirEntry>> m_mapChilds;
    CfgPtr      m_mapProps;
    CDirEntry* m_pParent;
    std::string m_strName;

    public:

    CDirEntry( const std::string& strName, CDirEntry* pParent );
    CDirEntry();
    ~CDirEntry();

    CDirEntry* GetParent() const;

    void SetParent( CDirEntry* );

    void SetName( const std::string& strName );
    const std::string& GetName() const;

    bool IsRoot() const;

    CDirEntry* GetChild( const std::string& strName ) const;
    gint32 AddChild( const std::string& strName );
    gint32 AddChild( std::shared_ptr< CDirEntry >& pEnt );

    gint32 GetProperty(
        gint32 iProp,
        Variant& oVar ) const override;

    gint32 SetProperty(
        gint32 iProp,
        const Variant& oVar ) override;

    gint32 RemoveChild( const std::string& strName );

    gint32 RemoveProp( gint32 iProp );
    const CDirEntry& operator=( const CDirEntry& rhs );

    inline gint32 GetCount() const
    { return m_mapChilds.size(); }

    gint32 RemoveAllChildren();
    void RemoveAllProps();

};

class CRegistry : public CObjBase
{
    protected:
    CDirEntry m_oRootDir;
    CDirEntry *m_pCurDir;
    mutable std::recursive_mutex m_oLock;

    public:

    typedef CObjBase super;

    CRegistry();
    ~CRegistry();

    std::recursive_mutex& GetLock(){ return m_oLock; };

    gint32 MakeEntry( const std::string& );
    gint32 MakeDir( const std::string& );
    gint32 ChangeDir( const std::string& );

    gint32 GetEntry(
        const std::string& strPath,
        CDirEntry*& pDir ) const;

    gint32 RemoveProperty( guint32 iProp );

    gint32 GetProperty(
        gint32 iProp,
        Variant& oVar ) const override;

    gint32 SetProperty(
        gint32 iProp,
        const Variant& oVar ) override;

    gint32 SetObject( gint32 iProp, const ObjPtr& oVal );
    gint32 GetObject( gint32 iProp, ObjPtr& oVal ) const;

    gint32 RemoveDir( const std::string& );
    gint32 ExistingDir( const std::string& ) const;

    static gint32 Namei(
        const std::string& strPath,
        std::vector<std::string>& vecComponents );

    gint32 ListDir(
        std::vector<std::string>& vecContents ) const;

    CDirEntry& GetRootDir() const
    { return ( CDirEntry& )m_oRootDir; }
};

typedef CAutoPtr< clsid( CRegistry ), CRegistry > RegPtr;

}
