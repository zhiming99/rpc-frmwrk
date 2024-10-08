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

#define INVALID_BNODE_IDX  ( 0xFFFF )

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
    guint32     m_dwUserData;

} __attribute__((aligned (8)));

#define BNODE_SIZE ( REGFS_PAGE_SIZE )
#define BNODE_BLKNUM ( BNODE_SIZE / BLOCK_SIZE )

#define REGFS_NAME_LENGTH 96

// #define DIR_ENTRY_SIZE 128

#define MAX_PTRS_PER_NODE \
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
    public ISynchronize,
    public CObjBase
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

    CFileImage( const IConfigDb* pCfg )
    {
        SetClassId( clsid( CFileImage ) );
        CCfgOpener oCfg( pCfg );
        ObjPtr pObj;
        oCfg.GetObjPtr( 0, pObj );
        m_pAlloc = pObj;
        oCfg.GetIntProp( 1, m_dwInodeIdx );
        oCfg.GetIntProp( 2, m_dwParentInode );
    }

    static gint32 Create( EnumFileType byType
        FImgSPtr& pFile, AllocPtr& pAlloc,
        guint32 dwInodeIdx,
        guint32 dwParentInode, guint32 dwType )
    {
        CParamList oParams;
        oParams.Push( ObjPtr( pAlloc ) );
        oParams.Push( dwInodeIdx );
        oParams.Push( dwParentInode );
        EnumClsid iClsid = clsid( Invalid );
        if( byType == ftRegular )
            iClsid = clsid( CFileImage );
        else if( bType == ftDirectory )
            iClsid = clsid( CDirImage );
        else if( bType == ftLink )
            iClsid = clsid( CLinkImage );
        else
        {
            DebugPrint( ret, "Error not a valid "
                "type of file to create" );
        }
        ret = pFile.NewObj( iClsid,
            oParams.GetCfg() );
        return ret;
    }

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
    inline guint32 GetSize() const
    { return m_oInodeStore.m_dwSize; }

    inline guint32 GetInodeIdx() const
    { return m_oInodeStore.m_dwInodeIdx; }

    inline bool IsRootDir() const
    { return m_dwInodeIdx == ROOT_INODE_BLKIDX; }
};

using FImgSPtr  = typename CAutoPtr< clsid( Invalid ), CFileImage >;
typedef enum : guint8
{
    ftRegular = 1,
    ftDirectory = 2,
    ftLink = 3
}EnumFileType

struct CLinkImage :
    public CFileImage
{
    stdstr m_strLink;
    typedef CFileImage super;
    CLinkImage( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CLinkImage ) ); }

    gint32 Reload() override;
    gint32 Format() override;
};


struct KEYPTR_SLOT
{
    char szKey[ REGFS_NAME_LENGTH ] = {0,};
    union{
        guint32 dwBNodeIdx = INVALID_BNODE_IDX;
        struct{
            guint32 dwInodeIdx;
            EnumFileType  byFileType;
            guint8  byReserved[ 1 ];
            guint16 wUserData;
        } oLeaf;
    }
}__attribute__((aligned (8)));

struct RegFSBNode
{
    BufPtr      m_pBuf;
    std::vector< KEYPTR_SLOT* > m_vecSlots;
    guint16     m_wNumKeys = 0;
    guint16     m_wNumPtrs = 0;
    guint16     m_wThisBNodeIdx = INVALID_BNODE_IDX;
    guint16     m_wParentBNode = INVALID_BNODE_IDX;
    guint16     m_wNextLeaf = INVALID_BNODE_IDX;
    guint16     m_wPrevLeaf = INVALID_BNODE_IDX;
    bool        m_bLeaf = false;
    bool        m_bReserved = false;
    // only valid on root node
    guint16     m_wFreeBNodeIdx = INVALID_BNODE_IDX;

    gint32  ntoh(
        const guint8* pSrc,
        guint32 dwSize );

    gint32  hton(
        guint8* pSrc,
        guint32 dwSize ) const;

    inline guint32 GetBNodeIndex() const
    { return m_wThisBNodeIdx; }

    inline void SetBNodeIndex( guint32 dwIdx )
    { m_wThisBNodeIdx = dwIdx; }

    RegFSBNode();
};

class CBPlusNode;
using BNodeUPtr = typename std::unique_ptr< CBPlusNode >; 
using ChildMap = typename std::hashmap< guint32, BNodeUPtr >;
using FIleMap = typename std::hashmap< guint32, FImgSPtr >;

