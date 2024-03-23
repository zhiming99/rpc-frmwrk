#pragma once
#include <stdio.h>
#include <libgen.h>

#define TRANS_LIMIT ( 2048L * 1024 * 1024 )

struct TransFileContext
{
    gint32 m_iError = 0;
    gint32 m_iBytesLeft = 0;
    gint32 m_iBytesSent = 0;
    bool m_bServer = false;
    stdstr m_strPath;
    gint64 m_iOffset = 0;
    gint64 m_iSize = 0;
    bool m_bRead = false;
    FILE* m_fp = nullptr;
    char m_cDirection = 'u';

    TransFileContext(
        const TransFileContext& rhs )
    {
        m_iError = rhs.m_iError;
        m_iBytesLeft = rhs.m_iBytesLeft;
        m_iBytesSent = rhs.m_iBytesSent;
        m_bServer = rhs.m_bServer;
        m_strPath = rhs.m_strPath;
        m_iOffset = rhs.m_iOffset;
        m_iSize = rhs.m_iSize;
        m_bRead = rhs.m_bRead;
        m_fp = rhs.m_fp;
        m_cDirection = rhs.m_cDirection;
    }
    TransFileContext( const stdstr& strPath )
    { m_strPath = strPath; }

    inline void SetError( gint32 iError )
    { m_iError = iError; }

    inline gint32 GetError() const
    { return m_iError; }

    inline bool IsDone() const
    { return m_fp == nullptr; }

    stdstr GetFileName(
        const stdstr& strPath ) const
    {
        BufPtr pBuf( true );
        *pBuf = strPath;
        stdstr strBase =
            basename( pBuf->ptr() );
        return strBase;
    }

    stdstr GetDirName(
        const stdstr& strPath ) const
    {
        BufPtr pBuf( true );
        *pBuf = strPath;
        stdstr strdir =
            dirname( pBuf->ptr() );
        return strdir;
    }

    gint32 OpenFile()
    {
        if( m_strPath.empty() )
            return ERROR_STATE;
        gint32 ret = 0;
        do{
            if( ( m_cDirection == 'u' && !m_bServer ) ||
                ( m_cDirection == 'd' && m_bServer ) )
            {
                m_bRead = true;
                m_fp = fopen( m_strPath.c_str(), "rb" );
                if( m_fp == nullptr )
                {
                    ret = -errno;
                    break;
                }
                fseek( m_fp, 0, SEEK_END );
                gint64 iSize = ftell( m_fp );
                if( iSize == -1 )
                {
                    ret = -errno;
                    break;
                }
                if( iSize == 0 )
                {
                    ret = -ENODATA;
                    break;
                }
                if( m_iOffset > iSize || m_iOffset < 0 )
                {
                    ret = -ERANGE;
                    break;
                }
                fseek( m_fp, m_iOffset, SEEK_SET );
                if( m_iSize > iSize - m_iOffset ||
                    m_iSize == 0 )
                    m_iSize = iSize - m_iOffset;

                if( m_iSize > TRANS_LIMIT )
                {
                    ret = -ERANGE;
                    break;
                }
            }
            else
            {
                if( m_iSize == 0 )
                {
                    ret = -ERANGE;
                    break;
                }
                m_fp = fopen(
                    m_strPath.c_str(), "wb+" );

                if( m_fp == nullptr )
                {
                    ret = -errno;
                    break;
                }
            }

            m_iBytesLeft = m_iSize;

        }while( 0 );

        return ret;
    }

    void CleanUp()
    {
        if( m_fp != nullptr )
        {
            fclose( m_fp );
            m_fp = nullptr;
        }
    }
};

enum{
    DECL_CLSID( TransferContext ) = clsid( UserClsidStart ) + 1001,
    DECL_CLSID( TransferContextCli ),
};

