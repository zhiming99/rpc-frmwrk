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
 * =====================================================================================
 */

#include "registry.h"
#include "configdb.h"

using namespace std;

CDirEntry::CDirEntry()
{
    m_pParent = NULL;
}
CDirEntry::~CDirEntry()
{
    RemoveAllChildren();
    RemoveAllProps();
    m_pParent = nullptr;
    m_strName = "";
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

gint32 CDirEntry::SetProperty( 
        gint32 iProp,
        const CBuffer& oBuf )
{
    gint32 ret = 0;
    do{
        BufPtr oBufPtr( true );
        if( ERROR( ret ) )
            break;

        // make a full copy
        *oBufPtr = oBuf;

        // this is just a reference
        // increment
        ret = m_mapProps->SetProperty(
            iProp, *oBufPtr );

    }while( 0 );

    return ret;
}

gint32 CDirEntry::GetProperty(
    gint32 iProp, CBuffer& oBuf )
{
    gint32 ret = 0;
    if( !m_mapProps->exist( iProp ) )
    {
        ret = -ENOENT;
    }
    else
    {
        ret = m_mapProps->GetProperty(
            iProp, oBuf ) ;
    }

    return ret;
}

gint32 CDirEntry::RemoveProp(
    gint32 iProp )
{

    return m_mapProps->RemoveProperty( iProp );
}

void CDirEntry::RemoveAllProps()
{
    m_mapProps->RemoveAll();
}

CDirEntry* CDirEntry::GetChild(
    const std::string& strName ) const
{
    CDirEntry* pRet = NULL;

    if( strName.size() > REG_MAX_NAME )
        return nullptr;

    map< string, CDirEntry* >::const_iterator itr =
                        m_mapChilds.find( strName );

    if( itr != m_mapChilds.end() )
    {
        pRet = itr->second;
    }
    return pRet;
}

gint32 CDirEntry::RemoveAllChildren()
{
    
    gint32 ret = 0;
    map< string, CDirEntry* >::iterator itr =
                        m_mapChilds.begin();

    while( itr != m_mapChilds.end() )
    {
        delete itr->second;
        itr->second = NULL;
        m_mapChilds.erase( itr );
        itr = m_mapChilds.begin();
    }

    return ret;
}

gint32 CDirEntry::AddChild(
    const std::string& strName )
{
    gint32 ret = -ENOENT;

    if( strName.size() > REG_MAX_NAME )
        return -EINVAL;

    map< string, CDirEntry* >::iterator itr =
                        m_mapChilds.find( strName );

    if( itr == m_mapChilds.end() )
    {
        CDirEntry* pEnt = new CDirEntry;
        pEnt->SetName( strName );
        pEnt->SetParent( this );
        m_mapChilds[ strName ] = pEnt;
        ret = 0;
    }
    return ret;
}

gint32 CDirEntry::RemoveChild(
    const std::string& strName )
{
    gint32 ret = -ENOENT;
    if( strName.size() > REG_MAX_NAME )
        return -EINVAL;

    map< string, CDirEntry* >::iterator itr =
                        m_mapChilds.find( strName );

    if( itr != m_mapChilds.end() )
    {
        delete itr->second;
        m_mapChilds.erase( itr );
        ret = 0;
    }
    return ret;
}

CRegistry::CRegistry()
{
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
            // repeated '/', remove it
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
        strTemp.erase( pos );
    }

    if( ret == 0 && vecComponents.size() > 1 )
    {
        reverse( vecComponents.begin(), vecComponents.end() -  1 );
    }
    return ret;
}

gint32 CRegistry::ListDir(
    vector<string>& vecContents ) const
{
    gint32 ret = 0;
    if( m_pCurDir == NULL )
    {
        ret = -EFAULT;
    }
    else
    {
        vecContents.clear();
        CStdRMutex a( const_cast< stdrmutex&> (m_oLock ) );
        map<std::string, CDirEntry*>::const_iterator itr =
                        m_pCurDir->m_mapChilds.begin();

        while( itr != m_pCurDir->m_mapChilds.end() )
        {
            vecContents.push_back( itr->second->m_strName );
        }
    }
    return ret;
}

gint32 CRegistry::ChangeDir(
    const std::string& strDir )
{
    gint32 ret = 0;
    vector<string> vecComp;
    CDirEntry* pCurDir = m_pCurDir;

    if( strDir.size() > REG_MAX_PATH )
    {
        return -EINVAL;
    }

    ret = Namei( strDir, vecComp );
    if( ret == 0 )
    {
        vector<string>::iterator itr = vecComp.begin();
        CStdRMutex a( m_oLock );
        while( itr != vecComp.end() )
        {
            if( *itr == "." )
            {
                ++itr;
                continue;
            }
            else if( *itr == ".." )
            {
                if( pCurDir == &m_oRootDir )
                {
                    ret = -ENOENT;
                    break;
                }
                pCurDir = pCurDir->GetParent();
                ++itr;
                continue;
            }
            else
            {
                pCurDir = pCurDir->GetChild( *itr );
                if( pCurDir == NULL )
                {
                    ret = -ENOENT;
                    break;
                }
                ++itr;
            }
        }
        if( ret == 0 )
        {
            m_pCurDir = pCurDir;
        }
    }
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
            pCurDir = &GetRootDir();
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
            pCurDir = &GetRootDir();
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

                ret = pCurDir->AddChild( *itr );
                if( ret < 0 )
                    break;

                pCurDir = pCurDir->GetChild( *itr );
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
    guint32 iProp )
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

gint32 CRegistry::SetProperty(
    gint32 iProp, const CBuffer& oVal )
{
    gint32 ret = 0;

    if( m_pCurDir == nullptr )
        return -EFAULT;

    try{
        CStdRMutex a( m_oLock );
        ret = m_pCurDir->SetProperty( iProp, oVal );
    }
    catch( std::bad_alloc& e )
    {
        ret = -ENOMEM;
    }
    return ret;
}

gint32 CRegistry::GetProperty(
    gint32 iProp, CBuffer& oVal ) const
{

    gint32 ret = 0;

    if( m_pCurDir == nullptr )
        return -EFAULT;

    try{

        CStdRMutex a( const_cast< stdrmutex&> (m_oLock ) );
        ret = m_pCurDir->GetProperty( iProp, oVal );
    }
    catch( std::out_of_range& e )
    {
        ret = -ENOENT;
    }
    return ret;
}

gint32 CRegistry::RemoveDir(
    const std::string& strPath )
{
    gint32 ret;
    try{
        CStdRMutex a( m_oLock );
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
        string strName = m_pCurDir->GetName();
        ChangeDir( ".." );
        ret = m_pCurDir->RemoveChild( strName );
        if( ret == -ENOENT )
        {
            // weird, fine
            ret = 0;
        }
    }
    catch( std::out_of_range& e )
    {
        ret = -ENOENT;
    }
    return ret;
}