struct CBPlusNode :
    public ISynchronize 
{
    DirImage* m_pDir;
    RegFSBNode m_oBNodeStore;
    guint32  m_dwMaxSlots = MAX_PTRS_PER_NODE;
    CBPlusNode* m_pParent = nullptr;

    CBPlusNode( CDirImage* pFile,
        guint32 dwBNodeIdx ) :
        m_pDir( pFile ),
    {
        m_oBNodeStore.SetBNodeIndex( dwBNodeIdx );
    }

    ChildMap& GetChildMap()
    { return m_pDir->GetChildMap(); }

    const ChildMap& GetChildMap() const
    { return m_pDir->GetChildMap(); }

    inline const FileMap& GetFileMap() const
    { return m_pDir->GetFileMap(); }

    inline FileMap& GetFileMap()
    { return m_pDir->GetFileMap(); }


    gint32 Flush() override;
    gint32 Format() override;
    gint32 Reload() override;

    guint32 GetParentSlotIdx()
    {
        if( IsRoot() )
            return 0;
        CBPlusNode* pParent = GetParent();
        guint32 dwCount =
            pParent->GetChildCount();
        guint32 i = 0;
        for( ; i < dwCount; i++ )
        {
            if( pParent->GetSlot( i ).dwBNodeIdx ==
                this->GetBNodeIndex() )
                break;
        }
        if( i == dwCount )
            i = 0xFFFFFFFF;
        return i;
    }

    inline void SetParent( CBPlusNode* pParent )
    { m_pParent = pParent; }

    inline CBPlusNode* GetParent()
    { return m_pParent; }

    bool IsRoot()
    { return m_pParent == nullptr; }

    bool IsLeaf() const
    { return m_oBNodeStore.m_bLeaf; }

    void SetLeaf( bool bLeaf = false )
    { return m_oBNodeStore.m_bLeaf = bLeaf; }

    guint32 GetKeyCount() const
    { return m_oBNodeStore.m_wNumKeys; }

    void SetKeyCount( guint32 wKeys )
    {
         m_oBNodeStore.m_wNumKeys =
            ( guint16 )wKeys;
    }

    inline guint32 IncKeyCount()
    { return ++m_oBNodeStore.m_wNumKeys; }

    inline guint32 DecKeyCount()
    { return --m_oBNodeStore.m_wNumKeys; }

    inline guint32 IncChildCount()
    { return ++m_oBNodeStore.m_wNumPtrs; }

    inline guint32 DecChildCount()
    { return --m_oBNodeStore.m_wNumPtrs; }

    gint32 CopyBNodeStore( CBPlusNode* pSrc,
        guint32 dwOff, guint32 dwCount );

    inline guint32 GetBNodeIndex() const
    { return m_oBNodeStore.GetBNodeIndex(); }

    guint32 GetChildCount() const
    { return m_oBNodeStore.m_wNumPtrs; }

    void SetChildCount( guint32 dwBNode )
    {
        return m_oBNodeStore.m_wNumPtrs =
            ( guint16 )dwBNode;
    }

    const char* GetKey( gint32 idx ) const
    {
        if( idx >= GetKeyCount() )
            return nullptr;
        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];
        return ps->szKey;
    }

    FImgSPtr GetFile( gint32 idx ) const 
    {
        if( idx >= GetChildCount() )
            return nullptr;

        if( !IsLeaf() )
            return nullptr;

        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];

        auto mapFiles = GetFileMap();
        auto itr = mapFiles.find(
            ps->oLeaf.dwInodeIdx );

        auto& oMap = GetChildMap();
        if( itr == oMap.end() )
            return nullptr;
        return itr->second;
    }

    gint32 RemoveFileDirect (
        guint32 dwInodeIdx, FImgSPtr& pFile )
    {
        auto mapFiles = GetFileMap();
        auto itr = mapFiles.find( dwInodeIdx );
        if( itr == mapFiles.end() )
            return -ENOENT;
        pFile = *itr;
        itr->reset();
        return STATUS_SUCCESS;
    }

    gint32 RemoveFile(
        guint32 idx, FImgSPtr& pFile )
    {
        if( idx >= GetKeyCount() )
            return -EINVAL;
        if( !IsLeaf() )
            return -ENOTSUP;
        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];
        return RemoveFileDirect(
            ps->oLeaf.dwInodeIdx, pFile );
    }

    gint32 RemoveFile(
        const char* szKey, FImgSPtr& pFile );

    gint32 AddFileDirect(
        guint32 dwInodeIdx, FImgSPtr& pFile )
    {
        auto& mapFiles = GetFileMap();
        auto itr = mapFiles.find( dwInodeIdx );
        if( itr != mapFiles.end() )
            return -EEXIST;
        pFile = *itr;
        itr->reset();
        return STATUS_SUCCESS;
    }

    gint32 AddFile(
        guint32 idx, FImgSPtr& pFile )
    {
        if( idx >= GetChildCount() )
            return -EINVAL;
        if( !IsLeaf() )
            return -ENOTSUP;
        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];
        return AddFileDirect(
            ps->dwInodeIdx, pFile );
    }

    CBPlusNode* GetChildDirect( gint32 idx ) const
    {
        auto& oMap = GetChildMap();
        auto itr = oMap.find( idx );
        if( itr == oMap.end() )
            return nullptr;
        return itr->second.get();
    }

    CBPlusNode* GetChild( gint32 idx ) const
    {
        if( idx >= GetChildCount() || idx < 0 )
            return nullptr;
        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];
        return GetChildDirect( ps->dwBNodeIdx );
    }

    gint32 RemoveChildDirect (
        guint32 dwBNodeIdx, BNodeUPtr& pChild )
    {
        auto& oMap = GetChildMap();
        auto itr = oMap.find( dwBNodeIdx );
        if( itr == oMap.end() )
            return -ENOENT;
        pChild = std::move( *itr );
        pChild->SetParent( nullptr );
        return STATUS_SUCCESS;
    }

    gint32 RemoveChild(
        guint32 idx, BNodeUPtr& pChild )
    {
        if( idx >= GetChildCount() )
            return -EINVAL;
        if( IsLeaf() )
            return -ENOTSUP;
        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];
        return RemoveChildDirect(
            ps->dwBNodeIdx, pChild );
    }

    gint32 AddChildDirect(
        guint32 dwBNodeIdx, BNodeUPtr& pChild )
    {
        auto& oMap = GetChildMap();
        auto itr = oMap.find( dwBNodeIdx );
        if( itr != oMap.end() )
            return -EEXIST;
        itr->reset( std::move( pChild ) );
        pChild->SetParent( this );
        return STATUS_SUCCESS;
    }

    gint32 AddChild(
        guint32 idx, BNodeUPtr& pChild )
    {
        if( idx >= GetChildCount() )
            return -EINVAL;
        if( IsLeaf() )
            return -ENOTSUP;
        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];

        return AddChildDirect(
            ps->dwBNodeIdx, pChild );
    }

    inline const KEYPTR_SLOT* GetSlot(
        gint32 idx ) const
    {
        if( idx >= GetChildCount() )
            return nullptr;
        return m_oBNodeStore.m_vecSlots[ idx ];
    }

    inline KEYPTR_SLOT* GetSlot( gint32 idx )
    {
        if( idx >= GetChildCount() )
            return nullptr;
        return m_oBNodeStore.m_vecSlots[ idx ];
    }


    gint32 InsertSlotAt(
        gint32 idx, KEYPTR_SLOT* pKey );

    gint32 RemoveSlotAt(
        gint32 idx, KEYPTR_SLOT& oKey );

    gint32 AppendSlot( KEYPTR_SLOT* pKey );

    void InsertNonFull(
        KEYPTR_SLOT* pKey );

    gint32 Split(
        CBPlusNode* pParent,
        guint32 dwSlotIdx );

    gint32 StealFromLeft( gint32 i );
    gint32 StealFromRight( gint32 i );

    // get the dwIdx'th predecessor of this current
    // BNode
    const char* GetPredKey(
        guint32 dwIdx = 0 ) const;

    // get the dwIdx'th successor
    const char* GetSuccKey(
        guint32 dwIdx = 0 ) const;

    inline guint32 GetChildCount()
    { return m_oBNodeStore.m_wNumPtrs; };
    
    inline guint32 GetFreeBNodeIdx() const
    {
        auto& o = m_oBNodeStore;
        return o.m_wFreeBNodeIdx;
    }

    inline void SetFreeBNodeIdx(
        guint32 dwBNodeIdx )
    {
        auto& o = m_oBNodeStore;
        o.m_wFreeBNodeIdx = dwBNodeIdx;
    }

    bool HasFreeBNode()
    {
        auto& o = m_oBNodeStore;
        return o.m_wFreeBNodeIdx !=
            INVALID_BNODE_IDX;
    }

    guint32 GetNextLeaf() const
    { return m_oBNodeStore.m_wNextLeaf; }

    void SetNextLeaf( guint32 dwBNodeIdx )
    {
        auto& o = m_oBNodeStore;
        o.m_wNextLeaf = ( guint16 )dwBNodeIdx;
    }

    gint32 BinSearch( const char* szKey,
        gint32 iOrigLower, gint32 iOrigUpper );

    gint32 Insert( KEYPTR_SLOT* pSlot );

};

