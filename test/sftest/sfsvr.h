/*
 * =====================================================================================
 *
 *       Filename:  sfsvr.h
 *
 *    Description:  Declaration of CSimpFileServer/Proxy and related unit
 *                  tests. The classes implemente the basic file transfer with
 *                  the streaming support from CStreamServerSync and
 *                  CStreamProxySync. 
 *
 *        Version:  1.0
 *        Created:  08/11/2018 14:32:00 AM Beijing
 *       modified:  09/21/2019 07:41:47 AM Beijing
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *   Organization:  
 *
 * =====================================================================================
 */
#pragma once

#include <rpc.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <ifhelper.h>
#include <filetran.h>
#include <counters.h>
#include <streamex.h>
#include <pwd.h>

#define METHOD_Echo         "Echo"

#define MOD_SERVER_NAME "SimpFileServer"
#define OBJNAME_ECHOSVR "CSimpFileServer"               

#define MOD_CLIENT_NAME "SimpFileClient"
#define OBJNAME_ECHOCLIENT "CSimpFileClient"               

#define SF_PARAM_BASE           ( propReservedEnd + 1 )
#define SF_START_OFFSET_IDX     ( SF_PARAM_BASE + 0 )
#define SF_REQSIZE_IDX          ( SF_PARAM_BASE + 1 )
#define SF_HASH_IDX             ( SF_PARAM_BASE + 2 )
#define SF_IODIR_IDX            ( SF_PARAM_BASE + 3 )
#define SF_FILENAME_IDX         ( SF_PARAM_BASE + 4 )

// future use
#define SF_USERNAME_IDX         ( SF_PARAM_BASE + 5 )
#define SF_SESSION_IDX          ( SF_PARAM_BASE + 6 )

// working parameter
#define SF_FD_IDX               ( SF_PARAM_BASE + 7 )
#define SF_CUR_OFFSET_IDX       ( SF_PARAM_BASE + 8 )

#define SF_FILEMODE_IDX         ( SF_PARAM_BASE + 9 )
#define SF_RESP_IDX             ( SF_PARAM_BASE + 10 )

#define SF_ROOT_DIR             "/tmp/sfsvr-root/"

// Declare the class id for this object and
// declare the iid for the interfaces we have
// implemented for this object.
//
// Note that, CFileTransferServer and
// IInterfaceServer are built-in interfaces
enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 611,
    DECL_CLSID( CSimpFileServer ),
    DECL_CLSID( CSimpFileClient ),
    DECL_CLSID( CTransferContext ),

    DECL_IID( MyStart ) = clsid( ReservedIidStart ) + 100,
    DECL_IID( CEchoServer ),
    // not used, because it is over the
    // CStreamServer interface, and we cannot get
    // this iid through the router yet. So use the
    // iid( IStream ) instead
    DECL_IID( CMyFileServer ), 
};

#define APPEND_STRING( _oBuf, _str, _dwSize ) \
do{ \
    guint32 dwVal; \
    if( _str.empty() ) \
    { \
        dwVal = 0; \
        _oBuf.Append( ( guint8* )&dwVal, \
            sizeof( dwVal ) ); \
        _dwSize += sizeof( dwVal ); \
        break; \
    } \
    dwVal = htonl( _str.size() + 1 ); \
    _oBuf.Append( ( guint8* )&dwVal, \
        sizeof( dwVal ) ); \
    _oBuf.Append( ( const guint8* )_str.c_str(), \
        _str.size() + 1 ); \
    _dwSize += sizeof( dwVal ) + \
        _str.size() + 1; \
}while( 0 )

#define RETRIEVE_STRING( _pBuf, _str, _dwOffset ) \
do{ \
    guint32 dwVal = 0; \
    memcpy( &dwVal, _pBuf + _dwOffset, \
        sizeof( dwVal ) );  \
    _dwOffset += sizeof( dwVal ); \
    dwVal = ntohl( dwVal ); \
    if( dwVal == 0 ) \
        break; \
    if( _pBuf[ _dwOffset + dwVal - 1 ] != 0 ) \
        break; \
    _str = ( char* )( _pBuf + _dwOffset ); \
    _dwOffset += dwVal; \
}while( 0 )

