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
 * =====================================================================================
 */

#pragma once

#include "defines.h"
#include "autoptr.h"

class CDirEntry : public CObjBase
{
    friend class CRegistry;
    protected:
    std::map<std::string, CDirEntry*> m_mapChilds;
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

    gint32 SetProperty( 
        gint32 iProp,
        const CBuffer& oBuf );

    gint32 GetProperty( 
        gint32 iProp,
        CBuffer& oBuf );

    gint32 RemoveChild( const std::string& strName );

    gint32 RemoveProp( gint32 iProp );
    const CDirEntry& operator=( const CDirEntry& rhs );

    gint32 RemoveAllChildren();
    void RemoveAllProps();

};

class CRegistry : public CObjBase
{
    protected:
    CDirEntry m_oRootDir;
    CDirEntry *m_pCurDir;
    std::recursive_mutex m_oLock;

    public:
    CRegistry();
    ~CRegistry();

    std::recursive_mutex& GetLock(){ return m_oLock; };

    gint32 MakeEntry( const std::string& );
    gint32 MakeDir( const std::string& );
    gint32 ChangeDir( const std::string& );
    gint32 SetProperty( gint32 iProp, const CBuffer& oVal );
    gint32 GetProperty( gint32 iProp, CBuffer& oVal ) const;
    gint32 RemoveProperty( guint32 iProp );

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