struct FREE_BNODES
{
    guint16 m_wBNCount;
    bool m_bNextBNode = false;
    guint8 m_byReserved;
    guint16 m_arrFreeBNIdx[ 0 ];

    static guint16 GetMaxCount() const
    {
        return ( ( BNODE_SIZE - 4 ) *
            sizeof( guint16 ) );
    }
    guint16 GetCount() const
    { return m_wBNCount; };

    bool IsFull()
    { return m_wBNCount == GetMaxCount(); }

    bool IsEmpty()
    { return m_wBNCount == 0; }

    guint16 GetLastBNodeIdx()
    {
        if( m_wBNCount > 0 )
            return m_arrFreeBNIdx[ m_wBNCount - 1 ];
        return INVALID_BNODE_IDX;
    }
    gint32 PushFreeBNode( guint16 wBNodeIdx )
    {
        if( IsFull() )
            return -ENOMEM;
        m_arrFreeBNIdx[ m_wBNCount++ ] =
            wBNodeIdx;
        return 0;
    }

    gint32 PopFreeBNode( guint16& wBNodeIdx )
    {
        if( IsEmpty() )
            return -ENOENT;
        wBNodeIdx = m_arrFreeBNIdx[ --m_wBNCount ];
        return 0;
    }

    gint32 hton();
    gint32 ntoh();

}__attribute__((aligned (8)));