template< class T >
struct TransferContext :
    public CObjBase
{
    T* m_pInst;
    std::hashmap< HANDLE, TransFileContext > m_mapChanToCtx;
    stdrmutex m_oLock;

    TransferContext( T* pIf )
    {
        m_pInst = pIf;
        SetClassId( clsid( TransferContext ) );
    };

    T& oInst() { return *m_pInst; }

    stdrmutex& GetLock()
    { return m_oLock; }

    gint32 AddContext( HANDLE hChannel,
        TransFileContext& o )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr != m_mapChanToCtx.end() )
            return -EEXIST;
        m_mapChanToCtx.insert( { hChannel, o } );
        return STATUS_SUCCESS;
    }

    gint32 RemoveContext( HANDLE hChannel )
    {
        CStdRMutex oLock( GetLock() );
        m_mapChanToCtx.erase( hChannel );
        return 0;
    }

    virtual void OnTransferDone(
        HANDLE hChannel, gint32 iRet )
    {
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr == m_mapChanToCtx.end() )
            return;
        itr->second.CleanUp();
        m_mapChanToCtx.erase( itr );
    }

    gint32 ReadFileAndSend( HANDLE hChannel )
    {
        gint32 ret = STATUS_SUCCESS;
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr == m_mapChanToCtx.end() )
            return -ENOENT;

        size_t iSizeLimit = 2 * 1024 * 1024;
        auto& o = itr->second;
        BufPtr pBuf( true );
        pBuf->Resize( iSizeLimit );
        while( true )
        {
            size_t iSize = o.m_iBytesLeft;
            if( iSize > iSizeLimit )
                iSize = iSizeLimit;
            else if( iSize == 0 )
            {
                OnTransferDone( hChannel, ret );
                break;
            }
            else if( iSize < iSizeLimit )
                pBuf->Resize( iSize );

            ret = fread( pBuf->ptr(),
                pBuf->size(), 1, o.m_fp );
            if( ret < 0 )
            {
                ret = -errno;
                break;
            }
            ret = oInst().WriteStreamAsync(
                hChannel, pBuf,
                ( IConfigDb* )nullptr );
            if( ERROR( ret ) )
            {
                OnTransferDone( hChannel, ret );
                break;
            }
            // return immediately if pending
            if( ret == STATUS_PENDING )
                break;
            o.m_iBytesLeft -= iSize;
            o.m_iBytesSent += iSize;
        }
        return ret;
    }

    gint32 SendToken( HANDLE hChannel, BufPtr& pBuf )
    {
        if( pBuf.IsEmpty() || pBuf->empty() )
            return -EINVAL;
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;
        return oInst().WriteStreamNoWait( hChannel, pBuf );
    }

    gint32 WriteFileAndRecv( HANDLE hChannel, BufPtr& pBufw )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        size_t iSizeLimit = 2 * 1024 * 1024;
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr == m_mapChanToCtx.end() )
            return -ENOENT;

        gint32 ret = STATUS_SUCCESS;
        auto& o = itr->second;
        BufPtr pBuf = pBufw;
        while( true )
        {
            gint32 iSize = 0;
            if( !pBuf.IsEmpty() && !pBuf->empty() )
                iSize = pBuf->size();
            else
                pBuf.NewObj();

            if( o.m_iBytesLeft < iSize )
            {
                ret = -ERANGE;
                break;
            }
            if( iSize > 0 )
            {
                fwrite( pBuf->ptr(), iSize, 1, o.m_fp );
                o.m_iBytesLeft -= iSize;
                o.m_iBytesSent += iSize;
                OutputMsg( ret, "Received %d, To receive %d",
                    o.m_iBytesSent, o.m_iBytesLeft );
            }

            size_t iSizeRecv = o.m_iBytesLeft;
            if( iSizeRecv > iSizeLimit )
                iSizeRecv = iSizeLimit;
            if( iSizeRecv == 0 )
            {
                OnTransferDone( hChannel, ret );
                if( oInst().IsServer() )
                {
                    OutputMsg( ret, "send 'over' token" );
                    stdstr strMsg = "over";
                    BufPtr pBuf2( true );
                    *pBuf2 = strMsg;

                    DEFER_CALL( oInst().GetIoMgr(),
                        ObjPtr( this ),
                        &TransferContext::SendToken,
                        hChannel, pBuf2 );
                }
                break;
            }

            pBuf->Resize( iSizeRecv );
            ret = oInst().ReadStreamAsync( hChannel,
                pBuf, ( IConfigDb* )nullptr );
            if( ERROR( ret ) )
                break;
            if( ret == STATUS_PENDING )
                break;
        }
        if( ERROR( ret ) )
            OnTransferDone( hChannel, ret );
        return ret;
    }

    gint32 OnWriteStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx ) 
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr == m_mapChanToCtx.end() )
            return -ENOENT;
        auto& o = itr->second;
        if( ERROR( iRet ) )
        {
            OnTransferDone( hChannel, iRet );
            return iRet;
        }

        o.m_iBytesLeft -= pBuf->size();
        o.m_iBytesSent += pBuf->size();
        OutputMsg( iRet, "Sent %d, To send %d",
            o.m_iBytesSent, o.m_iBytesLeft );

        return ReadFileAndSend( hChannel );
    }

    gint32 OnReadStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;
        if( ERROR( iRet ) )
        {
            OnTransferDone( hChannel, iRet );
            return iRet;
        }
        return WriteFileAndRecv( hChannel, pBuf );
    }

    gint32 OnWriteResumed( HANDLE hChannel )
    {
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr == m_mapChanToCtx.end() )
            return -ENOENT;
        auto& o = itr->second;

        OutputMsg( 0, "Send Resumed with %d sent, %d to send",
            o.m_iBytesSent, o.m_iBytesLeft );

        return ReadFileAndSend( hChannel );
    }

    void SetError( HANDLE hChannel, gint32 iError )
    {
        if( hChannel == INVALID_HANDLE )
            return;
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr == m_mapChanToCtx.end() )
            return;
        auto& o = itr->second;
        o.SetError( iError );
    }

    gint32 GetError( HANDLE hChannel )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapChanToCtx.find( hChannel );
        if( itr == m_mapChanToCtx.end() )
            return -ENOENT;
        auto& o = itr->second;
        return o.GetError();
    }
};

template< class T >
struct TransferContextCli :
    public TransferContext< T >
{
    sem_t m_semWait;
    typedef TransferContext< T > super;

    TransferContextCli( T* pIf )
        : super( pIf )
    {
        Sem_Init( &m_semWait, 0, 0 );
        this->SetClassId( clsid( TransferContextCli ) );
    }

    void OnTransferDone(
        HANDLE hChannel, gint32 iRet )
    {
        if( hChannel == INVALID_HANDLE )
            return;
        CStdRMutex oLock( this->GetLock() );
        auto itr = this->m_mapChanToCtx.find( hChannel );
        if( itr == this->m_mapChanToCtx.end() )
            return;
        auto& o = itr->second;
        if( o.IsDone() )
            return;
        o.SetError( iRet );
        o.CleanUp();
        NotifyComplete();
    }

    void NotifyComplete()
    { Sem_Post( &m_semWait ); }

    void WaitForComplete()
    { Sem_Wait( &m_semWait ); }
};

bool IsDirExist(const std::string& path );
gint32 MakeDir( const stdstr& strPath );
bool IsFileExist(const std::string& path );
size_t GetFileSize(const std::string& path );
bool IsFileNameValid( const stdstr& strFile );

