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
// group idx. It is used by inode's block table.
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

#define INODE_SIZE ( BLOCK_SIZE )

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

using SblkUPtr = std::unique_ptr< CSuperBlock >;

struct CBlockBitmap
{
    guint8 m_arrBytes[ BLOCKS_PER_GROUP / BYTE_BITS];
}
using BlkBmpUPtr = std::unique_ptr< CBlockBitmap >;

struct CGroupBitmap
{
    guint8 m_arrBytes[ BLKGRP_NUMBER / BYTE_BITS];
}
using GrpBmpUPtr = std::unique_ptr< CGroupBitmap >;

struct CBlockGroup : public ISynchronize
{
    CBlockAllocator* m_pParent;
    BlkBmpUPtr m_pBlockMap;
    guint8 m_arrDataSec[ 0 ];
    guint32 m_dwGroupIdx = 0;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;

    inline void SetParent( CBlockAllocator* pParent )
    { m_pParent = pParent; }
}

using BlkGrpUPtr = std::unique_ptr< CBlockGroup >;

template< guint32 BLKSIZE >
struct CBlockAllocator
    public CObjBase,
    public ISynchronize
{
    CSharedLock m_oLock;
    gint32      m_iFd;

    SblkUPtr     m_pSuperBlock;
    GrpBmpUPtr   m_pGroupBitmap;
    std::hashmap< guint32, BlkGrpUPtr > m_mapBlkGrps;

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
    // block or the end of the pBuf.
    // dwOff is the offset into the first block to
    // write
    gint32 WriteBlock(
        guint32 dwBlockIdx, guint8* pBuf,
        guint32 dwSize, guint32 dwOff = 0 );

    // write pBuf to an array of blocks, till all the
    // blocks are written or all the bytes are written
    // from the pBuf.
    // dwOff is the offset into the first block to
    // write
    gint32 WriteBlocks( const guint32* pBlocks,
        guint32 dwNumBlocks, guint8* pBuf,
        guint32 dwSize, guint32 dwOff = 0 );

    // read block at dwBlockIdx to pBuf, till the end of the
    // block.
    // dwOff is the offset into the first block to
    // read
    gint32 ReadBlock( guint32 dwBlockIdx,
        guint8* pBuf, guint32 dwSize,
        guint32 dwOff = 0 );

    // read blocks to pBuf specified by block index
    // from pBlocks, till the end of the pBlocks.
    // dwOff is the offset into the first block to
    // read
    gint32 ReadBlocks( const guint32* pBlocks,
        guint32 dwNumBlocks, guint8* pBuf
        guint32 dwSize, guint32 dwOff = 0 );

    gint32 AllocBlkGroup( guint32 dwNumGroups, 
        std::vector< guint32 >& vecGroups );

    gint32 FreeBlkGroups( const guint32* pGroups,
        guint32 dwNumGroups );

    gint32 IsBlkGroupFree( guint32 dwGroupIdx ) const;

    gint32 IsBlockFree( guint32 dwBlkIdx ) const;
};

using AllocPtr = CCAutoPtr< clsid( CBlockAllocator ), CBlockAllocator >;

struct RegFSInode
{
    // file type
    guint16     m_wMode;
    // file size in bytes
    guint32     m_dwSize;
    // time of last modification.
    guint32     m_dwmtime;
    // time of last access.
    guint32     m_dwatime;
    // uid
    guint16     m_wuid;
    // gid
    guint16     m_wgid;
    // type of file content
    guint32     m_dwFlags;
    // the block address for the parent directory
    guint32     m_dwParentInode;
    // blocks for data section.
    guint32     m_arrBlocks[ 15 ];

    // blocks for extended inode
    guint32     m_arrMetaFork[ 4 ];
    // for small data with length less than 96 bytes.
    union
    {
        Variant     m_oValue;
        guint8      m_arrBuf[ 96 ];
    }
};

#define BPNODE_SIZE ( PAGE_SIZE )
#define BPNODE_BLKNUM ( BPNODE_SIZE / BLOCK_SIZE )

#define NAME_LENGTH 96

#define DIR_ENTRY_SIZE 128

#define MAX_PTR_PER_NODE ( \
    ( BPNODE_SIZE - DIR_ENTRY_SIZE ) / DIR_ENTRY_SIZE )

#define LEAST_NUM_CHILD ( ( MAX_PTR_PER_NODE + 1 ) >> 1 )
#define LEASE_NUM_KEY ( LEAST_NUM_CHILD - 1 )

struct RegFSBPlusNode
{
    char*       m_arrKeys[ MAX_PTR_PER_NODE - 1 ][ NAME_LENGTH ]; 
    guint32     m_arrNodeIdx[ MAX_PTR_PER_NODE ];
    guint16     m_wNumKeys = 0;
    guint16     m_wNumNodeIdx = 0;
    guint16     m_wCurNodeIdx = 0;
    guint16     m_wParentIdx = 0;
    bool        m_bLeaf = false;
};

