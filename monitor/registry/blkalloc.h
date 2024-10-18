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
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include "autoptr.h"
#include "ifstat.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

#define BLKBMP_SIZE \
    ( BLOCKS_PER_GROUP_FULL / BYTE_BITS )

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

#define INVALID_BNODE_IDX  ( USHRT_MAX )

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

#define READ_LOCK( _p_ ) \
    CReadLock _oLock_( (_p_)->GetLock() ); \
    if( (_p_)->GetState() == stateStopped ) \
    { ret = ERROR_STATE; break; }

#define WRITE_LOCK( _p_ ) \
    CWriteLock _oLock_( (_p_)->GetLock() ); \
    if( (_p_)->GetState() == stateStopped ) \
    { ret = ERROR_STATE; break; }

#define UNLOCK( _p_ ) \
    _oLock_.Unlock()

#define RFHANDLE    guint64
#define FLAG_FLUSH_CHILD 0x01
#define FLAG_FLUSH_DATAONLY 0x02

extern rpcf::ObjPtr g_pIoMgr;

namespace rpcf{

struct ISynchronize
{
    virtual gint32 Flush( guint32 dwFlags ) = 0;
    virtual gint32 Format() = 0;
    virtual gint32 Reload() = 0;
};

class CBlockAllocator;

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
    gint32 Flush( guint32 dwFlags = 0 ) override;
    gint32 Format() override;
    gint32 Reload() override;
} ;

using SblkUPtr = typename std::unique_ptr< CSuperBlock >;

extern guint32 g_dwBlockSize;
extern guint32 g_dwRegFsPageSize;

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
        m_pBytes->Resize(
            BLOCKS_PER_GROUP_FULL / BYTE_BITS );
        m_arrBytes = ( guint8* )m_pBytes->ptr();
    }

    gint32 FreeBlock( guint32 dwBlkIdx );

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

    gint32 Flush( guint32 dwFlags = 0 ) override;
    gint32 Format() override;
    gint32 Reload() override;
};

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

    gint32 Flush( guint32 dwFlags = 0 ) override
    { return m_pBlockBitmap->Flush( dwFlags ); }

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

    gint32 IsBlockFree( guint32 dwBlkIdx )
    {
        return m_pBlockBitmap->IsBlockFree(
            dwBlkIdx );
    }
};

typedef CAutoPtr< clsid( CBlockAllocator ), CBlockAllocator > AllocPtr;

using BlkGrpUPtr = typename std::unique_ptr< CBlockGroup >;
struct CGroupBitmap :
    public ISynchronize
{
    guint8* m_arrBytes; //[ BLKGRP_NUMBER_FULL ];
    BufPtr m_pBytes;
    guint16 m_wFreeCount;
    AllocPtr m_pAlloc;
    inline guint32 GetFreeCount() const
    { return m_wFreeCount; }

    inline guint32 GetAllocCount() const
    { return BLKGRP_NUMBER - m_wFreeCount; }

    CGroupBitmap( AllocPtr pAlloc ) :
        m_pAlloc( pAlloc )
    {
        m_pBytes.NewObj();
        m_pBytes->Resize( GRPBMP_SIZE );
        m_arrBytes = ( guint8* )m_pBytes->ptr();
    }

    gint32 FreeGroup( guint32 dwGrpIdx );
    gint32 AllocGroup( guint32& dwGrpIdx );
    gint32 InitBitmap();
    gint32 IsBlockGroupFree(
        guint32 dwGroupIdx ) const;

    gint32 Flush( guint32 dwFlags = 0 ) override;
    gint32 Format() override;
    gint32 Reload() override;
};

typedef std::unique_ptr< CGroupBitmap > GrpBmpUPtr;

