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

#define MIN_PTRS( _root_ ) \
( (_root) ? 2 : ( ( MAX_PTRS_PER_NODE + 1 ) / 2 ) )

#define MIN_KEYS( _root_ ) ( (_root) ? 1 : \
    ( ( MAX_PTRS_PER_NODE + 1 ) / 2 - 1  ) )

#define SPLIT_POS( _leaf_ ) \
( ( (_leaf_) ? ( MAX_PTRS_PER_NODE/2 ) : \
    ( ( MAX_PTRS_PER_NODE + 1 ) / 2 - 1 ) ) - 1 )

#define COMPARE_KEY( _key1_, _key2_ ) \
    ( strncmp( (_key1_), (_key2_), REGFS_NAME_LENGTH - 1 ) )

#define COPY_KEY( _dst_, _src_ ) \
do{ strncpy( (_dst_), (_src_), REGFS_PAGE_SIZE - 1 ); \
    (_dst_)[ REGFS_PAGE_SIZE - 1 ] = 0; \
}while( 0 )

#define CLEAR_KEY( _key_ ) \
( (_key_)[ 0 ] = 0, (_key_)[ REGFS_NAME_LENGTH - 1 ] = 0 )

#define FILE_TYPE( _dwMode_ ) \
( ( (_dwMode_) & S_IFMT ) )

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
    CLEAR_KEY( p->szKey );
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
            auto& oMap = GetChildMap();
            auto& pNode = oMap[ dwIdx ];
            pNode.reset(
                new CBPlusNode( m_pDir, dwIdx ) );
            ret = pNode->Reload();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Flush( guint32 dwFlags )
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

        guint32 dwCount = this->GetChildCount();
        for( gint32 i = 0; i < dwCount; i++ ) 
        {
            CBPlusNode* pChild =
                this->GetChild( i );
            ret = pChild->Flush( dwFlags );
            if( ERROR( ret ) )
            {
                DebugPrint( ret, "Error during "
                    "BNode flushing" );
                break;
            }
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
        for( ; i + 1 <= dwCount; i++ )
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
            COPY_KEY( vec[ dwCount ]->szKey,
                pKey->szKey );
            oNewLast.dwBNodeIdx = pKey->dwBNodeIdx;
            CLEAR_KEY( oNewLast.szKey );
            memcpy( vec[ dwCount + 1 ],
                &oNewLast, sizeof( KEYPTR_SLOT ) );
            this->IncChildCount();
        }
        this->IncKeyCount();

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
            auto& oMap = GetChildMap();
            auto itr = oMap.find( dwIdx );
            if( itr->get() == nullptr )
            {
                auto& pNode = oMap[ dwIdx ];
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
    if( dwSrcOff + dwCount >= MAX_PTRS_PER_NODE ||
        dwDstOff + dwCount >= MAX_PTRS_PER_NODE )
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
        if( dwIdx >= 0x10000 )
            dwIdx -= 0x10000;

        if( dwIdx >= MAX_KEYS_PER_NODE )
        {
            ret = ERROR_FAIL;
            break;
        }

        ret = InsertSlotAt( dwIdx, pKey );
        break;
    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Split(
    CBPlusNode* pParent, guint32 dwSlotIdx )
{
    gint32 ret = 0;
    do{
        if( IsRoot() && IsLeaf() )
        {
            ret = ERROR_FAIL;
            DebugPrint( ret, "Error, we "
                "should not split a root leaf "
                "in Split" );
            break;
        }

        if( IsRoot() )
        {
            // non-leaf root, replace it with a new root.
            BNodeUPtr pNewRoot;
            ret = GetFreeBNode( false, pNewRoot );
            if( ERROR( ret ) )
                break;
            BNodeUPtr pOld;
            m_pDir->ReplaceRootBNode(
                pNewRoot, pOld );
            auto& oMap = this->GetChildMap();
            ret = pNewRoot->AddChildDirect(
                pOld->GetBNodeIndex(), pOld );
            if( ERROR( ret ) )
                break;
            pParent = pNewRoot;
            dwSlotIdx = 0;
        }

        guint32 dwChilds =
            pParent->GetChildCount();

        if( dwChilds == MAX_PTRS_PER_NODE )
        {
            // split the parent before insertion
            CBPlusNode* pGrandP =
                pParent->GetParent();
            guint32 dwParentSlot;
            if( pGrandP == nullptr )
                dwParentSlot = 0;
            else
            {
                guint32 dwBNodeIdx =
                    pParent->GetBNodeIndex();
                guint32 i =
                    pParent->GetParentSlotIdx();
                if( i == UINT_MAX )
                {
                    ret = -ENOENT;
                    break;
                }
                dwParentSlot = i;
            }
            ret = pParent->Split(
                pGrandP, dwParentSlot );
            if( ERROR( ret ) )
                break;

            pParent = this->GetParent();
            dwSlotIdx = GetParentSlotIdx()
            dwChilds =
                pParent->GetChildCount();
        }

        KEYPTR_SLOT* pSplitKs =
            this->GetSlot( dwSplitPos );

        // key to add to the parent slot.
        KEYPTR_SLOT oAscendKey;

        if( dwChilds == 0 )
        {
            KEYPTR_SLOT* pParentKs = 
                pParent->GetSlot( 0 );

            dwSlotIdx = 0;
            // init the slot[0] of the parent
            COPY_KEY( pParentKs->szKey,
                pSplitKs->szKey );
            pParent->IncKeyCount();
            pParentKs->dwBNodeIdx =
                this->GetBNodeIndex();
            pParent->AddChildDirect(
                this->GetBNodeIndex(), this );
            pParent->IncChildCount();
            CLEAR_KEY( oAscendKey.szKey );
        }
        else if( dwSlotIdx < dwChilds - 1 )
        {
            KEYPTR_SLOT* pParentKs = 
                pParent->GetSlot( dwSlotIdx );
            COPY_KEY( oAscendKey.szKey,
                pParentKs->szKey )
            COPY_KEY( pParentKs->szKey,
                pSplitKs->szKey );
        }
        else if( dwSlotIdx + 1 == dwChilds )
        {
            KEYPTR_SLOT* pParentKs = 
                pParent->GetSlot( dwSlotIdx );
            CLEAR_KEY( oAscendKey.szKey );
            COPY_KEY( pParentKs->szKey,
                pSplitKs->szKey );
        }
        else
        {
            ret = ERROR_FAIL;
            DebugPrint( ret, "Inserting beyond "
                "the max slots at %d",
                dwSlotIdx );
            break;
        }

        BNodeUPtr pNewSibling;
        ret = m_pDir->GetFreeBNode(
            this->IsLeaf(), pNewSibling );
        if( ERROR( ret ) )
            break;

        // pos to split
        guint32 dwSplitPos =
            SPLIT_POS( IsLeaf() ) ; 

        guint32 dwMoveStart = dwSplitPos;
        guint32 dwCount = 
            MAX_PTRS_PER_NODE - dwMoveStart;

        ret = pNewSibling->MoveBNodeStore(
            this, dwMoveStart, dwCount,  0 );
        if( ERROR( ret ) )
            break;

        if( IsLeaf() )
        {
            pNewSibling->SetNextLeaf(
                this->GetNextLeaf() );
            this->SetNextLeaf(
                pNewSibling->GetBNodeIndex() );
        }

        oAscendKey.dwBNodeIdx =
            pNewSibling->GetBNodeIndex();

        CLEAR_KEY( pSplitKs->szKey );

        CBPlusNode* pNew = pNewSibling;
        ret = pParent->AddChildDirect(
            oAscendKey.dwBNodeIdx, pNewSibling );
        if( ERROR( ret ) )
            break;

        ret = pParent->InsertSlotAt(
            dwSlotIdx + 1, &oAscendKey );
        if( ERROR( ret ) )
            break;
    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Insert( KEYPTR_SLOT* pSlot )
{
    gint32 ret = 0;
    if( pSlot == nullptr )
        return -EINVAL;
    do{
        guint32 dwCount = this->GetKeyCount();
        bool bFull =
            ( dwCount == MAX_KEYS_PER_NODE );

        CBPlusNode* pParent = nullptr;
        CBPlusNode* pChild = nullptr;
        if( !IsLeaf() )
        {
            ret = BinSearch(
                pSlot->szKey, 0, dwCount );
            if( ret < 0 )
            {
                 ret = -ret;
                 if( ret >= 0x10000 )
                     ret -= 0x10000;
            }
            pChild = this->GetChild( ret );
            pChild->Insert( pSlot );
            break;
        }
        else if( !bFull )
        {
            ret = this->InsertNonFull( pSlot );
            break;
        }
        else if( IsRoot() )
        {
            BNodeUPtr pNewRoot;
            ret = GetFreeBNode( false, pNewRoot );
            if( ERROR( ret ) )
                break;
            BNodeUPtr pOld;
            m_pDir->ReplaceRootBNode(
                pNewRoot, pOld );
            auto& oMap = this->GetChildMap();
            ret = pNewRoot->AddChildDirect(
                pOld->GetBNodeIndex(), pOld );
            if( ERROR( ret ) )
                break;
            ret = this->Split(
                pNewRoot, MAX_PTRS_PER_NODE );
            if( ERROR( ret ) )
                break;
            KEYPTR_SLOT* pKey =
                pNewRoot->GetSlot( 0 );

            gint32 iRet = COMPARE_KEY(
                pSlot->szKey, pKey->szKey )
            if( iRet < 0 )
            {
                ret = this->InsertNonFull(
                    pSlot );
            }
            else if( iRet > 0 )
            {
                CBPlusNode* pSibling =
                    pNewRoot->GetChild( 1 );
                ret = pSibling->InsertNonFull(
                    pSlot );
            }
            else
            {
                ret = -EEXIST;
            }
            break;
        }
        else
        {
            pParent = GetParent();
            if( pParent == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = this->Split(
                pParent, MAX_PTRS_PER_NODE );
            if( ERROR( ret ) )
                break;

            guint32 i;
            guint32 dwCount =
                pParent->GetKeyCount();

            // for( i = 0; i < dwCount; i++ )
            i = GetParentSlotIdx();
            if( i == UINT_MAX )
            {
                ret = -ENOENT;
                break;
            }
            CBPlusNode* pSibling =
                pParent->GetChild( i + 1 );
            KEYPTR_SLOT* pChildKs =
                pParent->GetSlot( i );
            gint32 iRet = COMPARE_KEY(
                pSlot->szKey, pChildKs->szKey )
            if( iRet == 0 )
            {
                ret = -EEXIST;
                break;
            }
            if( iRet > 0 )
            {
                ret = pSibling->InsertNonFull(
                    pSlot );
            }
            else
            {
                ret = this->InsertNonFull(
                    bSlot );
            }
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

gint32 CFreeBNodePool::Flush( guint32 dwFlags )
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

gint32 CDirImage::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        ret = super::Flush( dwFlags );
        if( ERROR( ret ) )
            break;
        ret = m_pRootNode->Flush( dwFlags );
        if( ERROR( ret ) )
            break;
        ret = m_pFreePool->Flush( dwFlags );
        if( ERROR( ret ) )
            break;
        if( dwFlags & FLAG_FLUSH_CHILD )
        {
            for( auto& elem : m_mapFiles )
            {
                ret = elem->Flush( dwFlags );
                if( ERROR( ret ) )
                    break;
            }
        }
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
        if( i < 0 || i >= MAX_PTRS_PER_NODE - 1 )
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
        KEYPTR_SLOT* pRightKs =
            pRight->GetSlot( 0 );

        KEYPTR_SLOT* pRightKs2 =
            pRight->GetSlot( 1 );

        KEYPTR_SLOT oSteal;
        COPY_KEY( oSteal.szKey,
            pParentKs->szKey );

        if( pRight->IsLeaf() )
        {
            COPY_KEY( pParentKs->szKey,
                pRightKs2->szKey );

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
            COPY_KEY( pParentKs->szKey,
                pRightKs->szKey );
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

gint32 CBPlusNode::StealFromLeft( gint32 i )
{
    gint32 ret = 0;
    do{
        if( i <= 0 || i >= MAX_PTRS_PER_NODE )
        {
            ret = -EINVAL;
            break;
        }
        CBPlusNode* pLeft = GetChild( i - 1 );
        if( pLeft == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        CBPlusNode* pRight = GetChild( i )
        if( pRight == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        KEYPTR_SLOT* pParentKs = GetSlot( i );
        KEYPTR_SLOT* pLeftKs = pLeft->GetSlot(
            pLeft->GetKeyCount() - 1 );

        KEYPTR_SLOT oSteal;

        if( pLeft->IsLeaf() )
        {
            COPY_KEY( oSteal.szKey,
                pLeftKs->szKey );

            COPY_KEY( pParentKs->szKey,
                pLeftKs->szKey );

            FImgPtr pFile;
            oSteal.oLeaf = pLeftKs->oLeaf;
            ret = pLeft->RemoveFileDirect(
                oSteal.oLeaf.dwInodeIdx, pFile );
            pLeft->DecKeyCount();
            pRight->InsertSlotAt( 0, oSteal );
            pRight->AddFileDirect(
                oSteal.oLeaf.dwInodeIdx, pFile );
            pRight->IncKeyCount();
        }
        else
        {
            KEYPTR_SLOT* pLeftKs0 = pLeft->GetSlot(
                pLeft->GetKeyCount() - 0 );

            COPY_KEY( oSteal.szKey,
                pParentKs->szKey );

            COPY_KEY( pParentKs->szKey,
                pLeftKs->szKey );

            BNodeUPtr pNode;
            oSteal.dwBNodeIdx = pLeftKs0->dwBNodeIdx;
            ret = pLeft->RemoveChildDirect(
                oSteal.dwBNodeIdx, pNode );
            if( ERROR( ret ) )
            {
                DebugPrint( ret, "Error, steal failed "
                    "with empty child pointer" );
                break;
            }

            CLEAR_KEY( pLeftKs->szKey );
            KEYPTR_SLOT oKey;
            pLeft->RemoveSlotAt(
                pLeft->GetKeyCount(), oKey );
            pRight->AddChildDirect(
                oSteal.dwBNodeIdx, pNode );
            pRight->InsertSlotAt( 0, oSteal );
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
            gint32 iRet = COMPARE_KEY( szKey,
                this->GetKey( iLower ) );
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
            ret = iLower + 1;
            break;
        }
        else if( iUpper == iLower + 1 )
        {
            gint32 iRet = COMPARE_KEY( szKey,
                this->GetKey( iLower ) );
            if( iRet < 0 )
            {
                ret = -( iLower + 0x10000 );
                break;
            }
            if( iRet == 0 )
            {
                ret = iLower + 1;
                break;
            }
            iRet = COMPARE_KEY( szKey,
                this->GetKey( iUpper ) )
            if( iRet < 0 )
            {
                ret = -( iUpper + 10000 );
                break;
            }
            else if( iRet == 0 )
            {
                ret = iUpper + 1;
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
        iRet = COMPARE_KEY( szKey,
            this->GetKey( iLower ) );
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

const char* CBPlusNode::GetSuccKey(
    guint32 dwIdx ) const
{
    CBPlusNode* pChild = this;
    while( pChild && !pChild->IsLeaf() )
        pChild = pChild->GetChild( 0 );
    if( unlikely( pChild == nullptr ) )
    {
        DebugPrint( -EFAULT,
            "Error internal error" );
        return nullptr;
    }
    guint32 dwCount = pChild->GetKeyCount();
    while( dwCount <= dwIdx )
    {
        dwIdx -= dwCount;
        guint32 dwNextLeaf =
            pChild->GetNextLeaf();
        if( dwNextLeaf == INVALID_BNODE_IDX )
            return nullptr;
        auto& oMap = GetChildMap();
        auto itr = oMap.find( dwNextLeaf );
        if( itr == oMap.end() )
            return nullptr;
        pChild = itr->second.get();
        dwCount = pChild->GetKeyCount();
    }
    return pChild->GetKey( dwIdx );
}

const char* CBPlusNode::GetPredKey(
    guint32 dwIdx ) const
{
    CBPlusNode* pChild = this;
    while (pChild && !pChild->IsLeaf())
    {
        pChild = pChild->GetChild(
            pChild->GetChildCount() - 1 );
    }
    if( unlikely( pChild == nullptr ) )
    {
        DebugPrint( -EFAULT,
            "Error internal error" );
    }
    if( pChild->GetKeyCount() <= dwIdx )
        return nullptr;
    return pChild->GetKey(
        pChild->GetKeyCount() - 1 - dwIdx );
}

gint32 CBPlusNode::MergeChilds(
    gint32 iPred, gint32 iSucc )
{
    gint32 ret = 0;
    if( iPred < 0 ||
        iPred >= MAX_PTRS_PER_NODE - 1 ||
        iSucc <= iPred ||
        iSucc >= MAX_PTRS_PER_NODE )
        return -EINVAL;
    do{
        CBPlusNode* pPred =
            this->GetChild( iPred );
        CBPlusNode* pSucc =
            this->GetChild( iSucc );
        guint32 dwCount =
            pPred->GetKeyCount() +
            pSucc->GetKeyCount();
        if( dwCount > MAX_KEYS_PER_NODE )
        {
            ret = -EOVERFLOW;
            break;
        }
        if( pPred->IsLeaf() )
        {
            ret = pPred->MoveBNodeStore( pSucc,
                0, pSucc->GetKeyCount() + 1,
                pPred->GetKeyCount() );
            if( ERROR( ret ) )
                break;
            pPred->SetNextLeaf(
                pSucc->GetNextLeaf() );
        }
        else
        {
            KEYPTR_SLOT* pPredKs =
                this->GetSlot( iPred );    
            KEYPTR_SLOT* pLast = pPred->GetSlot(
                this->GetChildCount() );
            COPY_KEY(
                pLast->szKey, pPredKs->szKey );
            pPred->IncKeyCount();
            ret = pPred->MoveBNodeStore( pSucc,
                0, pSucc->GetKeyCount() + 1,
                pPred->GetKeyCount() );
            if( ERROR( ret ) )
                break;
        }

        KEYPTR_SLOT oSuccKs;
        this->RemoveSlotAt( iSucc, oSuccKs );
        if( iSucc == this->GetKeyCount() )
        {
            KEYPTR_SLOT* pPredKs =
                this->GetSlot( iPred );    
            CLEAR_KEY( pPredKs->szKey );
        }
        BNodeUPtr pNode;
        this->RemoveChildDirect(
            iSucc->dwBNodeIdx, pNode );
        ret = m_pDir->PutFreeBNode( pNode );
        if( ERROR( ret ) )
            break;
        if( this->GetKeyCount() >= 
            MIN_KEYS( IsRoot() ) ) 
            break;
        ret = Rebalance();

    }while( 0 );
    return ret;
}

gint32 CBPlusNode::Rebalance()
{
    gint32 ret = 0;
    BNodeUPtr pRoot;
    do{
        if( IsRoot() && IsLeaf() )
        {
            // the last one is removed
            ret = m_pDir->FreeRootBNode( pRoot );
            break;
        }
        else if( IsRoot() )
        {
            // replace the root node
            if( this->GetKeyCount() != 0 )
            {
                ret = ERROR_FAIL;
                DebugPrint( ret, "Error, not "
                    "an expected state to "
                    "rebalance the root node" );
                break;
            }
            BNodeUPtr pNewRoot =
                this->RemoveChild( 0, pNewRoot );
            m_pDir->ReplaceRootBNode(
                pNewRoot, pOld );
            ret = m_pDir->PutFreeBNode( pOld );
            break;
        }

        guint32 i = GetParentSlotIdx();
        guint32 dwCount =
            pParent->GetChildCount();
        if( i == UINT_MAX )
        {
            ret = -ENOENT;
            DebugPrint( ret, "Error "
                "failed to find child node in "
                "parent's slot table" );
            break;
        }
        ret = pParent->RebalanceChild( i );

    }while( 0 );
    return ret;
}


gint32 CBPlusNode::RebalanceChild(
    guint32 idx )
{
    gint32 ret = 0;
    do{
        KEYPTR_SLOT* pChildKs =
            this->GetSlot( idx );

        CBPlusNode* pChild =
            this->GetChild( idx );

        guint32 dwLimit = MIN_KEYS( false );
        guint32 dwCount = GetKeyCount();
        if( idx == 0 && dwCount == 1 )
        {
            ret = ERROR_FAIL;
            DebugPrint( ret, "Error we should "
                "not be here" );
        }
        if( idx >= 0 && idx < dwCount )
        {
            CBPlusNode* pRight =
                this->GetChild( idx + 1 );
            if( pRight != nullptr &&
                pRight->GetKeyCount() >= dwLimit )
            {
                ret = this->StealFromRight(
                    idx + 1 );
                break;
            }
            ret = MergeChilds( idx, idx + 1 );
            break;
        }

        if( idx != dwCount )
        {
            ret = -ERANGE;
            break;
        }

        CBPlusNode* pLeft =
            this->GetChild( idx - 1 );
        if( pLeft != nullptr &&
            pLeft->GetKeyCount() >= dwLimit )
        {
            ret = this->StealFromLeft( idx + 1 );
            break;
        }
        ret = MergeChilds( idx - 1, idx );
        
    }while( 0 );
    return ret;
}

gint32 CBPlusNode::RemoveFile(
    const char* szKey, FImgSPtr& pFile )
{
    gint32 ret = 0;
    if( szKey == nullptr || szKey[ 0 ] == 0 )
        return -EINVAL;
    do{
        gint32 idx = BinSearch( szKey,
            0, this->GetKeyCount() - 1 );
        if( idx >= 0 )
        {
            if( IsLeaf() )
            {
                KEYPTR_SLOT* pKey = GetSlot( idx );
                if( ERROR( ret ) )
                    break;

                guint32& dwIondeIdx =
                    pSlot->oLeaf.dwInodeIdx;

                ret = this->GetFileDirect(
                    dwInodeIdx, pFile );
                if( ERROR( ret ) )
                {
                    // create one for deletion
                    CFileImage::Create(
                        pFile, m_pAlloc, dwInodeIdx,
                        this->GetInodeIdx() );
                    ret = pFile->Reload();
                    if( ERROR( ret ) )
                        break;
                }

                if( pFile->GetOpenCount() )
                {
                    DebugPrint( ret, "Error, "
                        "the file is still "
                        "being used" );
                    ret = -EBUSY;
                    break;
                }
                else if( pFile->GetState() ==
                    stateStopped )
                {
                    ret = -ENOENT;
                    DebugPrint( ret, "Error, "
                        "the file is being "
                        "removed" );
                    break;
                }

                CDirImage* pSubDir = pFile;
                while( pSubDir != nullptr )
                {
                    READLOCK( pSubDir );
                    ret = pSubDir->GetRootKeyCount();
                    if( ret > 0 )
                    {
                        ret = -ENOTEMPTY;
                        DebugPrint( ret, "Error, "
                            "the directory is not "
                            "empty to remove" );
                    }
                    break;
                }

                this->RemoveFileDirect(
                    dwInodeIdx, pFile );

                KEYPTR_SLOT oKey;
                this->RemoveSlotAt( idx, oKey );

                if( GetKeyCount() >= 
                    MIN_KEYS( IsRoot() ) )
                    break;
                Rebalance();
                break;
            }
            KEYPTR_SLOT* pChildKs =
                this->GetSlot( idx );
            CBPlusNode* pChildSuc =
                this->GetChild( idx + 1 );

            guint32 dwLimit = MIN_KEYS( false );
            auto p = pChildSuc->GetSuccKey(1);

            // replace with the key of its successor
            COPY_KEY( pChildKs->szKey, p );
            ret = pChildSuc->RemoveFile(
                szKey, pFile );
            break;
        }
        idx = -idx;
        if( idx >= 0x10000 )
            idx -= 0x10000;

        if( IsLeaf() )
        {
            ret = -ENOENT;
            DebugPrint( ret,
                "Error '%s' not found" );
            break;
        }

        CBPlusNode* pChild =
            this->GetChild( idx );

        ret = pChild->RemoveFile(
            szKey, pFile );

    }while( 0 );
    return ret;
}

bool CDirImage::Search( const char* szKey,
    FImgSPtr& pFile, CBPlusNode*& pNode )
{
    bool bRet = false;
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        guint32 dwCount = 0;
        pNode = nullptr;
        CBPlusNode* pCurNode = m_pRootNode->get(); 
        while( !pCurNode->IsLeaf() )
        {
            dwCount = pCurNode->GetKeyCount();
            gint32 i = pCurNode->BinSearch(
                szKey, 0, dwCount - 1 );
            if( i >= 0 )
            {
                bRet = true;
                pCurNode = pCurNode->GetChild( i );
                continue;
            }
            i = -i;
            if( i >= 0x10000 )
                i -= 0x10000;
            pCurNode = pCurNode->GetChild( i );
        }

        pNode = pCurNode;
        if( bRet )
        {
            CStdRMutex oExclLock( GetExclLock() );
            // found in non-leaf node
            KEYPTR_SLOT* pks =
                pNode->GetSlot( 0 );
            dwInodeIdx = pks->oLeaf.dwInodeIdx;
            auto& oMap = GetFileMap();
            auto itr = oMap.find( dwInodeIdx );
            if( itr != oMap.end )
                pFile = itr->second;
            else
            {
                // load the file if not yet
                EnumFileType byType =
                    pks->oLeaf.byFileType;
                ret = CFileImage::Create( byType,
                    pFile, m_pAlloc, dwInodeIdx,
                    this->GetInodeIdx() );
                if( ERROR( ret ) )
                    break;
                ret = pFile->Reload();
                if( ERROR( ret ) )
                    break;
                pNode->AddChildDirect(
                    dwInodeIdx,  pFile );
            }
            break;
        }

        // search through the leaf node
        dwCount = pCurNode->GetKeyCount();
        gint32 i = pCurNode->BinSearch(
            szKey, 0, dwCount - 1 );
        if( i >= 0 )
        {
            CStdRMutex oExclLock( GetExclLock() );
            KEYPTR_SLOT* pks =
                pNode->GetSlot( i );
            dwInodeIdx =
                pks->oLeaf.dwInodeIdx;
            bRet = true;
            auto& oMap = GetFileMap();
            auto itr = oMap.find( dwInodeIdx );
            if( itr != oMap.end )
                pFile = itr->second;
            else
            {
                EnumFileType byType =
                    pks->oLeaf.byFileType;
                ret = CFileImage::Create( byType,
                    pFile, m_pAlloc, dwInodeIdx,
                    this->GetInodeIdx() );
                if( ERROR( ret ) )
                    break;
                ret = pFile->Reload();
                if( ERROR( ret ) )
                    break;
                ret = pNode->AddChildDirect(
                    dwInodeIdx,  pFile );
            }
        }
    }while( 0 );

    if( ERROR( ret ) )
        DebugPrint( ret,
            "Error occurs during Search" );
    return bRet;
}

gint32 CDirImage::ReplaceRootBNode(
    BNodeUPtr& pNew, BNodeUPtr& pOld )
{
    pOld = std::move( m_pRootNode );
    m_pRootNode = std::move( pNew );
    m_oInodeStore.m_dwUserData =
        pNew->GetBNodeIndex();
    m_pRootNode->SetFreeBNodeIdx(
        pOld->GetFreeBNodeIdx() );
    return 0;
}

gint32 CDirImage::FreeRootBNode(
    BNodeUPtr& pRoot )
{
    gint32 ret = 0;
    do{
        pRoot.reset(
            std::move( m_pRootNode ) );
        PutFreeBNode( pRoot );
        m_pRootNode.reset();
        this->Truncate( 0 );

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
        this->Flush();

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

gint32 CDirImage::CreateFile(
    const char* szName,
    EnumFileType iType,
    FImgSPtr& pImg )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        CBPlusNode* pNode = nullptr;
        bool bRet = this->Search(
            szName, pImg, pNode );
        if( bRet )
        {
            ret = -EEXIST;
            break;
        }
        guint32 dwInodeIdx = 0;
        gint32 ret = m_pAlloc->AllocBlocks(
            &dwInodeIdx, 1 );
        if( ERROR( ret ) )
            break;
        ret = Create( iType,
            pImg, m_pAlloc, dwInodeIdx,
            this->GetInodeIdx() );
        if( ERROR( ret ) )
            break;

        pImg->Format();
        ret = pImg->Flush();
        if( ERROR( ret ) )
            break;
        KEYPTR_SLOT oKey;
        COPY_KEY( oKey.szKey, szName );
        oKey.oLeaf.byFileType = iType;
        oKey.oKey.dwInodeIdx = dwInodeIdx;
        ret = pNode->Insert( oKey );
        if( ERROR( ret ) )
            break;
        ret = pNode->AddFileDirect(
            dwInodeIdx, pImg );

    }while( 0 );
    return ret;
}


gint32 CDirImage::CreateFile(
    const char* szName,
    mode_t dwMode,
    FImgSPtr& pImg )
{
    gint32 ret = 0;
    do{
        ret = CheckAccess( W_OK | X_OK );
        if( ERROR( ret ) )
            break;

        ret = CreateFile(
            szName, ftRegular, pImg );

        SetMode( dwMode );
    }while( 0 );
    return ret;
}

gint32 CDirImage::ListDir(
    std::vector< KEYPTR_SLOT > vecDirEnt ) const
{
    bool bRet = false;
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        guint32 dwCount = 0;
        pNode = nullptr;
        CBPlusNode* pCurNode = m_pRootNode->get(); 
        while( !pCurNode->IsLeaf() )
            pCurNode = pCurNode->GetChild( 0 );

        while( pCurNode != nullptr )
        {
            guint32 i = 0;
            for( ;i < pCurNode->GetKeyCount(); i++ )
            {
                KEYPTR_SLOT* pKey =
                    pCurNode->GetSlot( i );
                vecDirEnt.push_back( *pKey );
            }
            guint32 dwBNodeIdx =
                pCurNode->GetNextLeaf();

            if( dwBNodeIdx == INVALID_BNODE_IDX )
                break;

            pCurNode = this->GetChildDirect(
                dwBNodeIdx );
        }

    }while( 0 );

    if( ERROR( ret ) )
        DebugPrint( ret,
            "Error occurs during Search" );
    return ret;
}

gint32 CDirImage::CreateDir( const char* szName,
    mode_t dwMode, FImgSPtr& pImg )
{
    gint32 ret = 0;
    do{
        CreateFile( szName, ftDirectory, pImg );
        SetMode( dwMode );
    }while( 0 );
    return ret;
}

gint32 CDirImage::CreateLink( const char* szName,
    const char* szLink, FImgSPtr& pImg )
{
    gint32 ret = 0;
    do{
        ret = CreateFile(
            szName, ftLink, pImg );

        if( ERROR( ret ) )
            break;

        gint32 dwSize = strlen( szLink );
        ret = pImg->WriteFile(
            0, dwSize, szLink );
    }while( 0 );
    return ret;
}

gint32 CDirImage::RemoveFile(
    const char* szKey )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        ret = m_pRootNode->RemoveFile(
            szKey, pFile );
        if( ERROR( ret ) )
            break;
        ret = pFile->Truncate( 0 );
        if( ERROR( ret ) )
            break;
        guint32 dwInodeIdx =
            pFile->GetInodeIdx();
        ret = m_pAlloc->FreeBlocks(
            &dwInodeIdx, 1 );
        pFile->SetState( stateStopped );
    }while( 0 );
    return ret;
}

gint32 CDirImage::Rename(
    const char* szFrom, const char* szTo)
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        ret = m_pRootNode->RemoveFile(
            szFrom, pFile );
        if( ERROR( ret ) )
            break;

        KEYPTR_SLOT oKey;
        COPY_KEY( oKey.szKey, szTo );
        oKey.oLeaf.dwInodeIdx =
            pFile->GetInodeIdx();
        guint32 dwMode = pFile->GetModeNoLock();
        if( FILE_TYPE( dwMode ) == S_IFREG )
            oKey.oLeaf.byFileType = ftRegular;
        else if( FILE_TYPE( dwMode ) == S_IFLNK )
            oKey.oLeaf.byFileType = ftLink;
        else if( FILE_TYPE( dwMode ) == S_IFDIR )
            oKey.oLeaf.byFileType = ftDirectory;
        ret = m_pRootNode->Insert( oKey );
        if( ERROR( ret ) )
            break;
        ret = m_pRootNode->AddFileDirect(
            oKey.oLeaf.dwInodeIdx, pFile );
    }while( 0 );
    return ret;
}

}
