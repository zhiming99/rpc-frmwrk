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

#pragma once
#include <arpa/inet.h>
#include "rpc.h"
#include <memory>

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

#define DEFAULT_BLOCK_SIZE 512
#define BLOCK_SIZE  ( GetBlockSize() )

#define BLOCK_SHIFT ( find_shift( BLOCK_SIZE ) - 1 )

#define BLOCKS_PER_GROUP ( 64 * 1024 - sizeof( guint16 ) * BYTE_BITS )

#define BLKBMP_BLKNUM \
    ( BLOCKS_PER_GROUP / ( BLOCK_SIZE * BYTE_BITS ) )

#define BLOCK_IDX_MASK \
    ( BLOCKS_PER_GROUP - 1 )

#define GROUP_SHIFT \
    ( find_shift( PAGE_SIZE ) )

#define BLKGRP_NUMBER \
    ( ( 1 << GROUP_SHIFT ) - sizeof( guint16 ) * BYTE_BITS )

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
    guint32     m_dwMagic = htonl( *( guint32* )"regi" );
    guint32     m_dwVersion = 0x01;
    guint32     m_dwGroupNum = BLKGRP_NUMBER;
    guint32     m_dwBlkSize = DEFAULT_BLOCK_SIZE;
    CBlockAllocator* m_pParent;

    CSuperBlock();
    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
    inline void SetParent( CBlockAllocator* pParent )
    { m_pParent = pParent; }
} __attribute__((aligned (8)));

using SblkUPtr = typename std::unique_ptr< CSuperBlock >;

guint32 g_dwBlockSize = DEFAULT_BLOCK_SIZE;

inline guint32 GetBlockSize()
{ return g_dwBlockSize; }

struct CBlockBitmap
{
    guint8 m_arrBytes[ BLOCKS_PER_GROUP / BYTE_BITS +
        sizeof( guint16 ) * BYTE_BITS ];
    guint16 m_wFreeCount;
    inline guint32 GetFreeCount() const
    { return m_wFreeCount; }

    gint32 AllocBlocks(
        guint32* pvecBlocks,
        guint32& dwNumBlocks );

    gint32 FreeBlocks(
        const guint32* pvecBlocks,
        guint32 dwNumBlocks );

} __attribute__((aligned (8)));

using BlkBmpUPtr = typename std::unique_ptr< CBlockBitmap >;

struct CGroupBitmap
{
    guint8 m_arrBytes[ BLKGRP_NUMBER / BYTE_BITS +
        sizeof( guint16 ) * BYTE_BITS ];
    guint16 m_wFreeCount;
    inline guint32 GetFreeCount() const
    { return m_wFreeCount; }

    gint32 FreeGroup( guint32 dwGrpIdx );
    gint32 AllocGroup( guint32& dwGrpIdx );
    gint32 InitBitmap();

} __attribute__((aligned (8)));

using GrpBmpUPtr = typename std::unique_ptr< CGroupBitmap >;
struct CBlockGroup : public ISynchronize
{
    guint32 m_dwGroupIdx = 0;
    CBlockAllocator* m_pParent;
    BlkBmpUPtr m_pBlockMap;

    CBlockGroup( guint32 dwGrpIdx )
        : m_dwGroupIdx( dwGrpIdx )
    {}

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;

    inline guint32 GetFreeCount() const
    { return m_pBlockMap->GetFreeCount(); }

    inline void SetParent( CBlockAllocator* pParent )
    { m_pParent = pParent; }
};

using BlkGrpUPtr = typename std::unique_ptr< CBlockGroup >;

using AllocPtr = typename CAutoPtr< clsid( CBlockAllocator ), CBlockAllocator >;

