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


#define DEFAULT_BLOCK_SIZE  512
#define DEFAULT_PAGE_SIZE   PAGE_SIZE
#define BLOCK_SIZE          ( GetBlockSize() )
#define REGFS_PAGE_SIZE     ( GetPageSize() )

#define SUPER_BLOCK_SIZE    ( BLOCK_SIZE )

#define BLOCK_SHIFT         ( find_shift( BLOCK_SIZE ) - 1 )

#define BLKGRP_GAP_BLKNUM ( sizeof( guint16 ) * BYTE_BITS )
#define BLOCKS_PER_GROUP ( 64 * 1024 - BLKGRP_GAP_BLKNUM )
#define BLOCKS_PER_GROUP_FULL ( 64 * 1024 )

// blocks occupied by block bitmap.
#define BLKBMP_BLKNUM \
    ( BLOCKS_PER_GROUP_FULL / ( BLOCK_SIZE * BYTE_BITS ) )

#define BLOCK_IDX_MASK \
    ( BLOCKS_PER_GROUP_FULL - 1 )

#define GROUP_SHIFT \
    ( find_shift( REGFS_PAGE_SIZE ) )

#define BLKGRP_NUMBER \
    ( ( 1 << GROUP_SHIFT ) - sizeof( guint16 ) * BYTE_BITS )

#define BLKGRP_NUMBER_FULL ( ( 1 << GROUP_SHIFT ) )

#define GRPBMP_BLKNUM \
    ( BLKGRP_NUMBER_FULL / ( BLOCK_SIZE * BYTE_BITS ) )

#define GRPBMP_SIZE \
    ( BLKGRP_NUMBER_FULL / BYTE_BITS )

#define GRPBMP_START \
    ( SUPER_BLOCK_SIZE )

#define GROUP_SIZE \
    ( BLOCKS_PER_GROUP_FULL * BLOCK_SIZE )

#define GROUP_IDX_MASK \
    ( ( 1 << GROUP_SHIFT ) - 1 )

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
#define LA_TO_BLKIDX( _la_ ) \
    ( ( ( _la_ ) >> BLOCK_SHIFT  ) & ( BLOCKS_PER_GROUP_FULL - 1 ) )

#define GROUP_INDEX_SHIFT \
    ( find_shift( BLOCKS_PER_GROUP_FULL ) - 1 )

// get the group idx from the block index
#define GROUP_INDEX( _blk_idx ) \
    ( ( _blk_idx >> GROUP_INDEX_SHIFT ) & GROUP_IDX_MASK ) 

// get the logical address of a group from a with a
// block index from inode
#define GROUP_START( _blk_idx ) \
    ( GROUP_INDEX( _blk_idx )  * GROUP_SIZE * BLOCK_SIZE )

// get the logical address of a group's data section
// from a with a block index from inode
#define GROUP_DATA_START( _blk_idx ) \
    ( GROUP_INDEX( _blk_idx )  * GROUP_SIZE * BLOCK_SIZE + \
    BLKBMP_BLKNUM * BLOCK_SIZE )

// get the block idx from the block addr
#define BLOCK_INDEX( _blk_idx ) \
    ( ( _blk_idx ) & BLOCK_IDX_MASK )

// get the logical address of a block
#define BLOCK_LA( _blk_idx ) \
    ( GROUP_START( _blk_idx ) + \
    ( BLOCK_INDEX( _blk_idx ) * BLOCK_SIZE ) )

#define BLOCK_ABS( _blk_idx ) \
    ABS_ADDR( BLOCK_LA( _blk_idx ) )

#define INODE_SIZE ( BLOCK_SIZE )
#define ROOT_INODE_BLKIDX ( BLKBMP_BLKNUM )

#define BNODE_IDX_TO_POS( _idx_ ) \
    ( _idx_ << ( find_shift( BNODE_SIZE ) - 1  ) )

#define MAX_BNODE_NUMBER \
    ( MAX_FILE_SIZE / BNODE_SIZE )

#define DECL_ARRAY( _array_, _size_ )\
    BufPtr pback##_array_(true); \
    pback##_array_->Resize( _size_ ); \
    auto _array_ = pback##_array_->ptr(); \

