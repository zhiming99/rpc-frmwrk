/*
 * =====================================================================================
 *
 *       Filename:  registry.cpp
 *
 *    Description:  implementation of the registry class
 *
 *        Version:  1.0
 *        Created:  03/11/2016 11:19:53 PM
 *       Revision:  none
 *       Compiler:  g++
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

#include "registry.h"
#include "configdb.h"

using namespace std;

namespace rpcf
{

CDirEntry::CDirEntry()
{
    m_pParent = NULL;
    m_mapProps.NewObj();
    SetClassId( clsid( CDirEntry ) );
}
CDirEntry::~CDirEntry()
{
    RemoveAllChildren();
    RemoveAllProps();
    m_pParent = nullptr;
    m_strName = "";
    m_mapProps.Clear();
}

void CDirEntry::SetName(
    const std::string& strName )
{
    m_strName = strName;
}

const string& CDirEntry::GetName() const 
{
    return m_strName;
}

CDirEntry* CDirEntry::GetParent() const
{
    return m_pParent;
}

void CDirEntry::SetParent(
    CDirEntry* pParent )
{
    m_pParent = pParent;
}

bool CDirEntry::IsRoot() const
{
    return ( m_pParent == NULL );
}

const CDirEntry& CDirEntry::operator=(
    const CDirEntry& rhs )
{
    m_strName = rhs.m_strName;
    m_mapProps = rhs.m_mapProps;
    return *this;
}

gint32 CDirEntry::GetProperty(
    gint32 iProp,
    Variant& oVar ) const
{
    gint32 ret = 0;
    CCfgOpener oCfg( ( IConfigDb* )m_mapProps );
    const Variant* p =
        oCfg.GetPropPtr( iProp );
    if( p == nullptr )
        return -ENOENT;
    oVar = *p;
    return STATUS_SUCCESS;
}

gint32 CDirEntry::SetProperty(
    gint32 iProp,
    const Variant& oVar )
{
    CCfgOpener  oCfg(
        ( IConfigDb* ) m_mapProps );
    Variant& o = oCfg[ iProp ];
    o = oVar;
    return STATUS_SUCCESS;
}

gint32 CDirEntry::RemoveProp(
    gint32 iProp )
{

    return m_mapProps->RemoveProperty( iProp );
}

void CDirEntry::RemoveAllProps()
{
    if( !m_mapProps.IsEmpty() )
        m_mapProps->RemoveAll();
}

CDirEntry* CDirEntry::GetChild(
    const std::string& strName ) const
{
    if( strName.size() > REG_MAX_NAME )
        return nullptr;

    CDirEntry* pRet = nullptr;
    auto itr = m_mapChilds.find( strName );
    if( itr != m_mapChilds.end() )
        pRet = itr->second.get();

    return pRet;
}

gint32 CDirEntry::GetChild(
    const std::string& strName,
    CHILD_TYPE& pChild ) const
{
    if( strName.size() > REG_MAX_NAME )
        return -ENAMETOOLONG;

    auto itr = m_mapChilds.find( strName );
    if( itr != m_mapChilds.end() )
    {
        pChild = itr->second;
        return 0;
    }

    return -ENOENT;
}

gint32 CDirEntry::RemoveAllChildren()
{
    
    gint32 ret = 0;
    auto itr = m_mapChilds.begin();

    while( itr != m_mapChilds.end() )
    {
        itr = m_mapChilds.erase( itr );
    }

    return ret;
}

gint32 CDirEntry::AddChild(
    const std::string& strName )
{
    if( strName.size() > REG_MAX_NAME )
        return -EINVAL;

    std::shared_ptr< CDirEntry >
        pEnt( new CDirEntry() );

    // give up ownership
    pEnt->DecRef();
    pEnt->SetName( strName );
    return AddChild( pEnt );
}

gint32 CDirEntry::AddChild(
    const CHILD_TYPE& pEnt )
{
    gint32 ret = -ENOENT;

    if( pEnt.get() == nullptr )
        return -EINVAL;

    stdstr strName = pEnt->GetName();

    auto itr = m_mapChilds.find( strName );
    if( itr != m_mapChilds.end() )
        return -EEXIST;

    pEnt->SetParent( this );
    m_mapChilds[ strName ] = pEnt;
    ret = 0;
    return ret;
}

gint32 CDirEntry::RemoveChild(
    const std::string& strName )
{
    gint32 ret = -ENOENT;
    if( strName.size() > REG_MAX_NAME )
        return -EINVAL;

    auto itr = m_mapChilds.find( strName );
    if( itr != m_mapChilds.end() )
    {
        m_mapChilds.erase( itr );
        ret = 0;
    }
    return ret;
}

gint32 CDirEntry::GetChildren(
    std::vector< CHILD_TYPE >& vecChildren ) const
{
    for( auto& elem : m_mapChilds )
        vecChildren.push_back( elem.second );
    return vecChildren.size();
}

gint32 CDirEntry::GetFullPath(
    stdstr& strPath ) const
{
    gint32 ret = 0;
    const CDirEntry* pde = this;
    while( !pde->IsRoot() )
    {
        stdstr strName = pde->GetName();
        if( strPath.size() )
            strName.push_back( '/' );
        strPath.insert( 0, strName );
        pde = pde->GetParent();
    }
    if( strPath.size() )
        strPath.insert( strPath.begin(), '/' );
    else
        strPath = "/";
    return 0;
}


CRegistry::CRegistry()
    : super()
{
    m_pRootDir = CHILD_TYPE( new CDirEntry() );
    m_pRootDir->SetName( "/" );
    m_pCurDir = m_pRootDir.get();
    SetClassId( Clsid_CRegistry );
}

CRegistry::~CRegistry()
{

}

gint32 CRegistry::Namei(
    const string& strPath,
    vector<string>& vecComponents )
{
    gint32 ret = 0;

    if( strPath.empty()
        || strPath.size() > REG_MAX_PATH )
    {
        return -EINVAL;
    }

    string strTemp = strPath;
    while( strTemp.size() )
    {
        size_t pos;
        pos = strTemp.rfind( '/' );
        if( pos == string::npos )
        {
            vecComponents.push_back( strTemp );
            break;
        }
        else if( pos < 0 )
        {
            // error occurs
            ret = -EFAULT;
            break;
        }

        if( pos > 0 && ( pos + 1 == strTemp.size() ) )
        {
            // repeated or trailing '/', remove it
            strTemp.erase( pos );
            continue;
        }
        else if( pos == 0 && ( pos + 1 == strTemp.size() ) )
        {
            // the last root '/'
            vecComponents.push_back( strTemp );
            break;
        }

        vecComponents.push_back( strTemp.substr( pos + 1 ) );

        if( pos > 0 )
            strTemp.erase( pos );
        else
        {
            // keep the root directory
            strTemp.erase( pos + 1 );
        }
    }

    if( ret == 0 && vecComponents.size() > 1 )
    {
        reverse( vecComponents.begin(), vecComponents.end() );
    }
    return ret;
}

gint32 CRegistry::ListDir(
    vector<string>& vecContents ) const
{
    gint32 ret = 0;
    CStdRMutex a( m_oLock );
    if( m_pCurDir == NULL )
    {
        ret = -EFAULT;
    }
    else
    {
        vecContents.clear();
        auto itr = m_pCurDir->m_mapChilds.begin();

        while( itr != m_pCurDir->m_mapChilds.end() )
        {
            vecContents.push_back( itr->second->m_strName );
            ++itr;
        }
    }
    return ret;
}


gint32 CRegistry::ChangeDir(
    const std::string& strDir )
{
    gint32 ret = 0;
    CStdRMutex a( m_oLock );
    CDirEntry* pDir = nullptr;
    ret = GetEntry( strDir, pDir );
    if( ERROR( ret ) )
        return ret;

    m_pCurDir = pDir;
    return ret;
}

gint32 CRegistry::GetEntry(
    const std::string& strDir,
    CDirEntry*& pDir ) const
{
    gint32 ret = 0;
    vector<string> vecComp;
    if( strDir.size() > REG_MAX_PATH )
        return -EINVAL;

    ret = Namei( strDir, vecComp );
    if( ERROR( ret ) )
        return ret;

    CStdRMutex a( m_oLock );
    auto itr = vecComp.begin();
    CDirEntry* pCurDir = m_pCurDir;
    while( itr != vecComp.end() )
    {
        if( *itr == "." )
        {
        }
        else if( *itr == ".." )
        {
            if( pCurDir == GetRootDir() )
            {
                ret = -ENOENT;
                break;
            }
            pCurDir = pCurDir->GetParent();
        }
        else if( *itr == "/" )
        {
            pCurDir = GetRootDir();
        }
        else
        {
            pCurDir = pCurDir->GetChild( *itr );
            if( pCurDir == NULL )
            {
                ret = -ENOENT;
                break;
            }
        }
        ++itr;
    }
    if( SUCCEEDED( ret ) )
        pDir = pCurDir;

    return ret;
}

gint32 CRegistry::MakeEntry(
    const std::string& strName )
{

    // make a direntry under current directory
    gint32 ret = 0;

    if( strName.size() > REG_MAX_NAME )
    {
        return -EINVAL;
    }

    vector<string> vecComp;
    do{
        if( strName.empty() )
            ret = -EINVAL;

        ret = Namei( strName, vecComp );
        if( ret < 0 )
            break;

        if( vecComp.size() > 1 )
        {
            ret = -EINVAL;
            break;
        }

        if( vecComp[ 0 ] == "/" )
        {
            ret = -EINVAL;
            break;
        }

        CStdRMutex a( m_oLock );
        ret = m_pCurDir->AddChild( strName );

    }while( 0 );

    return ret;
}

gint32 CRegistry::ExistingDir(
    const std::string& strPath ) const
{

    gint32 ret = 0;
    vector<string> vecComp;
    CDirEntry* pCurDir = m_pCurDir;

    if( strPath.size() > REG_MAX_PATH )
    {
        return -EINVAL;
    }

    do{
        if( strPath.empty() )
        {
            ret = -EINVAL;
            break;
        }

        ret = Namei( strPath, vecComp );
        if( ret < 0 )
            break;

        vector< string >::iterator itr =
                    vecComp.begin();

        if( vecComp[ 0 ] == "/" )
        {
            pCurDir = GetRootDir();
            ++itr;
        }

        CStdRMutex a( const_cast< stdrmutex&>( m_oLock ) );
        while( itr != vecComp.end() )
        {
            pCurDir = pCurDir->GetChild( *itr );            
            if( pCurDir == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            ++itr;
        }

    }while( 0 );

    return ret;
}

gint32 CRegistry::MakeDir(
    const std::string& strPath )
{
    // make directory along the path

    gint32 ret = 0;
    vector<string> vecComp;
    CDirEntry* pCurDir = m_pCurDir;
    CDirEntry* pRollback = nullptr;
    do{
        if( strPath.empty() || strPath.size() > REG_MAX_PATH )
        {
            ret = -EINVAL;
            break;
        }

        ret = Namei( strPath, vecComp );
        if( ret < 0 )
            break;

        vector< string >::iterator itr =
                    vecComp.begin();

        if( vecComp[ 0 ] == "/" )
        {
            pCurDir = GetRootDir();
            ++itr;
        }

        CStdRMutex a( m_oLock );
        CDirEntry* pParent = nullptr;
        while( itr != vecComp.end() )
        {
            pParent = pCurDir;
            pCurDir = pCurDir->GetChild( *itr );            
            if( pCurDir == nullptr )
            {
                if( pRollback == nullptr )
                    pRollback = pParent;

                ret = pParent->AddChild( *itr );
                if( ret < 0 )
                    break;

                pCurDir = pParent->GetChild( *itr );
                if( pCurDir == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
            }
            ++itr;
        }

        if( ERROR( ret )  )
        {
            // let's undo what we did
            CDirEntry* pLeaf = pParent;
            while( pRollback != pLeaf )
            {
                string strName = pLeaf->GetName();
                pLeaf = pLeaf->GetParent();
                if( pLeaf )
                {
                    pLeaf->RemoveChild( strName );
                }
            }
            break;
        }
        else
        {
            // change to the new dir
            m_pCurDir = pCurDir;
        }

    }while( 0 );

    return ret;
}

gint32 CRegistry::RemoveProperty(
    gint32 iProp )
{
    gint32 ret = 0;

    CStdRMutex a( m_oLock );
    ret = m_pCurDir->RemoveProp( iProp );

    return ret;

}
gint32 CRegistry::SetObject(
    gint32 iProp, const ObjPtr& oVal )
{
    // Please make sure oval does not
    // point to a temporary object
 
    CCfgOpenerObj a( this );
    return a.SetObjPtr( iProp, oVal );
}

gint32 CRegistry::GetObject(
    gint32 iProp, ObjPtr& oVal ) const
{
    CCfgOpenerObj a( this );
    return a.GetObjPtr( iProp, oVal );
}

gint32 CRegistry::GetProperty(
    gint32 iProp,
    Variant& oVal ) const
{

    gint32 ret = 0;
    CStdRMutex a(
        const_cast< stdrmutex& > (m_oLock ) );

    if( m_pCurDir == nullptr )
        return -EFAULT;

    ret = m_pCurDir->GetProperty(
        iProp, oVal );

    return ret;
}

gint32 CRegistry::SetProperty(
    gint32 iProp,
    const Variant& oVal )
{
    gint32 ret = 0;

    CStdRMutex a( m_oLock );

    if( m_pCurDir == nullptr )
        return -EFAULT;

    ret = m_pCurDir->SetProperty(
        iProp, oVal );

    return ret;
}

gint32 CRegistry::RemoveDir(
    const std::string& strPath )
{
    gint32 ret;
    CStdRMutex a( m_oLock );
    try{
        ret = ChangeDir( strPath );
        if( ERROR( ret ) )
            return ret;

        ret = m_pCurDir->RemoveAllChildren();
        if( ERROR( ret ) )
        {
            // don't knwo what to do
            // rollback?
            return ret;
        }
        m_pCurDir->m_mapProps.Clear();
    }
    catch( std::out_of_range& e )
    {
        ret = -ENOENT;
    }

    CDirEntry* pParent = 
        m_pCurDir->GetParent();

    if( pParent != nullptr )
    {
        string strName =
            m_pCurDir->GetName();
        ChangeDir( ".." );
        pParent->RemoveChild( strName );
    }

    return ret;
}

}