struct CTransferContext : public CObjBase
{
    typedef CObjBase super;
    // start offset
    guint64 m_qwStart  = 0;

    // request size
    guint32 m_dwReqSize = 0;

    // upload/download
    bool    m_bUpload = false;

    // file path
    std::string m_strFileName;

    // user name and session id for access control
    std::string m_strUserName;
    std::string m_strSessionId;

    // file hash, valid only full file is
    // requested
    std::string m_strHash;

    guint64 m_qwCurOffset = 0;
    gint32  m_iFd = -1;
    std::string m_strPathName;

    BufPtr m_pBufBlocked;
    gint32 m_iRet = STATUS_PENDING;

    CTransferContext() : super()
    { SetClassId( clsid( CTransferContext ) ); }

    ~CTransferContext()
    {
        m_pBufBlocked.Clear();
        if( m_iFd != -1 )
        {
            close( m_iFd );
            m_iFd = -1;
        }
    }

    gint32 SetError( gint32 iError )
    {
        return ( m_iRet = iError );
    }

    gint32 Serialize(
        CBuffer& oBuf ) const
    {
        // serialize to a byte stream because the
        // router can not recognize this object.
        guint32 dwSize = 0;
        guint8 arrBytes[ 8 ] = {0};
        guint64* qwVal = ( guint64* )arrBytes;
        gint32 ret = 0;
        do{
            oBuf.Append( ( guint8* )&dwSize,
                sizeof( dwSize ) );

            *qwVal = htonll( m_qwStart );
            oBuf.Append( ( guint8* )qwVal,
                sizeof( m_qwStart ) );

            guint32* dwVal = ( guint32* )arrBytes;
            *dwVal = htonl( m_dwReqSize );
            oBuf.Append( ( guint8*)dwVal,
                sizeof( m_dwReqSize ) );

            oBuf.Append( ( guint8*)&m_bUpload,
                sizeof( guint8 ) );

            oBuf.Append( arrBytes,
                3 * sizeof( guint8 ) );

            dwSize += sizeof( m_qwStart ) +
                sizeof( m_dwReqSize ) +
                sizeof( guint32 );
            
            APPEND_STRING( oBuf,
                m_strFileName, dwSize );
            if( m_strFileName.empty() )
            {
                ret = -EINVAL;
                break;
            }

            APPEND_STRING( oBuf,
                m_strUserName, dwSize );
            if( m_strUserName.empty() )
                break;

            APPEND_STRING( oBuf,
                m_strSessionId, dwSize );
            if( m_strSessionId.empty() )
                break;

        }while( 0 );

        if( ERROR( ret ) )
            return ret;

        dwSize = htonl( dwSize );
        memcpy( oBuf.ptr(),
            &dwSize, sizeof( dwSize ) );

        return ret;
    }

    gint32 Deserialize(
        CBuffer& oBuf )
    {
        return Deserialize(
            oBuf.ptr(), oBuf.size() );
    }

    gint32 Deserialize(
        const char* pBuf, guint32 dwMaxSize )
    {
        if( dwMaxSize < sizeof( guint32 ) )
            return -EINVAL;

        guint32 dwSize = ntohl(
            ( ( const guint32* )pBuf )[ 0 ] );

        if( dwSize > dwMaxSize )
            return -EINVAL;

        guint32 dwOffset = sizeof( guint32 );
        gint32 ret = 0;

        do{
            memcpy( &m_qwStart, pBuf + dwOffset,
                sizeof( m_qwStart ) );
            m_qwStart = ntohll( m_qwStart );
            dwOffset += sizeof( m_qwStart );

            memcpy( &m_dwReqSize, pBuf + dwOffset,
                sizeof( m_dwReqSize ) );
            m_dwReqSize = ntohl( m_dwReqSize );
            dwOffset += sizeof( m_dwReqSize );

            memcpy( &m_bUpload, pBuf + dwOffset,
                sizeof( guint8 ) );
            dwOffset += sizeof( guint32 );

            RETRIEVE_STRING( pBuf,
                m_strFileName, dwOffset );
            if( m_strFileName.empty() )
            {
                ret = -EINVAL;
                break;
            }

            RETRIEVE_STRING( pBuf,
                m_strUserName, dwOffset );
            if( m_strUserName.empty() )
                break;

            RETRIEVE_STRING( pBuf,
                m_strSessionId, dwOffset );
            if( m_strSessionId.empty() )
                break;

        }while( 0 );

        return ret;
    }
};