typedef std::pair< guint32, guint32 > CONTBLKS;

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
    ~CBlockAllocator();

    inline stdrmutex& GetLock() const
    { return m_oLock; }

    gint32 Format() override;
    gint32 Flush( guint32 dwFlags = 0 ) override;
    gint32 Reload() override;

    gint32 SaveGroupBitmap(
        const char* pbmp, guint32 dwSize );

    gint32 LoadGroupBitmap(
        char* pbmp, guint32 dwSize );

    gint32 SaveSuperBlock(
        const char* pSb, guint32 dwSize );

    gint32 LoadSuperBlock(
        char* pSb, guint32 dwSize );

    gint32 FreeBlocks(
        const guint32* pvecBlocks,
        guint32 dwNumBlocks,
        bool bByteOrder = false );

    gint32 AllocBlocks( 
        guint32* pvecBlocks,
        guint32 dwNumBlocks );

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
        guint32 dwNumBlocks, guint8* pBuf,
        bool bContigous = false );

    inline gint32 IsBlockGroupFree(
        guint32 dwGroupIdx ) const
    {
        return m_pGroupBitmap->IsBlockGroupFree(
            dwGroupIdx );
    }

    gint32 IsBlockFree( guint32 dwBlkIdx );

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

#define VALUE_SIZE 95

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
    guint8      m_arrBuf[ VALUE_SIZE ];
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

typedef enum : guint8
{
    ftRegular = 1,
    ftDirectory = 2,
    ftLink = 3
} EnumFileType;

class CFileImage;
typedef CAutoPtr< clsid( Invalid ), CFileImage > FImgSPtr;

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
    guint32 m_dwOpenCount = 0;
    

    RegFSInode  m_oInodeStore;
    Variant     m_oValue;
    EnumIfState m_dwState = stateStarted;

    mutable CSharedLock m_oLock;
    inline CSharedLock& GetLock() const
    { return m_oLock; }

    inline void SetState( guint32 dwState )
    {
        CStdRMutex oLock( GetExclLock() );
        m_dwState = ( EnumIfState )dwState;
    }

    inline guint32 GetState() const
    {
        CStdRMutex oLock( GetExclLock() );
        return ( guint32 )m_dwState;
    }

    inline guint32 IncOpenCount()
    {
        CStdRMutex oLock( GetExclLock() );
        return ++m_dwOpenCount;
    }

    inline guint32 DecOpenCount()
    {
        CStdRMutex oLock( GetExclLock() );
        return --m_dwOpenCount;
    }

    inline guint32 GetOpenCount()
    {
        CStdRMutex oLock( GetExclLock() );
        return m_dwOpenCount;
    }

    AllocPtr GetAlloc() const
    { return m_pAlloc; }

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

    CFileImage( const IConfigDb* pCfg );

    static gint32 Create(
        EnumFileType byType, FImgSPtr& pFile,
        AllocPtr& pAlloc, guint32 dwInodeIdx,
        guint32 dwParentInode )
    {
        gint32 ret = 0;
        CParamList oParams;
        oParams.Push( ObjPtr( pAlloc ) );
        oParams.Push( dwInodeIdx );
        oParams.Push( dwParentInode );
        EnumClsid iClsid = clsid( Invalid );
        if( byType == ftRegular )
            iClsid = clsid( CFileImage );
        else if( byType == ftDirectory )
            iClsid = clsid( CDirImage );
        else if( byType == ftLink )
            iClsid = clsid( CLinkImage );
        else
        {
            ret = -ENOTSUP;
            DebugPrint( ret, "Error not a valid "
                "type of file to create" );
        }
        ret = pFile.NewObj( iClsid,
            oParams.GetCfg() );
        return ret;
    }

    gint32 Flush( guint32 dwFlags = 0 ) override;
    gint32 Format() override;
    gint32 Reload() override;

    gint32 ReadFile( guint32 dwOff,
        guint32& dwSize, guint8* pBuf );

    gint32 CollectBlocksForRead(
        guint32 dwOff, guint32 dwSize,
        std::vector< guint32 >& vecBlocks );

    gint32 CollectBlocksForWrite(
        guint32 dwOff, guint32 dwSize,
        std::vector< guint32 >& vecBlocks );

    gint32 WriteFile( guint32 dwOff,
        guint32& dwSize, guint8* pBuf );

    gint32 ReadValue( Variant& oVar ) const;
    gint32 WriteValue( const Variant& oVar );

    gint32 TruncBlkDirect( guint32 lablkidx );
    gint32 TruncBlkIndirect( guint32 lablkidx );
    gint32 TruncBlkSecIndirect( guint32 lablkidx );

    gint32 Truncate( guint32 dwOff );
    gint32 Extend( guint32 dwOff );

    void SetSize( guint32 dwSize )
    { m_oInodeStore.m_dwSize = dwSize; }

    inline guint32 GetSize() const
    { return m_oInodeStore.m_dwSize; }

    inline guint32 GetInodeIdx() const
    { return m_dwInodeIdx; }

    inline bool IsRootDir() const
    { return m_dwInodeIdx == ROOT_INODE_BLKIDX; }

    void SetGid( gid_t wGid );
    void SetUid( uid_t wUid );
    void SetMode( mode_t dwMode );

    gid_t GetGid() const;
    uid_t GetUid() const;
    mode_t GetMode() const;

    inline mode_t GetModeNoLock() const
    { return ( mode_t )m_oInodeStore.m_dwMode; }

    inline uid_t GetGidNoLock() const
    { return ( uid_t )m_oInodeStore.m_wgid; }

    inline uid_t GetUidNoLock() const
    { return ( uid_t )m_oInodeStore.m_wuid; }


    gint32 CheckAccess( mode_t dwMode );
    gint32 GetAttr( struct stat& stBuf );
};