#define DECL_ARRAY2( _type_, _array_, _count_ )\
    BufPtr pback##_array_(true); \
    pback##_array_->Resize( _count_ * sizeof( _type_ ) ); \
    auto _array_ = ( _type_* )pback##_array_->ptr(); \

namespace rpcf{

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
    guint32     m_dwBlkSize = DEFAULT_BLOCK_SIZE;
    guint32     m_dwPageSize = DEFAULT_PAGE_SIZE;
    CBlockAllocator* m_pAlloc;

    CSuperBlock( CBlockAllocator* pAlloc )
        : m_pAlloc( pAlloc )
    {}
    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
} ;

using SblkUPtr = typename std::unique_ptr< CSuperBlock >;

guint32 g_dwBlockSize = DEFAULT_BLOCK_SIZE;
guint32 g_dwRegFsPageSize = DEFAULT_PAGE_SIZE;

inline guint32 GetBlockSize()
{ return g_dwBlockSize; }

inline guint32 GetPageSize()
{ return g_dwBlockSize; }

struct CBlockBitmap :
    public ISynchronize
{
    guint8* m_arrBytes;
    BufPtr m_pBytes;

    CBlockAllocator* m_pAlloc;
    guint16 m_wFreeCount;
    guint32 m_dwGroupIdx = 0;
    inline guint32 GetFreeCount() const
    { return m_wFreeCount; }

    CBlockBitmap( CBlockAllocator* pAlloc,
        guint32 dwGrpIdx ) :
        m_pAlloc( pAlloc ),
        m_dwGroupIdx( dwGrpIdx )
    {
        m_pBytes.NewObj();
        m_pBytes.Resize(
            BLOCKS_PER_GROUP_FULL / BYTE_BITS );
        m_arrBytes = m_pBytes->ptr();
    }

    gint32 AllocBlocks(
        guint32* pvecBlocks,
        guint32& dwNumBlocks );

    gint32 FreeBlocks(
        const guint32* pvecBlocks,
        guint32 dwNumBlocks );

    gint32 InitBitmap();

    gint32 GetGroupIndex() const
    { return m_dwGroupIdx; }

    gint32 IsBlockFree( guint32 dwBlkIdx ) const;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
} ;

using BlkBmpUPtr = typename std::unique_ptr< CBlockBitmap >;

struct CBlockGroup : public ISynchronize
{
    CBlockAllocator* m_pAlloc;
    BlkBmpUPtr m_pBlockBitmap;

    CBlockGroup(
        CBlockAllocator* pAlloc,
        guint32 dwGrpIdx )
    {
        m_pBlockBitmap = BlkBmpUPtr(
            new CBlockBitmap( pAlloc, dwGrpIdx ) );
        m_pAlloc = pAlloc;
    }

    gint32 Flush() override
    { return m_pBlockBitmap->Flush(); }

    gint32 Format() override
    { return m_pBlockBitmap->Format(); }

    gint32 Reload() override
    { return m_pBlockBitmap->Reload(); }


    inline guint32 GetFreeCount() const
    { return m_pBlockBitmap->GetFreeCount(); }

    inline guint32 GetGroupIndex() const
    { return m_pBlockBitmap->GetGroupIndex(); }

    inline gint32 AllocBlocks(
        guint32* pvecBlocks,
        guint32& dwNumBlocks )
    {
        return m_pBlockBitmap->AllocBlocks(
            pvecBlocks, dwNumBlocks );
    }

    inline gint32 FreeBlocks(
        const guint32* pvecBlocks,
        guint32 dwNumBlocks )
    {
        return m_pBlockBitmap->FreeBlocks(
            pvecBlocks, dwNumBlocks );
    }

    gint32 IsBlockFree( guint32 dwBlkIdx ) const
    {
        return m_pBlockBitmap->IsBlockFree(
            dwBlkIdx );
    }
};

using BlkGrpUPtr = typename std::unique_ptr< CBlockGroup >;
struct CGroupBitmap
{
    guint8* m_arrBytes; //[ BLKGRP_NUMBER_FULL ];
    BufPtr m_pBytes;
    guint16 m_wFreeCount;
    inline guint32 GetFreeCount() const
    { return m_wFreeCount; }

    inline guint32 GetAllocCount() const
    { return BLKGRP_NUMBER - m_wFreeCount; }

