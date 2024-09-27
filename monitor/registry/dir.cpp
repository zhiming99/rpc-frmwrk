/*
 * =====================================================================================
 *
 *       Filename:  dir.cpp
 *
 *    Description:  implemetation of CDirFileEntry and related classes 
 *
 *        Version:  1.0
 *        Created:  09/23/2024 12:04:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2024 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "blkalloc.h"
namespace rpcf
{
RegFSBNode::RegFSBNode();
{
    m_pBuf.NewObj();
    m_pBuf->Resize( BNODE_SIZE );
    memset( m_pBuf->ptr(), 0, m_pBuf->size() );
    m_vecSlots.resize( m_dwMaxSlots );
    auto p = ( KEYPTR_SLOT* )m_pBuf->ptr();
    for( elem : m_vecSlots )
        elem = p++;
}

RegFSBNode::ntoh(
    guint8* pSrc, guint32 dwSize )
{
    gint32 ret = 0;
    if( pSrc == nullptr || dwSize < BNODE_SIZE )
        return -EINVAL;
    do{
        auto arrSrc = ( KEYPTR_SLOT* )pSrc;
        guint16* pParams = arrSrc +
            sizeof( KEYPTR_SLOT ) *
            MAX_PTRS_PER_NODE;

        m_wNumKeys = ntohs( pParams[ 0 ] );
        m_wNumBNodeIdx = ntohs( pParams[ 1 ] );
        m_wThisBNodeIdx = ntohs( pParams[ 2 ] );
        m_wParentBNode = ntohs( pParams[ 3 ] );
        m_wNextLeaf = ntohs( pParams[ 4 ] );
        m_bLeaf = *( bool* )( pParams + 5 );
        if( m_wNumBNodeIdx >= MAX_PTRS_PER_NODE ||
            m_wNumKeys > MAX_KEYS_PER_NODE ||
            m_wParentBNode >= MAX_BNODE_NUMBER ||
            m_wNextLeaf >= MAX_BNODE_NUMBER ) 
        {
            ret = -EINVAL;
            break;
        }
        m_wFreeBNodeIdx = ntohs( pParams[ 6 ] );

        memcpy( m_pBuf->ptr(), pSrc,
            sizeof( KEYPTR_SLOT ) *
            ( m_wNumKeys + 1 ) );

        int i = 0;
        KEYPTR_SLOT* p = m_pBuf->ptr();
        for( ; i < MAX_PTRS_PER_NODE; i++ )
        {
            if( m_bLeaf )   
            {
                p->oLeaf.dwInodeIdx = 
                    ntohl( p->oLeaf.dwInodeIdx );
            }
            else
            {
                p->dwBNodeIdx = 
                    ntohl( p->dwBNodeIdx );
            }
        }

    }while( 0 );
    return ret;
}

RegFSBNode::hton(
    guint8* pSrc, guint32 dwSize )
{
    gint32 ret = 0;
    if( pSrc == nullptr || dwSize < BNODE_SIZE )
        return -EINVAL;
    do{
        auto arrSrc = ( KEYPTR_SLOT* )pSrc;
        memcpy( pSrc, m_pBuf->ptr(),
            sizeof( KEYPTR_SLOT ) *
            ( m_wNumKeys + 1 ) );
        auto p = ( KEYPTR_SLOT* )m_pBuf->ptr();
        for( int i = 0; i < m_wNumKeys + 1; i++ )
        {
            if( m_bLeaf )   
            {
                arrSrc[ i ]->oLeaf.dwInodeIdx = 
                    ntohl( p->oLeaf.dwInodeIdx );
            }
            else
            {
                arrSrc[ i ]->dwBNodeIdx = 
                    ntohl( p->dwBNodeIdx );
            }
        }

        guint16* pParams = arrSrc +
            sizeof( KEYPTR_SLOT ) *
            MAX_PTRS_PER_NODE;

        pParams[ 0 ] = ntohs( m_wNumKeys );
        pParams[ 1 ] = ntohs( m_wNumBNodeIdx );
        pParams[ 2 ] = ntohs( m_wThisBNodeIdx );
        pParams[ 3 ] = ntohs( m_wParentBNode );
        pParams[ 4 ] = ntohs( m_wNextLeaf );
        *( bool* )( pParams + 5 ) = m_bLeaf;
        
        pParams[ 6 ] = ntohs( m_wFreeBNodeIdx );

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Reload()
{
    gint32 ret = 0;
    do{
        __attribute__((aligned (8)))
        guint8 arrBytes[ BNODE_SIZE ];
        ret = m_pFile->ReadFile(
            BNODE_IDX_TO_POS( m_dwThisBNodeIdx ),
            arrBytes, sizeof( arrBytes ) );
        if( ERROR( ret ) )
            break;
        auto pBPNode = &m_oBNodeStore;
        ret = pBPNode->ntoh( arrBytes, dwSize );
        if( ERROR( ret ) )
            break;

        if( pBPNode->m_bLeaf )
            break;
        
        for( int i = 0; i <
            pBPNode->m_wNumBNodeIdx; i++ )
        {
            KEYPTR_SLOT* p = m_vecSlots[ i ];
            m_vecChilds[ i ].reset(
                new CBPlusNode( m_pFile,
                    p->m_dwThisBNodeIdx ) );
            m_vecChilds[ i ]->Reload();
        }

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Flush()
{
    gint32 ret = 0;
    do{
        __attribute__((aligned (8)))
        guint8 arrBytes[ BNODE_SIZE ];
        auto pBPNode = &m_oBNodeStore;
        ret = pBPNode->hton( arrBytes, dwSize );
        if( ERROR( ret ) )
            break;

        if( pBPNode->m_bLeaf )
            break;

        for( auto& elem : m_vecChilds )
        {
            ret = elem->Flush();
            if( ERROR( ret ) )
                break;
        }
    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Format()
{
    gint32 ret = 0;
    do{
        __attribute__((aligned (8)))
        guint8 arrBytes[ BNODE_SIZE ];
        auto pBPNode = &m_oBNodeStore;
        ret = pBPNode->hton( arrBytes, dwSize );
        if( ERROR( ret ) )
            break;

        ret = m_pFile->WriteFile(
            BNODE_IDX_TO_POS( m_dwThisBNodeIdx ),
            sizeof( arrBytes ), arrBytes );

        if( ERROR( ret ) )
            break;

        if( IsLeaf() )
        {
            m_vecFiles.resize(
                MAX_KEYS_PER_NODE );
        }
        else
        {
            m_vecChilds.resize(
                MAX_PTRS_PER_NODE );
        }
    }while( 0 );
    return ret;
}

gint32 FREE_BNODES::hton(
    guint8* pDest, guint32 dwSize )
{
    gint32 ret = 0;
    if( pDest == nullptr || dwSize == 0 )
        return -EINVAL;
    auto plhs = ( FREE_BNODES* )pDest;
    for( guint32 i = 0; i < m_wBNCount; i++ )
    {
        plhs->m_arrFreeBNIdx[ i ] =
            htons( m_arrFreeBNIdx[ i ] );
    }
    plhs->m_wBNCount = htons( m_wBNCount );
    return 0;
}

gint32 FREE_BNODES::ntoh(
    guint8* pSrc, guint32 dwSize )
{
    gint32 ret = 0;
    auto prhs = ( FREE_BNODES* )pSrc;
    if( prhs->m_wBNCount == 0 )
        return 0;
    m_wBNCount = ntohs( prhs->m_wBNCount );
    if( m_wBNCount > GetMaxCount() )
        return -EINVAL;
    for( guint32 i = 0; i < m_wBNCount; i++ )
    {
        m_arrFreeBNIdx[ i ] =
            ntohs( prhs->m_arrFreeBNIdx[ i ] );
    }
    return 0;
}

gint32 CFreeBNodePool::Reload()
{
    gint32 ret = 0;

    guint32 dwBNodeIdx =
    m_pDir->GetFreeBNodeIdx();
    if( dwBNodeIdx == INVALID_BNODE_IDX )
        return 0;

    __attribute__((aligned (8)))
    guint8 arrBytes[ BNODE_SIZE ];
    do{
        ret = m_pDir->ReadFile(
            BNODE_IDX_TO_POS( dwBNodeIdx ),
            sizeof( arrBytes ), arrBytes );
        if( ERROR( ret ) )
            break;
        BufPtr pBuf( true );        
        pBuf->Resize( BNODE_SIZE );
        FREE_BNODES* pfb = pBuf->ptr();
        ret = pfb->ntoh(
            arrBytes, sizeof( arrBytes ) );
        if( ERROR( ret ) )
            break;
        m_vecFreeBNodes.push_back(
            { dwBNodeIdx, pBuf } );
        if( !pfb->IsFull() )
            break;
        dwBNodeIdx = pfb->GetLastBNodeIdx();
    }while( dwBNodeIdx != INVALID_BNODE_IDX );
    return ret;
}

gint32 CFreeBNodePool::InitPoolStore(
    guint32 dwBNodeIdx )
{
    gint32 ret = 0;
    do{
        if( dwBNodeIdx == INVALID_BNODE_IDX )
            break;
        BufPtr pBuf( true );
        pBuf->Resize( BNODE_SIZE );
        FREE_BNODES* pfb = pBuf->ptr();
        pfb->m_wBNCount = 0;
        pfb->m_bNextBNode = false;
        for( int i = 0; i < pfb->GetMaxCount(); i++ )
            m_arrFreeBNIdx[ 0 ] = INVALID_BNODE_IDX;

        __attribute__((aligned (8)))
        guint8 arrBytes[ BNODE_SIZE ];

        ret = pfb->hton(
            arrBytes, sizeof( arrBytes ) );
        if( ERROR( ret ) )
            break;
        ret = m_pDir->WriteFile(
            BNODE_IDX_TO_POS( dwBNodeIdx ),
            BNODE_SIZE, pfb );
        if( ERROR( ret ) )
            break;
        if( m_vecFreeBNodes.empty )
            m_pDir->SetHeadFreeBNode(
                dwBNodeIdx );
        m_vecFreeBNodes.push_back(
            { dwBNodeIdx, pBuf } );
    }while( 0 );
    return ret;
}

gint32 CFreeBNodePool::Flush()
{
    gint32 ret = 0;
    guint32 dwBNodeIdx =
        m_pDir->GetFreeBNodeIdx();
    if( dwBNodeIdx == INVALID_BNODE_IDX )
        return 0;

    __attribute__((aligned (8)))
    guint8 arrBytes[ BNODE_SIZE ];
    for( auto& elem : m_vecFreeBNodes )
    {
        FREE_BNODES* pfb = elem.second->ptr();
        ret = pfb->hton(
            arrBytes, sizeof( arrBytes ) );
        if( ERROR( ret ) )
            break;
        ret = m_pDir->WriteFile(
            BNODE_IDX_TO_POS( elem.first ),
            sizeof( arrBytes ), arrBytes );
    }
    return 0;
}

gint32 CFreeBNodePool::PutFreeBNode(
    guint32 dwBNodeIdx )
{
    gint32 ret = 0;
    do{
        ret = ReleaseFreeBNode( dwBNodeIdx );
        if( SUCCEEDED( ret ) )
            break;
        if( m_pDir->GetHeadFreeBNode() ==
            INVALID_BNODE_IDX )
        {
            ret = InitPoolStore(
                dwBNodeIdx );
            break;
        }
        do{
            if( m_vecFreeBNodes.empty() )
            {
                ret = -ENOENT;
                break;
            }
            auto& elem = m_vecFreeBNodes.back();
            FREE_BNODES* pfb = elem.second->ptr();
            if( pfb->IsFull() )
            {
                guint32 dwNewPool =
                    pfb->GetLastBNodeIdx();
                ret = InitPoolStore( dwNewPool );
                if( ERROR( ret ) )
                    break;
                continue;
            }
            ret = pfb->PushFreeBNode(
                dwBNodeIdx );
            break;
        }while( 1 );
    }while( 0 );
    return ret;
}

gint32 CFreeBNodePool::ReleaseFreeBNode(
    guint32 dwBNodeIdx )
{
    gint32 ret = 0;
    if( dwBNodeIdx == INVALID_BNODE_IDX )
        return -EINVAL;
    do{
        gint32 dwPos =
            BNODE_IDX_TO_POS( dwBNodeIdx );
        if( dwPos + BNODE_SIZE ==
            m_pDir->GetSize() )
        {
            ret = m_pDir->Truncate( dwPos );
            break;
        }
        ret = ERROR_FAIL;
    }while( 0 );
    return ret;
}

gint32 CFreeBNodePool::GetFreeBNode(
    guint32& dwBNodeIdx )
{
    gint32 ret = 0;
    do{
        guint32 dwFirstFree =
            m_pDir->GetHeadFreeBNode();
        if( dwFirstFree == INVALID_BNODE_IDX )
        {
            ret = -ENOENT;
            break;
        }
        do{
            if( m_vecFreeBNodes.empty() )
            {
                m_pDir->SetHeadFreeBNode(
                    INVALID_BNODE_IDX );
                // return the last one
                dwBNodeIdx = dwFirstFree;
                break;
            }
            auto& elem = m_vecFreeBNodes.back();
            FREE_BNODES* pfb = elem.second->ptr();
            if( pfb->IsEmpty() )
            {
                m_vecFreeBNodes.pop_back();
                continue;
            }
            guint16 wBNodeIdx;
            ret = pfb->PopFreeBNode( wBNodeIdx );
            if( ERROR( ret ) )
                break;
            dwBNodeIdx = wBNodeIdx;
            break;
        }while( 1 );
    }while( 0 );
    return ret;
}

gint32 CDirImage::Flush()
{
    gint32 ret = 0;
    do{
        ret = super::Flush();
        if( ERROR( ret ) )
            break;
        ret = m_pRootNode->Flush();
        if( ERROR( ret ) )
            break;
        ret = m_pFreePool->Flush();
    }while( 0 );
    return ret;
}
gint32 CDirImage::Format()
{
    gint32 ret = 0;
    do{
        ret = super::Format();
        if( ERROR( ret ) )
            break;
        m_oInodeStore.m_dwMode = S_IFDIR;
        m_pRootNode.reset(
            new CBPlusNode( this, 0 ) );
        ret = m_pRootNode->Format();
        if( ERROR( ret ) )
            break;

        guint32 dwIdx =
            m_pRootNode->GetFreeBNodeIdx();

        m_pFreePool.reset(
            new CFreeBNodePool( this, dwIdx ) );

    }while( 0 );

    return ret;
}

gint32 CDirImage::Reload()
{
    gint32 ret = 0;
    do{
        ret = super::Reload();
        if( ERROR( ret ) )
            break;
        guint32& dwMode = 
            m_oInodeStore.m_dwMode;
        if( !( dwMode & S_IFDIR ) )
        {
            ret = -EINVAL;
            break;
        }
        m_pRootNode.reset(
            new CBPlusNode( this, 0 ) );
        ret = m_pRootNode->Reload();
        if( ERROR( ret ) )
            break;
        m_pFreePool->Reload();
    }while( 0 );
    return ret;
}

gint32 BinSearch(
    const char* szKey,
    CBPlusNode* pNode,
    gint32 iOrigLower
    gint32 iOrigUpper )
{
    gint32 ret = 0;

    gint32 iLower = iOrigLower;
    gint32 iUpper = iOrigUpper;
    
    do{
        if( iUpper == iLower )
        {
            gint32 iRet = strncmp( szKey,
                pNode->GetKey( iLower ] ),
                REGFS_NAME_LENGTH - 1 );
            if( iRet < 0 )
            {
                ret = -( iLower - 1 );
                break;
            }
            if( iRet < 0 )
            {
                ret = - iLower;
                break;
            }
            ret = iLower;    
            break;
        }
        if( iUpper == iLower + 1 )
        {
            gint32 iRet = strncmp( szKey,
                pNode->GetKey( iLower ] ),
                REGFS_NAME_LENGTH - 1 );
            if( iRet < 0 )
            {
                ret = -iLower;
                break;
            }
            if( iRet == 0 )
            {
                ret = iLower;
                break;
            }
            iRet = strncmp( szKey,
                pNode->GetKey( iLower ] ),
                REGFS_NAME_LENGTH - 1 );
            if( iRet < 0 )
            {
                ret = -iUpper;
                break;
            }
            else
            {
                ret = iUpper;
                break;
            }
            break;
        }
        gint32 iCur =
            ( iLower + iUpper ) >> 1;
        iRet = strncmp( szKey,
            pNode->GetKey( iLower ] ),
            REGFS_NAME_LENGTH - 1 );
        if( iRet == 0 )
        {
            ret = iLower;
            break;
        }
        if( iRet < 0 )
            iUpper = iCur - 1;
        else
            iLower = iCur + 1;

        iCur = ( iLower + iUpper ) >> 1;

    }while( 1 );

    return ret;
}

bool CDirImage::Search( const char* szKey )
{
    bool ret = false;
    do{
        CBPlusNode* pCurNode =
            m_pRootNode->get(); 

        guint32 dwCount = 0;
        while( !pCurNode->IsLeaf() )
        {
            dwCount = pCurNode->GetKeyCount();
            gint32 i = BinSearch(
                szKey, pCurNode, 1, dwCount );
            if( i > 0 )
            {
                ret = true;
                break;
            }
            i = -i;
            pCurNode = pCurNode->GetChild( i );
        }

        dwCount = pCurNode->GetKeyCount();
        gint32 i = BinSearch(
            szKey, pCurNode, 1, dwCount );
        if( i > 0 )
            ret = true;
    }while( 0 );
    return ret;
}

gint32 CDirImage::Insert( KEYPTR_SLOT* pSlot )
{
    gint32 ret = 0;
    do{
        CBPlusNode* pCurNode =
            m_pRootNode->get(); 
        if( pCurNode->GetKeyCount() ==
            MAX_KEYS_PER_NODE )
        {}
    }while( 0 );
    return ret;
}

}