struct CLinkImage :
    public CFileImage
{
    stdstr m_strLink;
    typedef CFileImage super;
    CLinkImage( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CLinkImage ) ); }

    gint32 ReadLink( guint8* buf, guint32& dwSize );

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
    };
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
class CDirImage;
using BNodeUPtr = typename std::unique_ptr< CBPlusNode >; 
using ChildMap = typename std::hashmap< guint32, BNodeUPtr >;
using FileMap = typename std::hashmap< guint32, FImgSPtr >;

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
    gint32 Flush( guint32 dwFlags = 0 ) override;
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
    gint32 Flush( guint32 dwFlags = 0 ) override;

    inline const ChildMap& GetChildMap() const
    { return m_mapChilds; }

    inline ChildMap& GetChildMap()
    { return m_mapChilds; }

    inline const FileMap& GetFileMap() const
    { return m_mapFiles; }

    inline FileMap& GetFileMap()
    { return m_mapFiles; }

    guint32 GetRootKeyCount() const;
    inline CBPlusNode* GetRootNode() const
    { return m_pRootNode.get(); }
    // B+ tree methods
    // Search through this dir to find the key, if
    // found, return pFile, otherwise, return the leaf
    // node pNode where the key should be inserted.
    bool Search(
        const char* szKey,
        FImgSPtr& pFile,
        CBPlusNode*& pNode );

    gint32 Split(
        CBPlusNode* pParent,
        gint32 index,  
        CBPlusNode* pNode );

    gint32 PutFreeBNode(
        CBPlusNode* pFreeNode );

    gint32 GetFreeBNode( bool bLeaf,
        BNodeUPtr& pNewNode );

    gint32 FreeRootBNode( BNodeUPtr& pRoot );
    gint32 ReplaceRootBNode(
        BNodeUPtr& pNew, BNodeUPtr& pOld );

    gint32 ReleaseFreeBNode( guint32 dwBNodeIdx );
    guint32 GetHeadFreeBNode();
    void SetHeadFreeBNode( guint32 dwBNodeIdx );

    gint32 CreateFile( const char* szName,
        EnumFileType iType, FImgSPtr& pImg );

    gint32 CreateFile( const char* szName,
        mode_t dwMode, FImgSPtr& pImg );

    gint32 CreateDir( const char* szName,
        mode_t dwMode, FImgSPtr& pImg );

    gint32 CreateLink( const char* szName,
        const char* szLink, FImgSPtr& pImg );

    gint32 RemoveFile( const char* szName );

    gint32 Rename( const char* szFrom,
        const char* szTo);
    gint32 ListDir(
        std::vector< KEYPTR_SLOT >& vecDirEnt ) const;

    guint32 GetFreeBNodeIdx() const;

    void SetFreeBNodeIdx(
        guint32 dwBNodeIdx );
};