    CGroupBitmap()
    {
        m_pBytes.NewObj();
        m_pBytes.Resize( BLKGRP_NUMBER_FULL );
        m_arrBytes = m_pBytes->ptr();
    }

    gint32 FreeGroup( guint32 dwGrpIdx );
    gint32 AllocGroup( guint32& dwGrpIdx );
    gint32 InitBitmap();
    gint32 IsBlockGroupFree(
        guint32 dwGroupIdx ) const;

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

using GrpBmpUPtr = typename std::unique_ptr< CGroupBitmap >;

using AllocPtr = typename CAutoPtr< clsid( CBlockAllocator ), CBlockAllocator >;

using CONTBLKS=std::pair< guint32, guint32 >;

class CBlockAllocator :
    public CObjBase,
    public ISynchronize
{
    mutable stdrmutex   m_oLock;
    gint32              m_iFd;

    SblkUPtr            m_pSuperBlock;
    GrpBmpUPtr          m_pGroupBitmap;
    std::hashmap< guint32, BlkGrpUPtr > m_mapBlkGrps;

    gint32 ReadWriteBlocks(
        bool bRead, const guint32* pBlocks,
        guint32 dwNumBlocks, guint8* pBuf,
        bool bContigous = false );

    gint32 ReadWriteFile(
        char* pBuf, guint32 dwSize,
        guint32 dwOff, bool bRead );

    public:
    CBlockAllocator( const IConfigDb* pCfg );
    inline stdrmutex& GetLock() const
    { return m_oLock; }

    gint32 Format() override;
    gint32 Flush() override;
    gint32 Reload() override;

    gint32 SaveGroupBitmap(
        const* pbmp, guint32 dwSize );

    gint32 LoadGroupBitmap(
        char* pbmp, guint32 dwSize );

    gint32 SaveSuperBlock(
        const char* pSb, guint32 dwSize );

    gint32 LoadSuperBlock(
        char* pSb, guint32 dwSize ) const;

    gint32 FreeBlocks(
        const guint32* pvecBlocks,
        guint32 dwNumBlocks,
        bool bByteOrder = false );

    gint32 AllocBlocks( 
        guint32* pvecBlocks,
        guint32& dwNumBlocks );

    // write pBuf to dwBlockIdx, till the end of the
    // block or the end of the pBuf.
    gint32 WriteBlock(
        guint32 dwBlockIdx, const guint8* pBuf );

    // write pBuf to an array of blocks, till all the
    // blocks are written or all the bytes are written
    // from the pBuf.
    gint32 WriteBlocks( const guint32* pBlocks,
        guint32 dwNumBlocks, const guint8* pBuf,
        bool bContigous = false );

    // read block at dwBlockIdx to pBuf, till the end of the
    // block.
    gint32 ReadBlock( guint32 dwBlockIdx,
        guint8* pBuf );

    // read blocks to pBuf specified by block index
    // from pBlocks, till the end of the pBlocks.
    gint32 ReadBlocks( const guint32* pBlocks,
        guint32 dwNumBlocks, guint8* pBuf
        bool bContigous = false );

    inline gint32 IsBlockGroupFree(
        guint32 dwGroupIdx ) const
    {
        return m_pGroupBitmap->IsBlockGroupFree(
            dwGroupIdx );
    }

    gint32 IsBlockFree( guint32 dwBlkIdx ) const;

    gint32 FindContigousBlocks(
        const guint32* pBlocks,
        guint32 dwNumBlocks,
        std::vector< CONTBLKS >& vecBlocks,
        bool bByteOrder = false );

    gint32 ReadWriteBlocks2(
        std::vector< CONTBLKS >& vecBlocks,
        guint8* pBuf,
        bool bRead );
};

struct RegFSInode
{
    // file size in bytes
    guint32     m_dwSize;
    // time of last modification.
    guint32     m_dwmtime;
    // time of last access.
    guint32     m_dwatime;
    // time of creation
    guint32     m_dwctime;
    // file type
    guint32     m_dwMode;
    // uid
    guint16     m_wuid;
    // gid
    guint16     m_wgid;

    // type of file content
    guint32     m_dwFlags;
    // the block address for the parent directory
    guint32     m_arrBlocks[ 15 ];
    guint32     m_dwReserved;

