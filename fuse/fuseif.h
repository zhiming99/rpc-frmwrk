/*
 * =====================================================================================
 *
 *       Filename:  fuseif.h
 *
 *    Description:  Declaration of rpc interface for fuse integration and
 *                  related classes
 *
 *        Version:  1.0
 *        Created:  03/01/2022 05:33:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include <exception>
#include <fuse3/fuse.h>

#define CONN_PARAM_FILE "conn_params"
#define HOPDIR  "_nexthop"
#define STREAM_DIR "_streams"
#define JSON_REQ_FILE "json_request"
#define JSON_EVT_FILE "json_event"
#define JSON_STMEVT_FILE "json_stmevt"

class CSharedLock
{
    stdrmutex m_oLock;
    bool m_bWrite = false;
    gint32 m_iReadCount = 0;
    sem_t m_semReader;
    sem_t m_semWriter;
    // true : reader, false: writer
    std::deque< bool > m_queue;

    public:
    CSharedLock()
    {
        Sem_Init( &m_semReader, 0, 0 );
        Sem_Init( &m_semWriter, 0, 0 );
    }

    ~CSharedLock()
    {
        sem_destroy( &m_semReader );
        sem_destroy( &m_semWriter );
    }

    inline gint32 LockRead()
    {
        CStdRMutex oLock( m_oLock );
        if( m_bWrite )
        {
            m_queue.push_back( true );
            oLock.Unlock();
            return Sem_Wait( &m_semReader );
        }
        else
        {
            if( m_queue.empty() )
            {
                m_iReadCount++;
                oLock.Unlock();
                return STATUS_SUCCESS;
            }
            m_queue.push_back( true );
            oLock.Unlock();
            return Sem_Wait( &m_semReader );
        }
    }

    inline gint32 TryLockRead()
    {
        CStdRMutex oLock( m_oLock );
        if( !m_bWrite )
        {
            if( m_queue.empty() )
            {
                m_iReadCount++;
                oLock.Unlock();
                return STATUS_SUCCESS;
            }
        }
        return -EAGAIN;
    }

    inline gint32 LockWrite()
    {
        CStdRMutex oLock( m_oLock );
        if( m_bWrite )
        {
            m_queue.push_back( false );
            oLock.Unlock();
            return Sem_Wait( &m_semWriter );
        }
        else if( m_iReadCount > 0 )
        {
            m_queue.push_back( false );
            oLock.Unlock();
            return Sem_Wait( &m_semWriter );
        }
        else if( m_iReadCount == 0 )
        {
            m_bWrite = true;
            oLock.Unlock();
            return 0;
        }
        return -EFAULT;
    }

    inline gint32 TryLockWrite()
    {
        CStdRMutex oLock( m_oLock );
        if( m_bWrite || m_iReadCount > 0 ) 
            return -EAGAIN;

        if( m_iReadCount == 0 )
        {
            m_bWrite = true;
            oLock.Unlock();
            return 0;
        }
        return -EFAULT;
    }

    inline gint32 ReleaseRead()
    {
        CStdRMutex oLock( m_oLock );
        m_iReadCount--;
        if( m_iReadCount > 0 )
            return STATUS_SUCCESS;

        if( m_iReadCount == 0 )
        {
            if( m_queue.empty() )
                return STATUS_SUCCESS;

            if( m_queue.front() == true )
                return -EFAULT;

            m_bWrite = true;
            m_queue.pop_front();
            return Sem_Post( &m_semWriter );
        }
        return -EFAULT;
    }

    inline gint32 ReleaseWrite()
    {
        CStdRMutex oLock( m_oLock );
        if( m_queue.empty() )
        {
            m_bWrite = false;
        }
        else if( m_queue.front() == false )
        {
            m_bWrite = true;
            m_queue.pop_front();
            Sem_Post( &m_semWriter );
        }
        else while( m_queue.front() == true )
        {
            m_iReadCount++;
            Sem_Post( &m_semReader );
            m_queue.pop_front();
        }
        return STATUS_SUCCESS;
    }
};

template < bool bRead >
struct CLocalLock
{
    // only works on the same thread
    CSharedLock* m_pLock = nullptr;
    bool m_bLocked = false;
    CLocalLock( CSharedLock& oLock )
    {
        m_pLock = &oLock;
        bRead ? oLock.LockRead():
            oLock.LockWrite();
        m_bLocked = true;
    }

    ~CLocalLock()
    { 
        if( m_bLocked && m_pLock != nullptr )
        {
            m_bLocked = false;
            bRead ? m_pLock->ReleaseRead() : 
                m_pLock->ReleaseWrite();
            m_pLock = nullptr;
        }
    }

    void Unlock()
    {
        if( !m_bLocked )
            return;
        m_bLocked = false;
        bRead ? oLock.ReleaseRead() :
            oLock.ReleaseWrite();
    }

    void Lock()
    {
        if( !m_pLock )
            return;
        bRead ? m_pLock->LockRead() :
            m_pLock->LockWrite();
        m_bLocked = true;
    }
};

typedef CLocalLock< true > CReadLock;
typedef CLocalLock< false > CWriteLock;


template< class T >
class CFuseRootBase:
    public T
{
    protected:
    fuse* m_pFuse = nullptr;
    CRegistry m_oReg;
    std::vector< ObjPtr > m_vecIfs;
    CIoManager* m_pMgr = nullptr;

    public:
    typedef T super;
    CFuseRootBase( const IConfigDb* pCfg ):
        super( pCfg )
    {
        EnumClsid iClsid;
        if( IsServer() )
            iClsid = clsid( CFuseRootServer );
        else
            iClsid = clsid( CFuseRootProxy );

        SetClassId( iClsid );
        CCfgOpener oCfg( pCfg );
        gint32 ret = oCfg.GetPointer(
            propIoMgr, m_pMgr );
        if( ERROR( ret ) )
        {
            string strMsg = DebugMsg( ret,
                "cannot find iomgr in "
                "CFuseRootBase's ctor" );
            throw runtime_error( strMsg );
        }
    }

    gint32 GetFuseObj(
        const stdstr& strPath,
        CFuseObjBase*& pDir )
    {
        if( strPath.empty() ||
            strPath[ 0 ] != '/' )
            return -EINVAL;

        if( strPath == "/" )
        {
            pDir = static_cast< CFuseObjBase* >(
                &m_oReg.GetRootDir() );
            return pDir;
        }

        CDirEntry* pTemp = nullptr;
        gint32 ret = m_oReg.GetEntry(
            strPath, pTemp );
        if( ERROR( ret ) )
            return ret;

        pDir = static_cast
            < CFuseObjBase* >( pTemp );
        return STATUS_SUCCESS;
    }

    // mount the rpc file system at the mount point
    // strMntPt
    gint32 DoMount(
        const stdstr& strMntPt );

    gint32 BuildDirectories(
        const std::vector< stdstr >& strDescs );

    using super::OnPreStop;
    gint32 OnPreStop(
        IEventSink* pCallback ) override;

    gint32 init();
    gint32 destroy();
};

class CFuseObjBase : public CDirEntry
{
    CSharedLock m_oLock;
    std::atomic< gint32 > m_iOpenCnt;
    timespec m_tsUpdTime;
    timespec m_tsModtime;
    guint32 m_dwStat;
    guint32 m_dwMode = S_IRWXU;

    public:
    typedef CDirEntry super;
    CFuseObjBase();

    void SetMode( guint32 dwMode )
    { m_dwMode = dwMode; }

    guint32 GetMode() const
    { return m_dwMode; }

    gint32 open();
    gint32 read();
    gint32 write();
    gint32 readdir();
    gint32 opendir();
    gint32 getattr();
    gint32 setattr();
    gint32 rmdir();
    gint32 getlk();
    gint32 setlk();
};

// top-level dirs under the mount point
// proxy side only
class CFuseConnDir : public CFuseObjBase
{
    CfgPtr m_pConnParams;

    public:
    typedef CFuseObjBase super;
    CFuseConnDir( const stdstr& strNodeName )
    {
        SetName( strNodeName );
        // add an RO _nexthop directory this dir is
        // for docking nodes of sub router-path
        stdstr strName = HOP_DIR;
        auto pDir = std::shared_ptr<CDirEntry>
            ( new CFuseDirectory( strName ) ); 
        CFuseObjBase* pObj = static_cast
            < CFuseObjBase >( pDir.get() );
        pObj->SetMode( S_IRUSR | S_IXUSR );
        pObj->DecRef();
        AddChild( pDir );

        // add an RO file for connection parameters
        strName = CONN_PARAM_FILE;
        auto pFile = std::shared_ptr<CDirEntry>
            ( new CFuseTextFile( strName ) ); 
        pObj = static_cast
            < CFuseObjBase* >( pFile.get() );
        pObj->SetMode( S_IRUSR );
        pObj->DecRef();
        AddChild( pFile );
    }

    stdstr GetRouterPath( CDirEntry* pDir ) const
    {
        if( pDir->IsRoot() )
            return stdstr( "" );

        stdstr strPath;
        strPath = "/";
        CFuseConnDir* pConnDir = dynamic_cast
            < CFuseConnDir* >( pDir );
        if( pConnDir != nullptr )
            return strPath;

        stdstr strName = pDir->GetName();
        strPath += strName;

        CDirEntry* pParent = pDir->GetParent();
        while( pParent != nullptr &&
            pConnDir == nullptr )
        {
            strName = "/";
            strName += pParent->GetName();
            pParent = pParent->GetParent();
            pConnDir = dynamic_cast
                < CFuseConnDir* >( pParent );
            if( strName == HOP_DIR )
                continue;
            strPath.insert( 0, strName );
        }
        return strPath;
    }

    gint32 AddSvcDir(
        const std::shared_ptr< CDirEntry >& pEnt )
    {
        const CFuseSvcDir* pDir = pEnt.get();
        gint32 ret = 0;
        do{
            if( pDir == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ObjPtr pIf = pDir->GetIf();
            if( pIf.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )pIf ); 

            IConfigDb* pConnParams = nullptr;
            ret = oIfCfg.GetPointer(
                propConnParams, pConnParams );
            if( ERROR( ret ) )
                break;

            if( super::GetCount() <= 2 )
            {
                SetConnParams( pConnParams );
            }
            else if( !IsEqualConn( pConnParams ) )
            {
                ret = -EACCES;
                break;
            }

            CConnParamsProxy oConn( pConnParams );
            stdstr strPath = oConn.GetRouterPath();
            stdstr strPath2 = "/";
            if( strPath == strPath2 )
            {
                ret = AddChild( pEnt );
                break;
            }

            ret =IsMidwayPath( strPath2, strPath );
            if( ERROR( ret ) )
            {
                ret = -EINVAL;
                break;
            }

            stdstr strRest =
                strPath.substr( strPath2.size() );

            std::vector< stdstr > vecComps;
            ret = CRegistry::Namei(
                strRest, vecComps );
            if( ERROR( ret ) )
                break;

            if( vecComps[ 0 ] == "/" )
                vecComps.pop_front();

            CDirEntry* pDir = nullptr;
            CDirEntry* pCur = this;
            CDirEntry* pHop = nullptr;
            for( auto& elem : vecComps )
            {
                pDir = pCur->GetChild( HOP_DIR );
                if( pDir == nullptr )
                {
                    CFuseDirectory* pNewDir =
                    new CFuseDirectory( HOP_DIR );
                    pNewDir->DecRef();

                    std::shared_ptr< CDirEntry >
                        pEnt1( pNewDir );
                    pCur->AddChild( pEnt1 );
                    pDir = pNewDir;
                    pNewDir->SetMode( S_IRUSR | S_IXUSR );
                    pHop = nullptr;
                }
                else
                {
                    pHop = pDir->GetChild( elem );
                }

                if( pHop == nullptr );
                {
                    CFuseDirectory* pNewDir =
                        new CFuseDirectory( elem );
                    pNewDir->DecRef();

                    std::shared_ptr< CDirEntry >
                        pEnt1( pNewDir );
                    pDir->AddChild( pEnt1 );
                    pNewDir->SetMode( S_IRUSR | S_IXUSR );
                    pHop = pEnt1.get();
                }
                pCur = pHop;
            }
            pCur->AddChild( pEnt );

        }while( 0 );

        return ret;
    };

    void SetConnParams( const CfgPtr& pCfg )
    {
        m_pConnParams = pCfg;
        stdstr strDump = DumpConnParams();
        CDirEntry* pEnt =
            GetChild( CONN_PARAM_FILE );
        CFuseTextFile* pText = static_cast
            < CFuseTextFile* >( pEnt );
        pText->SetContent( strDump );
    }

    bool IsEqualConn( const IConfigDb* pConn ) const
    {
        CConnParamsProxy oConn(
            ( IConfigDb* )m_pConnParams );
        CConnParamsProxy oConn1( pConn );
        return ( oConn == oConn1 );
    }

    stdstr DumpConnParams()
    {
        gint32 ret = 0;
        if( m_pConnParams.empty() )
            return "";

        stdstr strRet;
        do{
            CConnParamsProxy oConn(
                ( IConfigDb* )m_pConnParams );
            
            bool bVal = oConn.IsServer();
            strRet += "IsServer="; +
            strRet += ( bVal ? "true\n" : "false\n" );

            strstr strVal = oConn.GetSrcIpAddr();
            if( strVal.size() )
            {
                strRet += "SrcIpAddr=";
                strRet += strVal + "\n";
            }

            guint32 dwVal = oConn.GetSrcPortNum();
            if( dwVal != 0 )
            {
                strRet += "SrcPortNum=";
                strRet += std::to_string( dwVal );
                strRet += "\n";
            }

            strVal = oConn.GetDestIpAddr();
            if( strVal.size() )
            {
                strRet += "DestIpAddr";
                strRet += strVal + "\n";
            }

            dwVal = oConn.GetDestPortNum();

            if( dwVal != 0 )
            {
                strRet += "DestPortNum=";
                strRet += std::to_string( dwVal );
                strRet += "\n";
            }

            bVal = oConn.IsWebSocket();
            strRet += "WebSocket=";
            strRet += ( bVal ?\
                "enabled\n" : "disabled\n" );

            if( bVal )
            {
                strRet += "URL=";
                strRet += oConn.GetUrl() + "\n";
            }

            bVal = oConn.IsSSL();
            strRet += "SSL=";
            strRet += ( bVal ?
                "enabled\n" : "disabled\n" );

            bVal = oConn.HasAuth(); 
            strRet += "Authentication=";
            strRet += ( bVal ?
                "enabled\n" : "disabled\n" );

            if( bVal )
            {
                CCfgOpener oCfg(
                    ( IConfigDb*)m_pConnParams );
                IConfigDb* pAuthInfo;
                ret = oCfg.GetPointer(
                    propAuthInfo, pAuthInfo );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oAuth( pAuthInfo );
                ret = oAuth.GetStrProp(
                    propAuthMech, strVal );
                if( ERROR( ret ) )
                    break;

                strRet += "mechanism=";
                strRet += strVal + "\n";

                if( strMech != "krb5" )
                    break;

                ret = oAuth.GetStrProp(
                    propUserName, strVal );
                if( SUCCEEDED( ret ) )
                {
                    strRet += "\tuser=\"";
                    strRet += strVal + "\"\n";
                }

                ret = oAuth.GetStrProp(
                    propRealm, strVal );
                if( SUCCEEDED( ret ) ) 
                {
                    strRet += "\trealm=\"";
                    strRet += strVal + "\"\n";
                }

                ret = oAuth.GetBoolProp(
                    propSignMsg, bVal );
                if( SUCCEEDED( ret ) )
                {
                    strRet += "\tsignmsg=";
                    strRet += ( bVal ?
                        "true\n" : "false\n" );
                }
            }

        }while( 0 );

        return strRet;
    }
};

class CFuseDirectory : public CFuseObjBase
{
    typedef CFuseObjBase super;
    CFuseDirectory( const stdstr& strName )
        : super()
    { SetName( strSvcName ); }
};

class CFuseSvcDir : public CFuseDirectory
{
    public:
    CRpcServices* m_pIf;
    typedef CFuseDirectory super;
    CFuseSvcDir(
        const stdstr& strSvcName,
        CRpcServices* pIf ) :
        super( strSvcName ),
        m_pIf( pIf )
    {}

    inline CRpcServices* GetIf() const
    { return m_pIf; }

    inline void SetIf( CRpcServices* pIf )
    { m_pIf = pIf; }

    inline CIoManager* GetIoMgr()
    {
        if( m_pIf == nullptr )
            return nullptr;
        return m_pIf->GetIoMgr();
    }
};


class CFuseFileEntry : public CFuseObjBase
{
    protected:
    std::deque< BufPtr > m_queIncoming;
    fuse_pollhandle* m_pPollHandle;

    public:
    typedef CFuseObjBase super;
    CFuseFileEntry( const stdstr& strName )
        : super()
    { SetName( strName ); }

    gint32 open();
    gint32 close();

};

class CFuseTextFile : public CFuseObjBase
{
    stdstr m_strContent;
    typedef CFuseObjBase super;
    CFuseInfoFile( const stdstr& strName )
        : super()
    { SetName( strName ); }

    void SetContent( const stdstr& strConent )
    { m_strContent = strConent; }

    const stdstr& GetContent() const
    { return m_strContent; }
}

class CFuseEvtFile : CFuseFileEntry
{
    public:
    gint32 read();
    gint32 write();
    typedef CFuseFileEntry super;
    CFuseEvtFile( const stdstr& strName )
        : super( strName )
    {}
};

class CFuseReqFile : CFuseEvtFile
{
    public:
    typedef CFuseEvtFile super;
    CFuseReqFile( const stdstr& strName )
        : super( strName )
    {}
    gint32 write();
};

class CFuseStmDir : CFuseDirectory
{
    public:
    typedef CFuseDirectory super;
    CFuseStmDir() : super( STREAM_DIR )
    {}

};

class CFuseStmFile : CFuseFileEntry
{
    HANDLE m_hStream;
    CRpcServices* m_pIf;

    public:
    typedef CFuseFileEntry super;
    CFuseStmFile( const stdstr& strName,
        HANDLE hStm, CRpcServices* pIf ) :
        super( strName ),
        m_hStream( hStm ),
        m_pIf( pIf )
    {}

    HANDLE GetStream()
    { return hStream; }
};

class CFuseStmEvtFile : CFuseEvtFile
{
    public:
    typedef CFuseEvtFile super;
    CFuseStmEvtFile()
        : super( "json_stmevt" )
    {}
};

#define GET_STMCTX_LOCKED( _hStream, _pCtx ) \
    if( IsServer() ) \
    {\
        CStreamServerSync *pStm =\
            ObjPtr( this );\
        ret = pStm->GetContext(\
            _hStream, _pCtx );\
        if( ERROR( ret ) )\
            break;\
    }\
    else\
    {\
        CStreamServerSync *pStm = \
            ObjPtr( this ); \
        ret = pStm->GetContext( \
            _hStream, _pCtx ); \
        if( ERROR( ret ) ) \
            break; \
    } \
    CCfgOpener oCfg( \
        ( IConfigDb* )_pCtx ); \

#define IFBASE( _bProxy ) typename std::conditional< _bProxy, \
    CAggInterfaceProxy, CAggInterfaceServer>::type

template< bool bProxy >
class CFuseServicePoint :
    public virtual IFBASE( bProxy ) 
{
    std::shared_ptr< CDirEntry > m_pSvcDir;
    stdstr m_strSvcPath;

    public:
    typedef IFBASE( bProxy ) _MyVirtBase;
    typedef IFBASE( bProxy ) super;
    CFuseServicePoint( const IConfigDb* pCfg ) :
        super( pCfg )
    {}

    using super::GetIid;
    const EnumClsid GetIid() const override
    { return iid( CFuseServicePoint ); }

    using super::OnPreStop;
    gint32 OnPreStop(
        IEventSink* pCallback ) override;

    using super::OnPostStart;
    gint32 OnPostStart(
        IEventSink* pCallback ) override
    { return BuildDirectories(); }

    // build the directory hierarchy for this service
    gint32 BuildDirectories()
    {
        gint32 ret = 0;
        do{
            stdstr strObjPath;
            CCfgOpenerObj oIfCfg( this );

            ret = oIfCfg.GetStrProp(
                propObjPath, strObjPath );
            if( ERROR( ret ) )
                break;
            size_t pos = strObjPath.rfind( '/' );
            if( pos + 1 == strObjPath.size() )
            {
                ret = -EINVAL;
                break;
            }

            // add an RO directory for this svc
            stdstr strName =
                strObjPath.substr( pos + 1 );
            m_pSvcDir = std::shared_ptr<CDirEntry>
                ( new CFuseSvcDir( strName, this ) ); 
            m_pSvcDir->DecRef();

            // add an RW request file
            strName = JSON_REQ_FILE;
            auto pFile = std::shared_ptr<CDirEntry>
                ( new CFuseReqFile( strName ) ); 
            m_pSvcDir->AddChild( pFile );
            CFuseObjBase* pObj = static_cast
                < CFuseObjBase* >( pFile.get() );
            pObj->SetMode( S_IRUSR | S_IWUSR );
            pObj->DecRef();

            // add an RO/WO event file 
            strName = JSON_EVT_FILE;
            pFile = std::shared_ptr<CDirEntry>
                ( new CFuseEvtFile( strName ) ); 
            m_pSvcDir->AddChild( pFile );
            pObj = static_cast
                < CFuseObjBase* >( pFile.get() );
            pObj->SetMode( S_IRUSR );
            pObj->DecRef();

            // add an RW directory for streams
            auto pDir = std::shared_ptr<CDirEntry>
                ( new CFuseStmDir() ); 
            m_pSvcDir->AddChild( pDir );
            pObj = static_cast
                < CFuseObjBase* >( pDir.get() );
            pObj->SetMode( S_IRWXU );
            pObj->DecRef();

            // add an RO stmevt file for stream events
            strName = JSON_STMEVT_FILE;
            pFile = std::shared_ptr<CDirEntry>
                ( new CFuseStmEvtFile( strName ) ); 
            pDir->AddChild( pFile );
            pObj = static_cast
                < CFuseObjBase* >( pFile.get() );
            pObj->SetMode( S_IRUSR );
            pObj->DecRef();

        }while( 0 );
        return ret;
    }

    gint32 GetSvcPath( stdstr& strPath )
    {
        if( m_strSvcPath.empty() )
        {
            CDirEntry* pDir = m_pSvcDir.get();
            if( unlikely( m_pSvcDir == nullptr ) )
                return -EFAULT;
            else while( pDir != nullptr )
            {
                stdstr strName = "/";
                strName.append( pDir->GetName() );
                strPath.insert( strName );
                pDir = pDir->GetParent();
            }
            m_strSvcPath = strPath;
        }
        else
        {
            strPath = m_strSvcPath;
        }
        return STATUS_SUCCESS;
    }

    gint32 StreamToName(
        HANDLE hStream,
        stdstr& strName ) const
    {
        gint32 ret = 0;
        if( hStream == INVALID_HANDLE )
            return -EINVAL;
        do{
            CfgPtr pCtx;
            CStdRMutex oLock( GetLock() );
            GET_STMCTX_LOCKED( hStream, pCtx );
            ret = oCfg.GetStrProp(
                propPath1, strName );
            
        }while( 0 );

        return ret;
    }

    gint32 NameToStream(
        const stdstr& strName,
        HANDLE& hStream ) const
    {
        gint32 ret = 0;
        do{
            CDirEntry* pDir =
                GetChild( STREAM_DIR );
            if( pDir == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            CDirEntry* pStmDir =
                pDir->GetChild( strName );
            if( pStmDir == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            CFuseStmFile* pStmFile =
                static_cast< CFuseStmFile >(
                    pStmDir );
            hStream = pStmFile->GetStream();

        }while( 0 );

        return ret;
    }

    gint32 AddStreamName(
        HANDLE hStream,
        stdstr& strName )
    {
        gint32 ret = 0;
        do{
            CfgPtr pCtx;
            CStdRMutex oLock( GetLock() );
            GET_STMCTX_LOCKED( hStream, pCtx );
            ret = oCfg.SetStrProp(
                propPath1, strName );
        }while( 0 );

        return ret;
    }

    gint32 CreateStmFile(
        const stdstr& strName,
        HANDLE hStream );

    gint32 SendStmEvent(
        const stdstr& strName,
        const stdstr& strJsonMsg );

    gint32 RemoveStmFile(
        const stdstr& strName );

    gint32 OnWriteResumed(
        HANDLE hChannel )
    {
        gint32 ret = StreamToName(
            hStream, strName );
        if( ERROR( ret ) )
            return ret;
    }

    gint32 OnReadStreamComplete(
        HANDLE hChannel,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx );

    gint32 OnWriteStreamComplete(
        HANDLE hChannel,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx );

    inline gint32 OnStreamReady(
        HANDLE hStream )
    { return CreateStmFile(); }

    inline gint32 OnStmClosing(
        HANDLE hStream )
    {
        gint32 ret = StreamToName(
            hStream, strName );
        if( ERROR( ret ) )
            return ret;
        RemoveStmFile( strName );
    }

    inline gint32 AcceptNewStream(
        IConfigDb* pDataDesc )
    { return 0; }
};

class CFuseSvcProxy :
    public CFuseServicePoint< CAggInterfaceProxy >
{
    public:
    typedef CFuseServicePoint< CAggInterfaceProxy > super;
    CFuseSvcProxy( const IConfigDb* pCfg ):
    _MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 DispatchReq(
        IConfigDb* pContext,
        const stdstr& strReq,
        stdstr& strResp ) = 0;
};

class CFuseSvcServer :
    public CFuseServicePoint< CAggInterfaceServer >
{
    public:
    typedef CFuseServicePoint< CAggInterfaceServer > super;
    CFuseSvcServer( const IConfigDb* pCfg ):
    _MyVirtBase( pCfg ), super( pCfg )
    {}

    //Response&Event Dispatcher
    gint32 DispatchMsg(
        const std::string& strMsg ) = 0;
};
