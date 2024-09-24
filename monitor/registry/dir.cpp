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

gint32 CBPlusLeaf::Format()
{
    gint32 ret = 0;
    do{
        auto pBPNode = &m_oBNodeStore;
        pBPNode->m_bLeaf = true;
        ret = m_pFile->WriteFile(
            BNODE_IDX_TO_POS( m_dwBNodeIdx ),
            pBPNode, sizeof( *pBPNode ) );
        if( ERROR( ret ) )
            break;
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
        m_oInodeStore.m_wMode = S_IFDIR;
        m_pRootNode.reset( new CBPlusLeaf(
            m_pAlloc, 0 );
        ret = m_pRootInode->Format();
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

    }while( 0 );
    return ret;
}

}