struct CBPlusNode :
    public ISynchronize 
{
    CDirImage* m_pDir;
    RegFSBNode m_oBNodeStore;
    guint32  m_dwMaxSlots = MAX_PTRS_PER_NODE;
    CBPlusNode* m_pParent = nullptr;

    CBPlusNode( CDirImage* pFile,
        guint32 dwBNodeIdx ) :
        m_pDir( pFile )
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

    AllocPtr GetAlloc() const
    { return m_pDir->GetAlloc(); }

    gint32 Flush( guint32 dwFlags = 0 ) override;
    gint32 Format() override;
    gint32 Reload() override;

    guint32 GetParentSlotIdx()
    {
        if( IsRoot() )
            return UINT_MAX;
        CBPlusNode* pParent = GetParent();
        guint32 dwCount =
            pParent->GetChildCount();
        guint32 i = 0;
        for( ; i < dwCount; i++ )
        {
            if( pParent->GetSlot( i )->dwBNodeIdx ==
                this->GetBNodeIndex() )
                break;
        }
        if( i == dwCount )
            i = UINT_MAX;
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
    { m_oBNodeStore.m_bLeaf = bLeaf; }

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
        m_oBNodeStore.m_wNumPtrs =
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

    gint32 GetFileDirect(
        gint32 idx, FImgSPtr& pFile ) const 
    {
        FImgSPtr pEmpty;
        auto& mapFiles = GetFileMap();
        auto itr = mapFiles.find( idx );
        if( itr == mapFiles.end() )
            return -ENOENT;
        pFile = itr->second;
        return STATUS_SUCCESS;
    }

    gint32 GetFile(
        gint32 idx, FImgSPtr& pFile ) const 
    {
        FImgSPtr pEmpty;
        if( idx >= GetChildCount() )
            return -EINVAL;

        if( !IsLeaf() )
            return -EINVAL;

        KEYPTR_SLOT* ps = 
            m_oBNodeStore.m_vecSlots[ idx ];

        return GetFileDirect(
            ps->oLeaf.dwInodeIdx, pFile );
    }

    gint32 RemoveFileDirect (
        guint32 dwInodeIdx, FImgSPtr& pFile )
    {
        auto mapFiles = GetFileMap();
        auto itr = mapFiles.find( dwInodeIdx );
        if( itr == mapFiles.end() )
            return -ENOENT;
        pFile = itr->second;
        itr->second.Clear();
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
        itr->second = pFile;
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
            ps->oLeaf.dwInodeIdx, pFile );
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
        pChild->SetParent( nullptr );
        pChild = std::move( itr->second );
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
        pChild->SetParent( this );
        itr->second = std::move( pChild );
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

    gint32 MoveBNodeStore(
        CBPlusNode* pSrc, guint32 dwSrcOff,
        guint32 dwCount, guint32 dwDstOff );

    gint32 InsertNonFull( KEYPTR_SLOT* pKey );

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

    gint32 MergeChilds(
        gint32 iPred, gint32 iSucc );
    gint32 Rebalance();
    gint32 RebalanceChild( guint32 idx );
};

struct FREE_BNODES
{
    guint16 m_wBNCount;
    bool m_bNextBNode = false;
    guint8 m_byReserved;
    guint16 m_arrFreeBNIdx[ 0 ];

    static guint16 GetMaxCount()
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

    gint32 hton( guint8* pDest, guint32 dwSize );
    gint32 ntoh( guint8* pSrc, guint32 dwSize );

}__attribute__((aligned (8)));

class COpenFileEntry;
typedef CAutoPtr< clsid( Invalid ), COpenFileEntry > FileSPtr;

struct CAccessContext
{
    guint16 dwUid = USHRT_MAX;
    guint16 dwGid = USHRT_MAX;
};

struct COpenFileEntry :
    public ISynchronize,
    public CObjBase
{
    AllocPtr        m_pAlloc;
    stdstr          m_strFileName;
    FImgSPtr        m_pFileImage;
    CAccessContext  m_oUserAc;
    guint32         m_dwFlags;

    std::atomic< guint32 > m_dwPos = {0};
    mutable stdrmutex m_oLock;

    inline stdrmutex& GetLock() const
    { return m_oLock; }

    COpenFileEntry( const IConfigDb* pCfg )
    {
        SetClassId( clsid( COpenFileEntry ) );
        CCfgOpener oCfg( pCfg );
        ObjPtr pObj;
        oCfg.GetObjPtr( 0, pObj );
        m_pAlloc = pObj;
        oCfg.GetObjPtr( 1, pObj );
        m_pFileImage = pObj;
    }

    static gint32 Create(
        EnumFileType byType, FileSPtr& pOpenFile,
        FImgSPtr& pFile, AllocPtr& pAlloc )
    {
        gint32 ret = 0;
        CParamList oParams;
        oParams.Push( ObjPtr( pAlloc ) );
        oParams.Push( ObjPtr( pFile ) );
        EnumClsid iClsid = clsid( Invalid );
        if( byType == ftRegular )
            iClsid = clsid( COpenFileEntry );
        else if( byType == ftDirectory )
            iClsid = clsid( CDirFileEntry );
        else if( byType == ftLink )
            iClsid = clsid( CLinkFileEntry );
        else
        {
            ret = -ENOTSUP;
            DebugPrint( ret, "Error not a valid "
                "type of file to create" );
        }
        ret = pFile.NewObj( iClsid,
            oParams.GetCfg() );
        return ret;
    }

    gint32 ReadFile( guint32& size,
        guint8* pBuf, guint32 dwOff = UINT_MAX );

    gint32 WriteFile( guint32& size,
        guint8* pBuf, guint32 dwOff = UINT_MAX );

    inline gint32 ReadValue( Variant& oVar )
    { return m_pFileImage->ReadValue( oVar ); }

    inline gint32 WriteValue( const Variant& oVar )
    { return m_pFileImage->WriteValue( oVar ); }

    gint32 Flush( guint32 dwFlags = 0 ) override;

    gint32 Format() override
    { return 0; }

    gint32 Reload() override
    { return 0; }

    virtual gint32 Truncate( guint32 dwOff )
    { return m_pFileImage->Truncate( dwOff ); }

    inline gint32 Open( FileSPtr& pParent,
        CAccessContext* pac )
    {
        m_pFileImage->IncOpenCount();
        CStdRMutex oLock( GetLock() );
        if( pac )
            m_oUserAc = *pac;
        return 0;
    }

    inline gint32 Close()
    {
        gint32 ret = Flush();
        m_pFileImage->DecOpenCount();
        return ret;
    }
    inline void SetFlags( guint32 dwFlags )
    {   m_dwFlags = dwFlags; }

    inline guint32 GetFlags() const
    {   return m_dwFlags; }
};

struct CDirFileEntry :
    public COpenFileEntry
{
    typedef COpenFileEntry super;
    CDirFileEntry( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CDirFileEntry ) ); }

    inline bool IsRootDir() const
    { return m_pFileImage->IsRootDir(); }

    gint32 CreateFile( const stdstr& strName,
        guint32 dwFlags, guint32 dwMode );

    gint32  RemoveFile( const stdstr& strName );
    gint32  ListDir(
        std::vector< KEYPTR_SLOT >& vecDirEnt ) const
    {   
        CDirImage* pImg = m_pFileImage;
        return pImg->ListDir( vecDirEnt );
    }
};