struct RegFSDirEntry
{
    // block address for the file inode
    guint32     m_dwInode = 0;
    // file type
    guint8      m_byType = 0;
};

struct RegFSBPlusLeaf
    public RegFSBPlusNode
{
    RegFSDirEntry m_arrDirEntry[ MAX_PTR_PER_NODE - 1 ];
    guint16     m_wNextLeaf = 0;
};

struct KEY_SLOT {
    const char* szKey;
    union{
        guint32 dwNodeIdx;
        struct{
            guint32 dwInode;
            guint8  byType;
        } oLeaf;
    }
};

class CBPlusNode;
using BPNodeUPtr = std::unique_ptr< CBPlusNode >; 

struct CBPlusNode :
    public ISynchronize 
{
    RegFSBPlusNode m_oNodeStore;
    std::vector< CBPlusNode > m_vecChilds;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

class COpenFileEntry;
using FileSPtr = std::shared_ptr< COpenFileEntry >;

struct CBPlusLeaf :
    public CBPlusNode
{
    RegFSBPlusLeaf& m_oLeafStore;
    CBPlusLeaf() : CBPlusNode()
        m_oLeafStore( *(RegFSBPlusLeaf*)&m_oNodeStore )
    { m_oLeafStore.m_bLeaf = true, }
    std::vector< FileSPtr > m_vecFiles;

    CBPlusLeaf* m_pNextLeaf;
    guint32 GetFileInode( const char* szName ) const;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

struct COpenFileEntry :
    public ISynchronize
{
    CSharedLock m_oLock;

    guint32     m_dwInodeIdx;
    RegFSInode m_oInodeStore;
    stdstr m_strFileName;

    std::atomic< guint32 > m_dwRefs;
    FileSPtr m_pParentDir;

    inline guint32 AddRef()
    { return ++m_dwRefs; }
    inline guint32 DecRef()
    { return --m_dwRefs; }
    inline guint32 GetRef() const;
    { return m_dwRefs; }

    gint32 ReadFile( guint32 dwOff,
        guint32 size, guint8* pBuf );

    gint32 WriteFile( guint32 dwOff,
        guint32 size, guint8* pBuf );

    gint32 ReadValue( Variant& oVar );
    gint32 WriteValue( const Variant& oVar );

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

struct CAccessContext
{
    guint16 dwUid = 0xffff;
    guint16 dwGid = 0xffff;
};

struct CDirectoryFile :
    public COpenFileEntry
{
    BPNodeUPtr m_pRootNode;
    
    CAccessContext m_oUserAc;

    inline bool IsRootDir() const
    { m_oINodeStore.m_dwParentInode == 0; }

    gint32 NewFile( const stdstr& strName,
        guint32 dwOwner = 0,
        guint32 dwPerm = 0 );

    gint32  NewDirectory( const stdstr& strName,
        guint32 dwOwner = 0,
        guint32 dwPerm = 0 );

    HANDLE OpenChild(
        const stdstr& strName,
        CAccessContext* pAc );

    gint32  CloseChild( FileSPtr& pFile );
    gint32  FindChild( const stdstr& strName ) const;
    gint32  RemoveChild( const stdstr& strName );
    gint32  RemoveDirectory( const stdstr& strName );

    gint32  SetGid( guint16 wGid );
    gint32  SetUid( guint16 wUid );

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};



class CRegistryFs :
    public IService,
    public ISynchronize
{
    AllocPtr    m_pBlkAlloc;
    FileSPtr    m_pRootDir;    

    public:
    CRegistryFs( const IConfigDb* pCfg );
    gint32 Start() override;
    gint32 Stop() override;

    static gint32 Namei(
        const string& strPath,
        std::vector<stdstr>& vecNames ) const;

    gint32 NewFile( const stdstr& strPath,
        guint32 dwOwner = 0,
        guint32 dwPerm = 0 );

    gint32  NewDirectory( const stdstr& strPath,
        guint32 dwOwner = 0,
        guint32 dwPerm = 0 );

    HANDLE OpenChild( const stdstr& strPath );

    gint32  CloseChild( FileSPtr& pFile );
    gint32  FindChild( const stdstr& strPath ) const;
    gint32  RemoveChild( const stdstr& strPath );
    gint32  RemoveDirectory( const stdstr& strPath );

    gint32  SetGid( guint16 wGid );
    gint32  SetUid( guint16 wUid );
};

using RegFsPtr = CCAutoPtr< clsid( CRegistryFs ), CRegistryFs >;