    // blocks for extended inode
    guint32     m_arrMetaFork[ 4 ];

    // for small data with length less than 96 bytes.
    guint8      m_arrBuf[ 95 ];
    guint8      m_iValType;

} __attribute__((aligned (8)));

#define BNODE_SIZE ( REGFS_PAGE_SIZE )
#define BNODE_BLKNUM ( BNODE_SIZE / BLOCK_SIZE )

#define REGFS_NAME_LENGTH 96

// #define DIR_ENTRY_SIZE 128

#define MAX_PTRS_PER_NODE ( \
    ( BNODE_SIZE / sizeof( KEYPTR_SLOT ) - 1 )

#define MAX_KEYS_PER_NODE ( MAX_PTRS_PER_NODE - 1 )

#define LEAST_NUM_CHILD ( ( MAX_PTRS_PER_NODE + 1 ) >> 1 )
#define LEASE_NUM_KEY ( LEAST_NUM_CHILD - 1 )

#define DIRECT_BLOCKS 13

#define INDIRECT_BLOCKS \
    ( BLOCK_SIZE / sizeof( guint32 ) )

#define SEC_INDIRECT_BLOCKS \
    ( INDIRECT_BLOCKS * INDIRECT_BLOCKS ) 

#define MAX_FILE_SIZE ( BLOCK_SIZE * 13 + \
    INDIRECT_BLOCKS * BLOCK_SIZE + \
    SEC_INDIRECT_BLOCKS * BLOCK_SIZE )

#define WITHIN_DIRECT_BLOCK( _offset_ ) \
    ( _offset_ >= 0 && _offset_ < INDIRECT_BLOCKS * BLOCK_SIZE )

#define BEYOND_MAX_LIMIT( _offset_ ) \
    ( _offset_ >= MAX_FILE_SIZE )

#define INDIRECT_BLOCK_START ( DIRECT_BLOCKS * BLOCK_SIZE )

#define SEC_INDIRECT_BLOCK_START \
    ( INDIRECT_BLOCK_START + INDIRECT_BLOCKS * BLOCK_SIZE )

#define WITHIN_INDIRECT_BLOCK( _offset_ ) \
    ( _offset_ >= INDIRECT_BLOCK_START && \
     _offset_ < SEC_INDIRECT_BLOCKS * BLOCK_SIZE )

// secondary indirect mapping block
#define WITHIN_SEC_INDIRECT_BLOCK( _offset_ ) \
    ( _offset_ >= SEC_INDIRECT_BLOCK_START  && \
     _offset_ < MAX_FILE_SIZE )

#define BLKIDX_PER_TABLE ( BLOCK_SIZE >> 2 )

#define BLKIDX_PER_TABLE_SHIFT \
    ( find_shift( BLOCK_SIZE >> 2 ) - 1 )

#define BLKIDX_PER_TABLE_MASK \
    ( BLKIDX_PER_TABLE - 1 )

#define BIT_IDX  13
#define BITD_IDX  14

#define INDIRECT_BLK_IDX( _offset_ ) \
    ( ( _offset_ - INDIRECT_BLOCK_START ) >> BLOCK_SHIFT )

#define SEC_BLKIDX_TABLE_IDX( _offset_ ) \
    ( ( ( _offset_ - SEC_INDIRECT_BLOCK_START ) >> \
        ( BLOCK_SHIFT + BLKIDX_PER_TABLE_SHIFT ) ) & \
        BLKIDX_PER_TABLE_MASK )

#define SEC_BLKIDX_IDX( _offset_ ) \
    ( ( ( _offset_ - SEC_INDIRECT_BLOCK_START ) >> \
        BLOCK_SHIFT ) & BLKIDX_PER_TABLE_MASK )

