/*
 * =====================================================================================
 *
 *       Filename:  blkalloc.h
 *
 *    Description:  declarations of classes for block allocation within a file 
 *
 *        Version:  1.0
 *        Created:  09/04/2024 01:19:42 PM
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
#include <arpa/inet.h>
#include <stdint.h>
// byte offset
// 0                             64
// -------------------------------
// bytes_in_blk|block_idx_grp|group_idx

// 
// ---layout---
// superblock 512b
// group bitmap 512b( 4096 groups )
// group0 starts
//     group0 block bitmap 8kb( 64k blocks )
//     group0 data blocks
//          root dir inode area block 
//          root dir file area
// group1 starts
//     group1 block bitmap 8kb( 64k blocks )
//     group1 data blocks
#define BYTE_BITS 8
#define find_shift( _x ) ( ffs( _x ) - 1 )

#define SUPER_BLOCK_SIZE 512

#define BLOCK_SHIFT 9

#define BLOCK_SIZE  ( 1 << BLOCK_SHIFT )

#define BLOCKS_PER_GROUP ( 64 * 1024 )

#define BLKBMP_BLKNUM \
    ( BLOCKS_PER_GROUP / ( BLOCK_SIZE * BYTE_BITS ) )

#define BLOCK_IDX_MASK \
    ( BLOCKS_PER_GROUP - 1 )

#define GROUP_SHIFT \
    ( find_shift( PAGE_SIZE ) )

#define BLKGRP_NUMBER \
    ( 1 << GROUP_SHIFT )

#define GRPBMP_BLKNUM \
    ( BLKGRP_NUMBER / ( BLOCK_SIZE * BYTE_BITS ) )

#define GRPBMP_SIZE \
    ( BLKGRP_NUMBER / BYTE_BITS )

#define GRPBMP_START \
    ( SUPER_BLOCK_SIZE )

#define GROUP_SIZE \
    ( BLOCKS_PER_GROUP * BLOCK_SIZE )

#define GROUP_IDX_MASK \
    ( BLKGRP_NUMBER - 1 )

// group index in a block index
#define GROUP_IDX_SHIFT \
    ( 1 << ( find_shift( GROUP_SIZE ) ) )

#define LA_ADDR( _byte_addr ) \
    ( _byte_addr - SUPER_BLOCK_SIZE - GRPBMP_SIZE )

#define ABS_ADDR( _la_addr ) \
    ( _la_addr + SUPER_BLOCK_SIZE + GRPBMP_SIZE )

// get a tripple( group_idx, blk_idx, offset ) from absolute address
#define BYTEADDR_TO_OFF_IN_BLK( _byte_addr ) \
    ( ( LA_ADDR( _byte_addr ) ) & ( BLOCK_SIZE - 1 ) )

#define BYTEADDR_TO_BLKIDX( _byte_addr ) \
    ( ( ( LA_ADDR( _byte_addr ) ) >> BLOCK_SHIFT  ) & BLOCK_IDX_MASK )

#define BYTEADDR_TO_GRPIDX( _byte_addr ) \
    ( ( ( LA_ADDR( _byte_addr ) ) >> ( BLOCK_SHIFT + GROUP_SHIFT ) ) & GROUP_IDX_MASK );

#define BYTEADDR_TO_CHS( _byte_addr, _grpidx, _blkidx, _off_in_blk )  \
({ \
    _off_in_blk = BYTEADDR_TO_OFF_IN_BLK( _byte_addr );  \
    _blkidx = BYTEADDR_TO_BLKIDX( _byte_addr ); \
    _grpidx = BYTEADDR_TO_GRPIDX( _byte_addr ); \
});

// block address is a combination of block idx and
// group idx. It is used by inode.
#define BLKADDR_TO_BLKIDX( _blk_addr ) \
    ( ( ( _blk_addr ) >>   ) & ( BLOCKS_PER_GROUP - 1 ) )

// get the group idx from the block addr
#define GROUP_INDEX( _blk_addr ) \
    ( ( _blk_addr >> BLOCK_SHIFT ) & GROUP_IDX_MASK ) 

// get the logical address of a group from a with a
// block index from inode
#define GROUP_START( _blk_addr ) \
    ( GROUP_INDEX( _blk_addr )  * GROUP_SIZE )

// get the logical address of a group's data section
// from a with a block index from inode
#define GROUP_DATA_START( _blk_addr ) \
    ( GROUP_INDEX( _blk_addr )  * GROUP_SIZE + \
    BLKBMP_BLKNUM * BLOCK_SIZE )

// get the block idx from the block addr
#define BLOCK_INDEX( _blk_addr ) \
    ( ( _blk_addr ) & BLOCK_IDX_MASK )

// get the logical address of a block
#define BLOCK_LA( _blk_addr ) \
    ( GROUP_DATA_START( _blk_addr ) + \
    ( BLOCK_IDX( _blk_addr ) * BLOCK_SIZE ) )

#define BLOCK_ABS( _blk_addr ) \
    ABS_ADDR( BLOCK_LA( _blk_addr ) )

struct ISynchronize
{
    virtual gint32 Flush() = 0;
    virtual gint32 Format() = 0;
    virtual gint32 Reload() = 0;
};

struct CSuperBlock : public ISynchronize
{
    guint32     m_dwMagic;
    guint32     m_dwMagic = htonl( *( guint32* )"regi" );
    guint32     m_dwVersion = 0x01;
    guint32     m_dwGroupNum = 0;
    guint32     m_dwBlkSize = BLKSIZE;
    CBlockAllocator* m_pParent;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
    inline void SetParent( CBlockAllocator* pParent )
    { m_pParent = pParent; }
};

using SblkPtr = std::unique_ptr< CSuperBlock >;

struct CBlockBitmap
{
    guint8 m_arrBytes[ BLOCKS_PER_GROUP / BYTE_BITS];
}
using BlkBmpPtr = std::unique_ptr< CBlockBitmap >;

struct CGroupBitmap
{
    guint8 m_arrBytes[ BLKGRP_NUMBER / BYTE_BITS];
}
using GrpBmpPtr = std::unique_ptr< CGroupBitmap >;

struct CBlockGroup : public ISynchronize
{
    CBlockAllocator* m_pParent;
    BlkBmpPtr m_pBlockMap;
    guint8 m_arrDataSec[ 0 ];
    guint32 m_dwGroupIdx = 0;
    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;

    inline void SetParent( CBlockAllocator* pParent )
    { m_pParent = pParent; }
}

using BlkGrpPtr = std::unique_ptr< CBlockGroup >;

template< guint32 BLKSIZE >
struct CBlockAllocator
    public CObjBase,
    public ISynchronize
{
    CSharedLock m_oLock;
    gint32      m_iFd;

    SblkPtr     m_pSuperBlock;
    GrpGmpPtr   m_pGroupBitmap;
    std::hashmap< guint32, BlkGrpPtr > m_mapBlkGrps;

    CBlockAllocator( const IConfigDb* pCfg );

    gint32 Format() override;
    gint32 Flush() override;
    gint32 Reload() override;

    gint32 FreeBlocks(
        const guint32* pvecBlocks,
        guint32 dwNumBlocks );

    gint32 AllocBlocks( guint32 dwNumBlocks,
        std::vector< guint32 >& vecBlocks );

    // write pBuf to dwBlockIdx, till the end of the
    // block or the end of the pBuf
    gint32 WriteBlock(
        guint32 dwBlockIdx, BufPtr& pBuf );

    // write pBuf to an array of blocks, till all the
    // blocks are written or all the bytes are written
    // from the pBuf.
    gint32 WriteBlocks( const guint32* pBlocks,
        guint32 dwNumBlocks, BufPtr& pBuf );

    // read block at dwBlockIdx to pBuf, till the end of the
    // block.
    gint32 ReadBlock(
        guint32 dwBlockIdx, BufPtr& pBuf );

    // read blocks to pBuf specified by block index
    // from pBlocks, till the end of the pBlocks.
    gint32 ReadBlocks( const guint32* pBlocks,
        guint32 dwNumBlocks, BufPtr& pBuf );

    gint32 AllocBlkGroup( guint32 dwNumGroups, 
        std::vector< guint32 >& vecGroups );

    gint32 FreeBlkGroups( const guint32* pGroups,
        guint32 dwNumGroups );

    gint32 IsBlkGroupFree( guint32 dwGroupIdx ) const;

    gint32 IsBlockFree( guint32 dwBlkIdx ) const;
};