// Declare the interface class
//
// Note that unlike the CEchoServer in Helloworld
// test.  this class inherit from a virtual base
//
// CInterfaceServer. The reason is simple,
// multiple interfaces share the same instance of
// CInterfaceServer.
class CEchoServer :
    public virtual CAggInterfaceServer
{ 
    public: 
    typedef CAggInterfaceServer super;

    CEchoServer(
        const IConfigDb* pCfg )
    : super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( CEchoServer ); }

    gint32 InitUserFuncs();
    gint32 Echo(
        IEventSink* pCallback,
        const std::string& strFile,
        std::string& strReply );
};

// Declare the interface class
class CEchoClient :
    public virtual CAggInterfaceProxy
{
    public:
    typedef CAggInterfaceProxy super;

    const EnumClsid GetIid() const
    { return iid( CEchoServer ); }

    CEchoClient( const IConfigDb* pCfg );
    gint32 InitUserFuncs();
    gint32 Echo( const std::string& strEmit,
        std::string& strReply );
};


template< class T >
class CMyFileServerBase :
    public T
{
    public:
    typedef T super;
    CMyFileServerBase( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {
    }

    gint32 ScheduleToClose( HANDLE hChannel,
        bool bQuitLoop = false )
    {
        TaskletPtr pCloseTask;
        gint32 ret = 0;
        do{
            TaskletPtr pDummy;
            ret = pDummy.NewObj(
                clsid( CIfDummyTask ) );
            if( ERROR( ret ) )
                break;

            ret = DEFER_IFCALLEX_NOSCHED(
                pCloseTask, ObjPtr( this ),
                &T::OnClose,
                hChannel, ( IEventSink* )pDummy );

            if( ERROR( ret ) )
                break;

        }while( 0 );

        if( ERROR( ret ) )
            return ret;

        CMainIoLoop* pLoop = this->m_pRdLoop;
        if( unlikely( pLoop == nullptr ) )
            return -EFAULT;

        if( !bQuitLoop )
            pLoop->AddTask( pCloseTask );
        else
            pLoop->AddQuitTask( pCloseTask );
        return 0;
    }

    gint32 GetTransCtx( HANDLE hChannel,
        ObjPtr& pTransCtx )
    {
        gint32 ret = 0;
        do{
            if( hChannel == INVALID_HANDLE )
            {
                ret = -EINVAL;
                break;
            }

            CfgPtr pCfg;

            // get channel specific context
            ret = this->GetContext( hChannel, pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCtx( ( IConfigDb* )pCfg );
            ret = oCtx.GetObjPtr( 0, pTransCtx );

        }while( 0 );

        return ret;
    }

    gint32 SendBlock( HANDLE hChannel, BufPtr& pBuf )
    {
        if( pBuf.IsEmpty() || pBuf->empty() )
            return -EINVAL;

        if( pBuf->size() > STM_MAX_BYTES_PER_BUF )
            return -EINVAL;

        return this->WriteMsg( hChannel, pBuf, -1 );
    }

    gint32 CheckAndOpenFile(
        CTransferContext* ptctx )
    {
        if( ptctx == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            bool bUpload = ptctx->m_bUpload;
            if( ptctx->m_dwReqSize >
                MAX_BYTES_PER_FILE )
            {
                ret = -ERANGE;
                break;
            }

            std::string strFile = SF_ROOT_DIR;

            struct passwd* pwst =
                getpwuid( getuid() );

            if( pwst == nullptr )
            {
                ret = -errno;
                if( ret == 0 )
                    ret = -ENOENT;
                break;
            }

            std::string strLogin = pwst->pw_name;
            strLogin += "/";

            strFile += strLogin +
                ptctx->m_strFileName;

            ptctx->m_strPathName = strFile;
            gint32& fd = ptctx->m_iFd;

            bool bWrite = false;
            if( bUpload == this->IsServer() )
                bWrite = true;

            if( bWrite )
            {
                // receiving
                // just for demo purpose
                if( this->IsServer() )
                    strFile += "-1";

                ret = access(
                    strFile.c_str(), W_OK );

                guint32 dwFlag = 0;
                if( ret == -1 && errno != ENOENT )
                {
                    ret = -errno;
                    break;
                }
                else if( ret == -1 )
                {
                    ret = 0;
                    dwFlag = O_CREAT |
                        O_WRONLY | O_TRUNC;
                }
                else
                {
                    dwFlag = O_WRONLY;
                    ret = truncate( strFile.c_str(),
                        ptctx->m_qwStart );
                    if( ret == -1 )
                    {
                        if ( errno == EINVAL ||
                            errno == EFBIG )
                            ret = -ERANGE;
                        else
                            ret = -errno;
                        break;
                    }
                }

                fd = open( strFile.c_str(),
                    dwFlag, S_IRUSR | S_IWUSR );
                if( fd <= 0 )
                {
                    ret = -errno;
                    break;
                }
                if( dwFlag == O_WRONLY )
                    lseek( fd, 0, SEEK_END );
            }
            else
            {
                // sending
                if( this->IsServer() )
                    strFile += "-1";

                ret = access( strFile.c_str(), R_OK );
                if( ret == -1 )
                {
                    ret = -errno;
                    break;
                }

                fd = open( strFile.c_str(), O_RDONLY );
                if( fd <= 0 )
                {
                    ret = -errno;
                    break;
                }

                off_t iSize = lseek( fd, 0, SEEK_END );

                lseek( fd, 0, SEEK_SET );
                if( iSize < 0 )
                {
                    ret = -errno;
                    break;
                }
                else if( iSize == 0 )
                {
                    ret = -ENODATA;
                    break;
                }

                guint32 qwReqSize = ptctx->m_qwStart +
                    ptctx->m_dwReqSize;
                if( qwReqSize > ( guint64 )iSize )
                {
                    ret = -ERANGE;
                    break;
                }

                if( ptctx->m_dwReqSize == 0 )
                {
                    ptctx->m_dwReqSize = std::min(
                        ( guint64 )MAX_BYTES_PER_FILE,
                        iSize - ptctx->m_qwStart );
                }

                ret = lseek( fd,
                    ptctx->m_qwStart, SEEK_SET );
                if( ret == -1 )
                {
                    ret = -errno;
                    break;
                }
                ret = 0;
            }
            ptctx->m_qwCurOffset = ptctx->m_qwStart;

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( ptctx->m_iFd >= 0 )
            {
                close( ptctx->m_iFd );
                ptctx->m_iFd = -1;
            }
        }

        return ret;
    }
/**
* @name OnStreamStarted
* @{
* Parameters:
*   hChannel: the handle to the channel which is
*   ready for transfer.
*
* */
/**
 * Actually it is called by the first
 * OnWriteEnabled_Loop, as the channel is
 * established, and ready for transfer. It will
 * initialize the transfer context object for this
 * channel, including open the local file for
 * read/write, and setting up the counters and
 * file pointers.
 * @} */

    virtual gint32 OnStreamStarted( HANDLE hChannel )
    {
        gint32 ret = 0;
        do{
            if( hChannel == INVALID_HANDLE )
            {
                ret = -EINVAL;
                break;
            }

            CfgPtr pDataDesc;
            ret = this->GetDataDesc(
                hChannel, pDataDesc );
            if( ERROR( ret ) )
                break;

            BufPtr pCtxBuf;
            CParamList oCfg(
                ( IConfigDb* )pDataDesc );
            ret = oCfg.GetProperty( 0, pCtxBuf );
            if( ERROR( ret ) )
                break;
            
            ObjPtr pObj;
            ret = pObj.NewObj(
                clsid( CTransferContext ) );

            if( ERROR( ret ) )
                break;

            oCfg.ClearParams();

            CTransferContext* ptctx = pObj;
            ret = ptctx->Deserialize(
                pCtxBuf->ptr(), pCtxBuf->size() );

            if( ERROR( ret ) )
                break;

            ret = CheckAndOpenFile( ptctx );
            if( ERROR( ret ) )
                break;

            BufPtr pBuf( true );
            ret = ptctx->Serialize( *pBuf );
            if( ERROR( ret ) )
                break;

            CfgPtr pCtx;
            ret = this->GetContext( hChannel, pCtx );
            if( ERROR( ret ) )
                break;

            CParamList oCtx( pCtx );
            if( oCtx.exist( 0 ) )
            {
                ret = -EEXIST;
                break;
            }
            oCtx.Push( pObj );

        }while( 0 );

        return ret;
    }

    gint32 ReadAndSend( HANDLE hChannel,
        CTransferContext* ptctx )
    {
        if( ptctx == nullptr )
            return -EINVAL;

        if( ptctx->m_bUpload == this->IsServer() )
            return -EINVAL;

        gint32 ret = 0;
        do{
            BufPtr pBuf( true );
            pBuf->Resize( STM_MAX_BYTES_PER_BUF );
            ret = read( ptctx->m_iFd,
                pBuf->ptr(), pBuf->size() );
            if( ret == -1 )
            {
                ret = -errno;
                break;
            }
            if( ret == 0 )
                break;

            if( ret < STM_MAX_BYTES_PER_BUF )
                pBuf->Resize( ret );

            ret = SendBlock( hChannel, pBuf );
            if( ret == ERROR_QUEUE_FULL )
            {
                ptctx->m_pBufBlocked = pBuf;
                ret = 0;
            }
            else if( ret == STATUS_PENDING )
            {
                ret = 0;
            }

        }while( 0 );

        return ret;
    }
    gint32 ResumeBlockedSend( HANDLE hChannel,
        CTransferContext* ptctx )
    {
        if( ptctx == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            if( !ptctx->m_pBufBlocked.IsEmpty() )
            {
                ret = SendBlock( hChannel,
                    ptctx->m_pBufBlocked );
                if( ret == ERROR_QUEUE_FULL )
                {
                    ret = 0;
                    break;
                }
                ptctx->m_pBufBlocked.Clear();
                if( ret == STATUS_PENDING )
                {
                    ret = 0;
                    break;
                }
            }

        }while( 0 );

        return ret;
    }

    gint32 OnStart_Loop()
    { return 0; }
};

class CMyFileServer;
template<>
gint32 GetIidOfType< const CMyFileServer >(
    std::vector< guint32 >& vecIids, const CMyFileServer* pType );

class CMyFileServer :
    public CMyFileServerBase< CStreamServerSync >
{
    public:
    typedef CMyFileServerBase< CStreamServerSync > super;
    CMyFileServer( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    // mandatory, must be implemented if new
    // interface method be added, GetFileInfo in
    // this case
    const EnumClsid GetIid() const
    { return iid( CMyFileServer ); }

    /**
    * @name SupportIid: given a interface id,
    * to test if the iid is supported by this
    * class.
    *
    * In this case, IStream is supported by
    * CMyFileServer. This method is declared in
    * CAggInterfaceProxy and CAggInterfaceServer.
    *
    * To get the whole thing work, here are some
    * points to notice
    *   1. Override SupportIid to return true if
    *   the iid is intended to be supported by
    *   this class.
    *
    *   2. call super::InitUserFuncs when you
    *   implement the InitUserFuncs of this class,
    *   say, CMyFileServer, and CMyFileProxy, to
    *   make sure the function map and proxy map
    *   are created for each interface.
    *
    *   3. make sure IStream is listed the object
    *   description file, sfdesc.json, for
    *   example. This is to make sure the request
    *   for the IStream interface will be accepted
    *   by the underlying port and deliverred to
    *   the interface object.
    *
    *   4. Instanciate the GetIidOfType for
    *   CMyFileProxy and CMyFileServer, to add
    *   both iid( IStream ) and iid(
    *   CMyFileServer) to the iid array. This step
    *   is to enable the EnumInterfaces to return
    *   the complet set of interfaces supported by
    *   this object.
    *
    * @{
    * */
    /**
     *@} */

    bool SupportIid( EnumClsid iIfId ) const
    {
        if( iIfId == iid( IStream ) )
            return true;
        return false;
    }

    gint32 InitUserFuncs();

    /**
    * @name OnRecvData_Loop
    * @{ an event handler called when there is
    * data arrives to the channel `hChannel'.
    * */
    /**  @} */

    virtual gint32 OnRecvData_Loop(
        HANDLE hChannel, gint32 iRet );
    /**
    * @name OnWriteEnabled_Loop
    * @{ an event handler called when the channel
    * `hChannel' is ready for writing. it could be
    * temporarily blocked if the flowcontrol
    * happens. The first event to the channel is
    * also a WiteEnabled event after the channel
    * is established for read/write. When the flow
    * control happens, the WriteXXX will return
    * ERROR_QUEUE_FULL, and you cannot write more
    * messages to the stream channel.
    * */
    /**  @} */
    virtual gint32 OnWriteEnabled_Loop(
        HANDLE hChannel );

    /**
    * @name OnSendDone_Loop
    * @{ an event handler called when the last
    * pending write is done on the channel.
    * */
    /**  @} */
    virtual gint32 OnSendDone_Loop(
        HANDLE hChannel, gint32 iRet );

    virtual gint32 OnCloseChannel_Loop(
        HANDLE hChannel );

    gint32 GetFileInfo(
        IEventSink* pCallback,
        const std::string& strFile, // [ in ]
        CfgPtr& strResp );    // [ out ]
};

class CMyFileProxy;
template<>
gint32 GetIidOfType< const CMyFileProxy >(
    std::vector< guint32 >& vecIids, const CMyFileProxy* pType );

class CMyFileProxy :
    public CMyFileServerBase< CStreamProxySync >
{
    protected:

    gint32 OnOpenChanComplete(
        IEventSink* pCallback,
        IEventSink* pIoReqTask,
        IConfigDb* pContext );

    gint32 UploadDownload(
        const std::string& strSrcFile,
        bool bUpload );

    public:
    typedef CMyFileServerBase< CStreamProxySync > super;
    CMyFileProxy( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( CMyFileServer ); }

    bool SupportIid( EnumClsid iIfId ) const
    {
        if( iIfId == iid( IStream ) )
            return true;
        return false;
    }

    gint32 StartTransfer( CTransferContext* pCtx );

    // synchronous transfer
    gint32 UploadFile(
        const std::string& strSrcFile );

    gint32 DownloadFile(
        const std::string& strSrcFile );

    gint32 InitUserFuncs();
    /**
    * @name OnRecvData_Loop
    * @{ an event handler called when there is
    * data arrives to the channel `hChannel'.
    * */
    /**  @} */
    virtual gint32 OnRecvData_Loop(
        HANDLE hChannel, gint32 iRet );

    /**
    * @name OnSendDone_Loop
    * @{ an event handler called when the last
    * pending write is done on the channel.
    * */
    /**  @} */
    virtual gint32 OnSendDone_Loop(
        HANDLE hChannel, gint32 iRet );

    virtual gint32 OnWriteEnabled_Loop(
        HANDLE hChannel );

    virtual gint32 OnCloseChannel_Loop(
        HANDLE hChannel );

    gint32 GetFileInfo(
        const std::string& strFile, // [ in ]
        CfgPtr& pResp );    // [ out ]

    void SetErrorAndClose(
        HANDLE hChannel, gint32 iError = 0 );
};
// Declare the major server/proxy objects. Please note
// that, the second class to the last one are
// implementations of the different interfaces
DECLARE_AGGREGATED_PROXY(
    CSimpFileClient,
    CMyFileProxy,
    CEchoClient,
    CStatCountersProxy );

DECLARE_AGGREGATED_SERVER(
    CSimpFileServer,
    CMyFileServer,
    CEchoServer,
    CStatCountersServer );