struct CFileImage : 
    public ISynchronize
{
    AllocPtr m_pAlloc;
    guint32 m_dwInodeIdx;
    guint32 m_dwParentInode;

    // mapping offset into file to block
    std::map< guint32, BufPtr > m_mapBlocks;

    // block index table 
    BufPtr m_pBitBlk;

    // block index table directory
    BufPtr m_pBitdBlk;
    // catch of the secondary block index tables
    std::hashmap< guint32, BufPtr > m_mapSecBitBlks;
    

    RegFSInode  m_oInodeStore;
    Variant     m_oValue;

    mutable CSharedLock m_oLock;
    inline CSharedLock& GetLock() const
    { return m_oLock; }

    // this lock is used under readlock
    mutable stdrmutex m_oExclLock;
    inline stdrmutex& GetExclLock() const
    { return m_oExclLock; }

    CFileImage( CBlockAllocator* pAlloc,
        guint32 dwInode,
        guint32 dwParentIdx ) :
        m_pAlloc( pAlloc ),
        m_dwInodeIdx( dwInode ),
        m_dwParentInode( dwParentIdx )
    {}

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;

    gint32 ReadFile( guint32 dwOff,
        guint32& size, guint8* pBuf );

    gint32 CollectBlocksForRead(
        guint32 dwOff, guint32 dwSize,
        std::vector< guint32 > vecBlocks );

    gint32 CollectBlocksForWrite(
        guint32 dwOff, guint32& dwSize,
        std::vector< guint32 > vecBlocks );

    gint32 WriteFile( guint32 dwOff,
        guint32 size, guint8* pBuf );

    gint32 ReadValue( Variant& oVar );
    gint32 WriteValue( const Variant& oVar );

    gint32 TruncBlkDirect( guint32 lablkidx );
    gint32 TruncBlkIndirect( guint32 lablkidx );
    gint32 TruncBlkSecIndirect( guint32 lablkidx );

    gint32 Truncate( guint32 dwOff );
    gint32 Extend( guint32 dwOff );
};

using FImgSPtr = typename std::unique_ptr< CFileImage >;

struct KEYPTR_SLOT {
    char m_szKey[ NAME_LENGTH ];
    union{
        guint32 dwBNodeIdx;
        struct{
            guint32 dwBNodeIdx;
            guint8  byFileType;
            guint8  byReserved[ 3 ];
        } oLeaf;
    }
}__attribute__((aligned (8)));

struct RegFSBPlusNode
{
    BufPtr      m_pBuf;
    std::vector< KEYPTR_SLOT* > m_vecSlots;
    guint16     m_wNumKeys = 0;
    guint16     m_wNumBNodeIdx = 0;
    guint16     m_wThisBNodeIdx = 0;
    guint16     m_wParentBNode = 0;
    guint16     m_wNextLeaf = 0;
    bool        m_bLeaf = false;

    gint32  ntoh(
        const guint8* pSrc,
        guint32 dwSize );

    gint32  hton(
        guint8* pSrc,
        guint32 dwSize ) const;

    RegFSBPlusNode();
};

class CBPlusNode;
using BPNodeUPtr = typename std::unique_ptr< CBPlusNode >; 