struct CLinkFileEntry:
    public COpenFileEntry
{
    typedef COpenFileEntry super;
    CLinkFileEntry( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CLinkFileEntry ) ); }

    gint32 Truncate( guint32 dwOff ) override
    { return -ENOTSUP; }
};

class CRegistryFs :
    public IService,
    public ISynchronize
{
    ObjPtr      m_pIoMgr;
    AllocPtr    m_pAlloc;
    FileSPtr    m_pRootDir;    
    FImgSPtr    m_pRootImg;
    std::hashmap< guint64, FileSPtr > m_mapOpenFiles;
    mutable stdrmutex   m_oExclLock;
    EnumIfState m_dwState = stateStarted;

    gint32 CreateRootDir();
    gint32 OpenRootDir();

    mutable CSharedLock m_oLock;

    public:
    CRegistryFs( const IConfigDb* pCfg );
    gint32 Start() override;
    gint32 Stop() override;

    inline void SetState( guint32 dwState )
    {
        CStdRMutex oLock( GetExclLock() );
        m_dwState = ( EnumIfState )dwState;
    }

    inline guint32 GetState() const
    {
        CStdRMutex oLock( GetExclLock() );
        return ( guint32 )m_dwState;
    }

    inline CSharedLock& GetLock() const
    { return m_oLock; }

    inline stdrmutex& GetExclLock() const
    { return m_oExclLock; }

    static gint32 Namei(
        const stdstr& strPath,
        std::vector<stdstr>& vecNames );

    gint32 GetOpenFile(
        RFHANDLE hFile, FileSPtr& pFile );

    gint32 CreateFile( const stdstr& strPath,
        mode_t dwMode, guint32 dwFlags,
        RFHANDLE hFile,
        CAccessContext* pac = nullptr );

    gint32 MakeDir( const stdstr& strPath,
        mode_t dwMode,
        CAccessContext* pac = nullptr );

    gint32 GetFile( RFHANDLE hFile,
        FileSPtr& pFile );

    gint32 OpenFile( const stdstr& strPath,
        guint32 dwFlags, RFHANDLE& hFile,
        CAccessContext* pac = nullptr );

    gint32 CloseFile( RFHANDLE hFile );

    gint32 RemoveFile( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32 ReadFile( RFHANDLE hFile,
        char* buffer, guint32& dwSize,
        guint32 dwOff );

    gint32 WriteFile( RFHANDLE hFile,
        const char* buffer, guint32& dwSize,
        guint32 dwOff );

    gint32 Truncate(
        RFHANDLE hFile, guint32 dwOff );

    RFHANDLE OpenDir( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32 CloseDir( RFHANDLE hFile );

    gint32 RemoveDir( const stdstr& strPath,
        CAccessContext* pac = nullptr );

    gint32 SetGid(
        const stdstr& strPath, gid_t wGid,
        CAccessContext* pac = nullptr );

    gint32 SetUid(
        const stdstr& strPath, uid_t wUid,
        CAccessContext* pac = nullptr );

    gint32 GetGid(
        const stdstr& strPath, gid_t& gid );

    gint32 GetUid(
        const stdstr& strPath, uid_t& uid );

    gint32 SymLink( const stdstr& strSrcPath,
        const stdstr& strDestPath,
        CAccessContext* pac = nullptr );

    gint32 GetValue(
        const stdstr&, Variant& oVar );

    gint32 SetValue(
        const stdstr&, Variant& oVar );

    gint32 Chmod(
        const stdstr& strPath, mode_t dwMode,
        CAccessContext* pac = nullptr );

    gint32 Chown(
        const stdstr& strPath,
        uid_t dwUid, gid_t dwGid,
        CAccessContext* pac = nullptr );

    gint32 ReadLink(
        const stdstr& strPath,
        char* buf, guint32& dwSize );

    gint32 Rename(
        const stdstr& szFrom, const stdstr& szTo);

    gint32 Flush( guint32 dwFlags = 0 ) override;
    gint32 Format() override;
    gint32 Reload() override;

    gint32 Access( const stdstr& strPath,
        guint32 dwFlags );

    gint32 GetAttr( const stdstr& strPath,
        struct stat& stBuf );

    gint32 ReadDir( RFHANDLE hDir,
        std::vector< KEYPTR_SLOT > vecDirEnt );

    gint32 OpenDir( const stdstr& strPath,
        mode_t dwMode, RFHANDLE hDir,
        CAccessContext* pac = nullptr );

    gint32 GetParentDir(
        const stdstr& strPath, FImgSPtr& pDir );

    gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData ) override
    { return -ENOTSUP; }
};

typedef CAutoPtr< clsid( CRegistryFs ), CRegistryFs > RegFsPtr;

}