class CBlockAllocator :
    public CObjBase,
    public ISynchronize
{
    mutable CStdRMutex  m_oLock;
    gint32              m_iFd;

    SblkUPtr            m_pSuperBlock;
    GrpBmpUPtr          m_pGroupBitmap;
    std::hashmap< guint32, BlkGrpUPtr > m_mapBlkGrps;

    public:
    CBlockAllocator( const IConfigDb* pCfg );

    gint32 Format() override;
    gint32 Flush() override;
    gint32 Reload() override;

    gint32 FreeBlocks(
        const guint32* pvecBlocks,
        guint32 dwNumBlocks );

    gint32 SaveGroupBitmap(
        const CGroupBitmap& gbmp );

    gint32 LoadGroupBitmap(
        CGroupBitmap& gbmp );

    gint32 SaveSuperBlock(
        char* pSb, guint32 dwSize );

    gint32 LoadSuperBlock(
        char* pSb, guint32 dwSize ) const;

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

struct CFileImage : 
    public ISynchronize
{
    AllocPtr m_pAlloc;
    guint32 m_dwInode;
    std::hashmap< guint32, BufPtr >& m_mapImage;

    RegFSInode m_oInodeStore;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;

    gint32 ReadFile( guint32 dwOff,
        guint32 size, guint8* pBuf );

    gint32 WriteFile( guint32 dwOff,
        guint32 size, guint8* pBuf );

    gint32 ReadValue( Variant& oVar );

    gint32 WriteValue( const Variant& oVar );

    gint32 Trim( guint32 dwOff );
    gint32 Grow( guint32 dwBlocks );
};

using FImgSPtr = typename std::unique_ptr< CFileImage >;

class CBPlusNode;
using BPNodeUPtr = typename std::unique_ptr< CBPlusNode >; 

struct CBPlusNode :
    public ISynchronize 
{
    AllocPtr m_pAlloc;
    RegFSBPlusNode m_oNodeStore;
    std::vector< CBPlusNode > m_vecChilds;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

struct CBPlusLeaf :
    public CBPlusNode
{
    RegFSBPlusLeaf& m_oLeafStore;
    CBPlusLeaf() : CBPlusNode()
        m_oLeafStore( *(RegFSBPlusLeaf*)&m_oNodeStore )
    { m_oLeafStore.m_bLeaf = true, }
    std::vector< FImgSPtr > m_vecFiles;

    CBPlusLeaf* m_pNextLeaf;
    guint32 GetFileInode( const char* szName ) const;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

class COpenFileEntry;
using FileSPtr = typename std::shared_ptr< COpenFileEntry >;

struct COpenFileEntry :
    public ISynchronize
{
    AllocPtr        m_pAlloc;
    guint32         m_dwInodeIdx;
    stdstr          m_strFileName;
    FImgSPtr        m_pFileImage;
    guint32         m_dwPos;
    CAccessContext  m_oUserAc;

    mutable CSharedLock m_oLock;
    inline CSharedLock& GetLock() const
    { return m_oLock; }

    std::atomic< guint32 > m_dwRefs;
    FileSPtr m_pParentDir;

    COpenFileEntry( AllocPtr& pAlloc );

    inline guint32 AddRef()
    { return ++m_dwRefs; }
    inline guint32 DecRef()
    { return --m_dwRefs; }
    inline guint32 GetRef() const;
    { return m_dwRefs; }

    gint32 ReadFile(
        guint32 size, guint8* pBuf );

    gint32 WriteFile(
        guint32 size, guint8* pBuf );

    gint32 GetValue( Variant& oVar );
    gint32 SetValue( const Variant& oVar );

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
    
    CDirectoryFile( AllocPtr& pAlloc );

    inline bool IsRootDir() const
    { m_oINodeStore.m_dwParentInode == 0; }

    gint32 NewFile( const stdstr& strName );

    gint32  NewDirectory( const stdstr& strName );

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
    AllocPtr    m_pAlloc;
    FileSPtr    m_pRootDir;    
    std::hashmap< HANDLE, FilePtr > m_mapOpenFiles;

    public:
    CRegistryFs( const IConfigDb* pCfg );
    gint32 Start() override;
    gint32 Stop() override;

    static gint32 Namei(
        const string& strPath,
        std::vector<stdstr>& vecNames ) const;

    gint32 NewFile( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32  NewDirectory( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    HANDLE OpenChild( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32  CloseChild( HANDLE hFile );

    gint32  FindChild( const stdstr& strPath,
        CAccessContext* pac = nullptr ) const;

    gint32  RemoveChild( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32  RemoveDirectory( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32  SetGid( guint16 wGid,
        CAccessContext* pac = nullptr );

    gint32  SetUid( guint16 wUid,
        CAccessContext* pac = nullptr );

    gint32  SymLink( const stdstr& strSrcPath,
        const stdstr& strDestPath,
        CAccessContext* pac = nullptr );

    gint32 GetValue(
        HANDLE hFile, Variant& oVar );

    gint32 SetValue(
        HANDLE hFile, Variant& oVar );
};

using RegFsPtr = typename CCAutoPtr< clsid( CRegistryFs ), CRegistryFs >;