struct CFreeBNodePool :
    public ISynchronize
{
    CDirImage* m_pDir = nullptr;

    CFreeBNodePool(
        CDirImage* pDir, guint32 dwFirstFree ):
        m_pDir( pDir )
    {}

    std::vector< std::pair< guint32, BufPtr > > m_vecFreeBNodes;

    gint32 InitPoolStore(
        guint32 dwBNodeIdx );

    gint32 PutFreeBNode( guint32 dwBNodeIdx );
    gint32 GetFreeBNode( guint32& dwBNodeIdx );
    gint32 ReleaseFreeBNode( guint32 dwBNodeIdx );

    gint32 Format() override
    { return 0; }

    gint32 Reload() override;
    gint32 Flush() override;
};

struct CDirImage : 
    public CFileImage
{
    typedef CFileImage super;
    BNodeUPtr m_pRootNode;
    std::unique_ptr< CFreeBNodePool > m_pFreePool;
    ChildMap m_mapChilds;
    FileMap m_mapFiles;

    CDirImage( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CDirImage ) ); }

    gint32 Format() override;
    gint32 Reload() override;
    gint32 Flush() override;

    inline const ChildMap& GetChildMap() const
    { return m_mapChilds; }

    inline ChildMap& GetChildMap()
    { return m_mapChilds; }

    inline const FileMap& GetFileMap() const
    { return m_mapFiles; }

    inline FileMap& GetFileMap()
    { return m_mapFiles; }

    // B+ tree methods
    // Search through this dir to find the key, if
    // found, return pFile, otherwise, return the leaf
    // node pNode where the key should be inserted.
    bool Search(
        const char* szKey,
        FImgSPtr& pFile,
        CBPlusNode*& pNode ) const;

    gint32 Split(
        CBPlusNode* pParent,
        gint32 index,  
        CBPlusNode* pNode );

    gint32 PutFreeBNode(
        CBPlusNode* pFreeNode );

    gint32 GetFreeBNode( bool bLeaf,
        BNodeUPtr& pNewNode );

    gint32 ReleaseFreeBNode( guint32 dwBNodeIdx );
    {
        return m_pRootNode->ReleaseFreeBNode(
            dwBNodeIdx );
    }

    gint32 FreeRootBNode( BNodeUPtr& pRoot );
    gint32 ReplaceRootBNode(
        BNodeUPtr& pNew, BNodeUPtr& pOld );

    guint32 GetHeadFreeBNode()
    { return m_pRootNode->GetFreeBNodeIdx(); }

    void SetHeadFreeBNode()
    { return m_pRootNode->SetFreeBNodeIdx(); }

    gint32 CreateFile( const char* szName );
    gint32 CreateDir( const char* szName );
    gint32 CreateLink( const char* szName,
        const char* szLink );
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
    { return m_pFileImage->IsRootDir(); }

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
    std::hashmap< HANDLE, FileSPtr > m_mapOpenFiles;

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

    gint32 MakeDir( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    HANDLE OpenFile( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32  CloseFile( HANDLE hFile );

    gint32  FindFile( const stdstr& strPath,
        CAccessContext* pac = nullptr ) const;

    gint32  RemoveFile( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32  RemoveDir( const stdstr& strPath,
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