struct CBPlusNode :
    public ISynchronize 
{
    FileImage* m_pFile;
    RegFSBPlusNode m_oBNodeStore;
    std::vector< BPNodeUPtr > m_vecChilds;
    guint32  m_dwMaxSlots = MAX_PTRS_PER_NODE;
    guint32 m_dwBNodeIdx = 0;

    CBPlusNode( CFileImage* pFile,
        guint32 dwBNodeIdx ) :
        m_pFile( pFile ),
        m_dwBNodeIdx( dwBNodeIdx )
    {
        m_vecChilds.resize( MAX_PTRS_PER_NODE );
    }

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

struct CBPlusLeaf :
    public CBPlusNode
{
    typedef CBPlusNode super;
    RegFSBPlusNode& m_oLeafStore;
    CBPlusLeaf() : CBPlusNode()
        m_oLeafStore( m_oNodeStore )
    { m_oLeafStore.m_bLeaf = true, }
    std::vector< FImgSPtr > m_vecFiles;

    CBPlusLeaf* m_pNextLeaf;
    gint32 GetInode(
        const char* szName,
        guint32 dwInode ) const;

    CBPlusLeaf( CFileImage* pFile,
        guint32 dwBNodeIdx ) :
        super( pFile, dwBNodeIdx )
    {}

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

struct CDirImage : 
    public CFileImage
{
    typedef CFileImage super;
    BPNodeUPtr m_pRootNode;

    CDirImage( CBlockAllocator* pAlloc,
        guint32 dwInodeIdx,
        guint32 dwParentIdx = 0 ):
        super( pAlloc, dwInodeIdx,
            dwParentIdx )
    {}
    gint32 Format() override;
    gint32 Reload() override;
    gint32 Flush() override;
};

class COpenFileEntry;
using FileSPtr = typename std::shared_ptr< COpenFileEntry >;

struct COpenFileEntry :
    public ISynchronize
{
    AllocPtr        m_pAlloc;
    guint32         m_dwInodeIdx = 0;
    stdstr          m_strFileName;
    FImgSPtr        m_pFileImage;
    CAccessContext  m_oUserAc;

    std::atomic< guint32 > m_dwRefs;
    FileSPtr m_pParentDir;

    COpenFileEntry(
        AllocPtr& pAlloc, FImgSPtr& pImage ) :
        m_pAlloc( pAlloc ),
        m_pFileImage( FImgSPtr )
    {
        m_dwInodeIdx = pImage->m_dwInodeIdx;
    }

    inline guint32 AddRef()
    { return ++m_dwRefs; }
    inline guint32 DecRef()
    { return --m_dwRefs; }
    inline guint32 GetRef() const;
    { return m_dwRefs; }

    void SetParent( FileSPtr& pParent )
    {
        if( pParent )
        {
            m_pParentDir = pParent;
            m_pParentDir->AddRef();
        }
    }

    inline gint32 ReadFile( guint32 dwOff,
        guint32& size, guint8* pBuf )
    {
        return m_pFileImage->ReadFile(
            dwOff, size, pBuf );
    }

    inline gint32 WriteFile( guint32 dwOff,
        guint32 size, guint8* pBuf )
    {
        return m_pFileImage->WriteFile(
            dwOff, size, pBuf );
    }
    inline gint32 ReadValue( Variant& oVar )
    { return m_pFileImage->ReadValue( oVar ); }

    inline gint32 WriteValue( const Variant& oVar );
    { return m_pFileImage->WriteValue( oVar ); }

    gint32 Flush() override
    { return m_pFileImage->Flush(); }

    gint32 Format() override
    { return 0; }

    gint32 Reload() override
    { return 0; }

    inline gint32 Truncate( guint32 dwOff )
    { return m_pFileImage->Truncate( dwOff ); }

    inline gint32 Open( FileSPtr& pParent,
        CAccessContext* pac )
    {
        SetParent( pParent );
        if( pac )
            m_oUserAc = *pac;
        return 0;
    }

    inline gint32 Close()
    {
        m_pParentDir->DecRef(); 
        return this->Flush();
    }
};

struct CAccessContext
{
    guint16 dwUid = 0xffff;
    guint16 dwGid = 0xffff;
};

struct CDirFileEntry :
    public COpenFileEntry
{
    typedef COpenFileEntry super;
    
    CDirFileEntry(
        AllocPtr& pAlloc, FImgSPtr& pImage ) :
        super( pAlloc, pImage )
    {}

    inline bool IsRootDir() const
    { m_pFileImage.m_dwParentInode == 0; }

    gint32 CreateFile( const stdstr& strName,
        guint32 dwFlags, guint32 dwMode );

    gint32 CreateSubDir( const stdstr& strName );

    HANDLE OpenChild(
        const stdstr& strName,
        FileSPtr& pParent,
        FImgSPtr& pFile,
        guint32 dwFlags,
        CAccessContext* pAc );

    gint32  CloseChild( FileSPtr& pFile );
    gint32  FindChild( const stdstr& strName ) const;
    gint32  RemoveFile( const stdstr& strName );
    gint32  RemoveSubDir( const stdstr& strName );

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
    FImgSPtr    m_pRootImg;
    std::hashmap< HANDLE, FilePtr > m_mapOpenFiles;

    gint32 CreateRootDir();
    gint32 OpenRootDir();

    public:
    CRegistryFs( const IConfigDb* pCfg );
    gint32 Start() override;
    gint32 Stop() override;

    static gint32 Namei(
        const string& strPath,
        std::vector<stdstr>& vecNames ) const;

    gint32 CreateFile( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32 CreateDirectory( const stdstr& strPath,
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

    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;
};

using RegFsPtr = typename CCAutoPtr< clsid( CRegistryFs ), CRegistryFs >;

}
