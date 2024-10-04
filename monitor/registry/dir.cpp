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

#define MIN_PTRS( _maxptrs_, _root_ ) \
( (_root) ? 2 : \
    ( ( (_maxptrs_) & 1 ) ? ( (_maxptrs_) + 1 ) / 2 ) : \
    ( (_maxptrs_) / 2 ) )

#define MIN_KEYS( _maxkeys_, _root_ ) \
( (_root) ? 1 : \
    ( ( ( (_maxkeys_) & 1 ) ? ( ( (_maxkeys_) + 1 ) / 2 - 1 ) ) : \
    ( (_maxkeys_) / 2 - 1 ) ) )

#define SPLIT_POS( _x_, _leaf_ ) \
( ( ( (_x_) & 1 ) ? \
    ( (_leaf_) ? ( ( (_x_)- 1 ) / 2 ) : \
        ( ( (_x_) + 1 ) / 2 - 1 ) ) : \
    ( (_leaf_) ? ( (_x_) / 2 ) : \
        ( ( (_x_) / 2 ) - 1 ) ) ) - 1 ) \

namespace rpcf
{
RegFSBNode::RegFSBNode()
{
    m_pBuf.NewObj();
    m_pBuf->Resize( BNODE_SIZE );
    memset( m_pBuf->ptr(), 0, m_pBuf->size() );
    m_vecSlots.resize( m_dwMaxSlots );
    auto p = ( KEYPTR_SLOT* )m_pBuf->ptr();
    // make the first element always be the
    // first
    p->szKey[ 0 ] = 0;
    m_wNumKeys = 1;
    m_wNumPtrs = 0;
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
        m_wNumPtrs = ntohs( pParams[ 1 ] );
        m_wThisBNodeIdx = ntohs( pParams[ 2 ] );
        m_wParentBNode = ntohs( pParams[ 3 ] );
        m_wNextLeaf = ntohs( pParams[ 4 ] );
        m_bLeaf = *( bool* )( pParams + 5 );
        if( m_wNumPtrs >= MAX_PTRS_PER_NODE ||
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
        pParams[ 1 ] = ntohs( m_wNumPtrs );
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
        ret = m_pDir->ReadFile(
            BNODE_IDX_TO_POS( GetBNodeIndex() ),
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
            pBPNode->m_wNumPtrs; i++ )
        {
            KEYPTR_SLOT* p = m_vecSlots[ i ];
            guint32 dwIdx = p->wBNodeIdx;
            auto& pNode = m_mapChilds[ dwIdx ];
            pNode.reset(
                new CBPlusNode( m_pDir, dwIdx ) );
            ret = pNode->Reload();
            if( ERROR( ret ) )
                break;
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

        for( auto& elem : m_mapChilds )
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

        ret = m_pDir->WriteFile(
            BNODE_IDX_TO_POS( GetBNodeIndex() ),
            sizeof( arrBytes ), arrBytes );

        if( ERROR( ret ) )
            break;

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::RemoveSlotAt(
    gint32 idx, KEYPTR_SLOT& oKey )
{
    if( idx < 0 || pKey == nullptr )
        return  -EINVAL;

    gint32 ret = 0;
    do{
        auto& vec = m_oBNodeStore.m_vecSlots;
        if( idx >= MAX_KEYS_PER_NODE )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwCount = this->GetKeyCount();
        if( dwCount == 0 )
        {
            ret = -ENOMEM;
            DebugPrint( ret,
                "Error no key to remove" );
            break;
        }
        if( idx > dwCount )
        {
            ret = -ENOENT;
            break;
        }
        oKey = *vec[ idx ];

        guint32 i = idx;
        for( ; i <= dwCount; i++ )
        {
            memcpy( vec[ i ],
                vec[ i + 1 ], sizeof( KEYPTR_SLOT ) );
        }

        this->DecKeyCount();
        if( !IsLeaf() )
            this->DecChildCount();

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::AppendSlot(
    KEYPTR_SLOT* pKey )
{
    if( pKey == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        guint32 dwCount = this->GetKeyCount();
        if( !IsLeaf() &&
            GetChildCount() == MAX_PTRS_PER_NODE )
        {
            ret = -EOVERFLOW;
            break;
        }
        else if( IsLeaf() &&
            dwCount == MAX_KEYS_PER_NODE )
        {
            ret = -EOVERFLOW;
            break;
        }

        auto& vec = m_oBNodeStore.m_vecSlots;
        if( IsLeaf() )
        {
            memcpy( vec[ dwCount ],
                pKey, sizeof( KEYPTR_SLOT ) );
        }
        else
        {
            KEYPTR_SLOT oNewLast;
            strncpy( vec[ dwCount ]->szKey,
                pKey->szKey,
                sizeof( pKey->szKey ) - 1 );
            oNewLast.dwBNodeIdx = pKey->dwBNodeIdx;
            oNewLast.szKey[ 0 ] = 0;
            memcpy( vec[ dwCount + 1 ],
                &oNewLast, sizeof( KEYPTR_SLOT ) );
        }
        this->IncKeyCount();
        if( !IsLeaf() )
            this->IncChildCount();

    }while( 0 );
}
gint32 CBPlusNode::InsertSlotAt(
    gint32 idx, KEYPTR_SLOT* pKey )
{
    if( idx < 0 || pKey == nullptr )
        return  -EINVAL;

    gint32 ret = 0;
    do{
        auto& vec = m_oBNodeStore.m_vecSlots;
        if( idx >= MAX_KEYS_PER_NODE )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwCount = this->GetKeyCount();
        if( dwCount >= MAX_KEYS_PER_NODE )
        {
            ret = -ENOMEM;
            DebugPrint( ret,
                "Error child keys overflow" );
            break;
        }
        if( idx > dwCount + 1 )
        {
            ret = -EINVAL;
            break;
        }
        guint32 i = dwCount;
        for( ; i > idx; i-- )
        {
            memcpy( vec[ i ],
                vec[ i - 1 ], sizeof( KEYPTR_SLOT ) );
        }
        memcpy( vec[ idx ],
            pKey, sizeof( KEYPTR_SLOT ) );

        this->IncKeyCount();
        if( !IsLeaf() && pKey->dwBNodeIdx !=
            INVALID_BNODE_IDX )
        {
            guint32 dwIdx = p->wBNodeIdx;
            auto itr = m_mapChilds.find( dwIdx );
            if( itr->get() == nullptr )
            {
                auto& pNode = m_mapChilds[ dwIdx ];
                pNode.reset(
                    new CBPlusNode( m_pDir, dwIdx ) );
                ret = pNode->Reload();
            }
            this->IncChildCount(
                this->GetChildCount() + 1 );
        }

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::MoveBNodeStore(
    CBPlusNode* pSrc, guint32 dwSrcOff,
    guint32 dwCount, guint32 dwDstOff )
{
    gint32 ret = 0;
    if( dwSrcOff + dwCount >= MAX_KEYS_PER_NODE ||
        dwDstOff + dwCount >= MAX_KEYS_PER_NODE )
        return -EINVAL;
    do{
        guint32 dwActCount =
            pSrc->GetKeyCount();
        auto pSrc = ( KEYPTR_SLOT* )
            pSrc->m_oBNodeStore.m_pBuf->ptr();
        auto pDst = ( KEYPTR_SLOT* )
            m_oBNodeStore.m_pBuf->ptr();
        guint32 i = 0;
        pDst += dwDstOff;
        pSrc += dwSrcOff;
        for( ; i < dwCount; i++, pDst++, pSrc++ )
        {
            memcpy( pDst, pSrc,
                sizeof( KEYPTR_SLOT) );
            if( IsLeaf() )
            {
                FImgSPtr pFile;
                gint32 iRet =
                    pSrc->RemoveFile( i, pFile );
                if( ERROR( iRet ) )
                    continue;
                this->AddFileDirect(
                    pDst->dwInodeIdx,
                    pFile );
            }
            else
            {
                BNodeUPtr pNode;
                gint32 iRet =
                    pSrc->RemoveChild( i, pNode );
                if( ERROR( iRet ) )
                    continue;
                this->AddChildDirect(
                    pDst->dwBNodeIdx,
                    pNode );
            }
        }

        pSrc->SetKeyCount( dwActCount - dwCount );
        SetKeyCount( GetKeyCount() + dwCount );

        if( !IsLeaf() )
        {
            pSrc->SetChildCount(
                pSrc->GetChildCount() - dwCount );
            SetChildCount(
                GetKeyCount() + dwCount + 1 );
        }

    }while( 0 );
    return ret;
}

void CBPlusNode::InsertNonFull(
    KEYPTR_SLOT* pKey )
{
    gint32 ret = 0;
    if( pChild.get() == nullptr ||
        pKey == nullptr )
        return -EINVAL;

    do{
        gint32 iRet = this->BinSearch(
            pKey->szKey, 0,
            this->GetKeyCount() - 1 );

        if( iRet > 0 )
        {
            ret = -EEXIST;
            break;
        }
        guint32 dwIdx = ( guint32 )( -iRet );
        if( dwIdx > 0x10000 )
            dwIdx -= 0x10000;

        if( dwIdx >= MAX_KEYS_PER_NODE )
        {
            ret = ERROR_FAIL;
            break;
        }

        if( IsLeaf() )
        {
            ret = InsertSlotAt( dwIdx, pKey );
            break;
        }
        else
        {
            // insert to child node
            CBPlusNode* pChild =
                this->GetChild( dwIdx );
            if( unlikely( pChild == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            if( pChild->GetKeyCount() ==
                MAX_KEYS_PER_NODE )
            {
                ret = pChild->Split( this, dwIdx );
                if( ERROR( ret ) )
                    break;
                CBPlusNode* pNewIns =
                    this->GetChild( dwIdx + 1 );
                gint32 iRet = strncmp( pKey->szKey,
                    this->GetKey( dwIdx ),
                    REGFS_NAME_LENGTH - 1 );
                if( iRet >= 0 )
                    pChild = pNewIns;
            }
            ret = pChild->InsertNonFull( pKey );
        }
    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Split(
    CBPlusNode* pParent, guint32 dwSlotIdx )
{
    gint32 ret = 0;
    do{
        BNodeUPtr pNewSibling;
        ret = m_pDir->GetFreeBNode(
            this->IsLeaf(), pNewSibling );
        if( ERROR( ret ) )
            break;

        // key to add to the parent slot.
        KEYPTR_SLOT oAscendKey;

        // pos to split
        guint32 dwSplitPos = SPLIT_POS(
            MAX_PTRS_PER_NODE, IsLeaf() ) ; 

        KEYPTR_SLOT* pSplitKs =
            this->GetSlot( dwSplitPos );

        guint32 dwRightMost =
            pSplitKs->dwBNodeIdx;

        guint32 dwChilds =
            pParent->GetChildCount();

        if( dwSlotIdx < dwChilds )
        {
            KEYPTR_SLOT* pParentKs = 
                pParent->GetSlot( dwSlotIdx );

            strncpy( &oAscendKey.szKey,
                pParentKs->szKey,
                sizeof( oAscendKey.szKey ) );

            strncpy( pParentKs->szKey,
                pSplitKs->szKey, 
                sizeof( oAscendKey.szKey ) );
        }
        else if( dwChilds == 0 )
        {
            KEYPTR_SLOT* pParentKs = 
                pParent->GetSlot( 0 );

            dwSlotIdx = 0;
            // init the slot[0] of the parent
            strncpy( pParentKs->szKey,
                pSplitKs->szKey, 
                sizeof( pSplitKs->szKey ) );
            pParent->IncKeyCount();
            pParentKs->dwBNodeIdx =
                this->GetBNodeIndex();
            pParent->AddChildDirect(
                this->GetBNodeIndex(), this );
            pParent->IncChildCount();
            oAscendKey.szKey[ 0 ] = 0;
        }
        else
        {
            ret = ERROR_FAIL;
            DebugPrint( ret, "Unknown situation" );
            break;
        }

        oAscendKey.dwBNodeIdx =
            pNewSibling->GetBNodeIndex();

        pSplitKs->szKey[ 0 ] = 0;

        CBPlusNode* pNew = pNewSibling;
        ret = pParent->AddChildDirect(
            oAscendKey.dwBNodeIdx, pNewSibling );
        if( ERROR( ret ) )
            break;

        ret = pParent->InsertSlotAt(
            dwSlotIdx + 1, &oAscendKey );
        if( ERROR( ret ) )
            break;

        guint32 dwMoveStart;
        if( !IsLeaf() )
            dwMoveStart = dwSplitPos + 1;
        else
            dwMoveStart = dwSplitPos;

        guint32 dwCount = 
            MAX_PTRS_PER_NODE - dwMoveStart;

        ret = pNew->MoveBNodeStore(
            this, dwMoveStart, dwCount,  0 );
        if( ERROR( ret ) )
            break;

        if( IsLeaf() )
        {
            pNew->SetNextLeaf(
                this->GetNextLeaf() );
            this->SetNextLeaf(
                pNew->GetBNodeIndex() );
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

        m_oInodeStore.m_dwUserData = 0;
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
            new CBPlusNode( this,
            m_oInodeStore.m_dwUserData ) );
        ret = m_pRootNode->Reload();
        if( ERROR( ret ) )
            break;
        m_pFreePool->Reload();
    }while( 0 );
    return ret;
}

gint32 CBPlusNode::StealFromRight( gint32 i )
{
    gint32 ret = 0;
    do{
        if( i == 0 || i >= MAX_PTRS_PER_NODE )
        {
            ret = -EINVAL;
            break;
        }
        CBPlusNode* pLeft = GetChild( i );
        if( pLeft == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        CBPlusNode* pRight = GetChild( i + 1 )
        if( pRight == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        KEYPTR_SLOT* pParentKs = GetSlot( i );
        KEYPTR_SLOT* pRightKs = pRight->GetSlot( 0 );

        KEYPTR_SLOT oSteal;
        strncpy( oSteal.szKey, pParentKs->szKey,
            REGFS_NAME_LENGTH - 1 );
        strncpy( pParentKs->szKey, pRightKs->szKey,
            REGFS_NAME_LENGTH - 1 );

        if( pRight->IsLeaf() )
        {
            FImgPtr pFile;
            oSteal.oLeaf = pRightKs->oLeaf;
            ret = pRight->RemoveFileDirect(
                oSteal.oLeaf.dwInodeIdx, pFile );
            pRight->DecKeyCount();
            pLeft->AppendSlot( oSteal );
            pLeft->AddFileDirect(
                oSteal.oLeaf.dwInodeIdx, pFile );
            pLeft->IncKeyCount();
        }
        else
        {
            BNodeUPtr pNode;
            oSteal.dwBNodeIdx = pRightKs->dwBNodeIdx;
            ret = pRight->RemoveChildDirect(
                oSteal.dwBNodeIdx, pNode );
            if( ERROR( ret ) )
            {
                DebugPrint( ret, "Error, steal failed "
                    "with empty child pointer" );
                break;
            }
            KEYPTR_SLOT oKey;
            ret = pRight->RemoveSlotAt( 0, oKey );
            if( ERROR( ret ) )
                break;
            pLeft->AddChildDirect(
                oSteal.dwBNodeIdx, pNode );
            pLeft->AppendSlot( oSteal );
        }

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::BinSearch(
    const char* szKey,
    gint32 iOrigLower,
    gint32 iOrigUpper )
{
    gint32 ret = 0;

    gint32 iLower = iOrigLower;
    gint32 iUpper = iOrigUpper;
    
    do{
        if( iUpper == iLower )
        {
            gint32 iRet = strncmp( szKey,
                this->GetKey( iLower ),
                REGFS_NAME_LENGTH - 1 );
            if( iRet < 0 )
            {
                ret = -( iLower + 0x10000 );
                break;
            }
            else if( iRet > 0 )
            {
                ret = -( iLower + 1 );
                break;
            }
            ret = iLower;
            break;
        }
        else if( iUpper == iLower + 1 )
        {
            gint32 iRet = strncmp( szKey,
                this->GetKey( iLower ),
                REGFS_NAME_LENGTH - 1 );
            if( iRet < 0 )
            {
                ret = -( iLower + 0x10000 );
                break;
            }
            if( iRet == 0 )
            {
                ret = iLower;
                break;
            }
            iRet = strncmp( szKey,
                this->GetKey( iUpper ),
                REGFS_NAME_LENGTH - 1 );
            if( iRet < 0 )
            {
                ret = -( iUpper + 10000 );
                break;
            }
            else if( iRet == 0 )
            {
                ret = iUpper;
                break;
            }
            else
            {
                ret = -( iUpper + 1 );
            }
            break;
        }
        gint32 iCur =
            ( iLower + iUpper ) >> 1;
        iRet = strncmp( szKey,
            this->GetKey( iLower ),
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
            gint32 i = pCurNode->BinSearch(
                szKey, 0, dwCount - 1 );
            if( i >= 0 )
            {
                ret = true;
                break;
            }
            i = -i;
            if( i > 0x10000 )
                i -= 0x10000;
            pCurNode = pCurNode->GetChild( i );
        }

        dwCount = pCurNode->GetKeyCount();
        gint32 i = pCurNode->BinSearch(
            szKey, 0, dwCount - 1 );
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
        {
            BNodeUPtr pNewRoot;
            ret = GetFreeBNode( false, pNewRoot );
            if( ERROR( ret ) )
                break;
            ret = pCurNode->Split(
                pNewRoot, MAX_PTRS_PER_NODE );
            if( ERROR( ret ) )
                break;
            pNewRoot->InsertNonFull( pSlot );
            m_pRootNode = std::move( pNewRoot );
            m_oInodeStore.m_dwUserData =
                m_pRootNode->GetBNodeIndex();
        }
        else
        {
            pCurNode->InsertNonFull( pSlot );
        }
    }while( 0 );
    return ret;
}

gint32 CDirImage::PutFreeBNode(
    CBPlusNode* pNode )
{
    guint32 dwBNodeIdx =
        pNode->GetBNodeIndex();

    return m_pFreePool->PutFreeBNode(
            dwBNodeIdx );
}

gint32 CDirImage::GetFreeBNode(
    bool bLeaf, BNodeUPtr& pNewNode )
{
    gint32 ret = 0;
    do{
        guint32 dwBNodeIdx = INVALID_BNODE_IDX;
        ret = m_pFreePool->GetFreeBNode(
            dwBNodeIdx );
        if( ERROR( ret ) )
        {
            dwBNodeIdx =
                ( m_oInodeStore.m_dwSize +
                    REGFS_PAGE_SIZE - 1 ) /
                    REGFS_PAGE_SIZE;
        }
        pNewNode.reset( new CBPlusNode(
            this, dwBNodeIdx ) );
        pNewNode->SetLeaf( bLeaf );
        ret = pNewNode->Format();
        if( ERROR( ret ) )
            break;

    }while( 0 );
    return ret;
}

}
