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
#include <unordered_set>

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
#define DEFAULT_PAGE_SIZE   4096
#define BLOCK_SIZE          ( GetBlockSize() )
#define REGFS_PAGE_SIZE     ( GetPageSize() )

#define SUPER_BLOCK_SIZE    ( BLOCK_SIZE )

#define BLOCK_SHIFT         ( find_shift( BLOCK_SIZE ) )
#define BLOCK_MASK         ( BLOCK_SIZE - 1 )

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
    ( find_shift( BLOCKS_PER_GROUP_FULL ) )

// get the group idx from the block index
#define GROUP_INDEX( _blk_idx ) \
    ( ( _blk_idx >> GROUP_INDEX_SHIFT ) & GROUP_IDX_MASK ) 

// get the logical address of a group with a block
// index from inode
#define GROUP_START( _blk_idx ) \
    ( GROUP_INDEX( _blk_idx )  * GROUP_SIZE )

// get the logical address of a group's data section
// with a block index from inode
#define GROUP_DATA_START( _blk_idx ) \
    ( GROUP_INDEX( _blk_idx )  * GROUP_SIZE + \
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
    ( _idx_ << find_shift( BNODE_SIZE ) )

#define INVALID_BNODE_IDX  ( USHRT_MAX )
#define INVALID_BLOCK_IDX  ( USHRT_MAX )

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
    if( (_p_)->IsStopped() ) \
    { ret = ERROR_STATE; break; }

#define WRITE_LOCK( _p_ ) \
    CWriteLock _oLock_( (_p_)->GetLock() ); \
    if( (_p_)->IsStoppedNoLock() ) \
    { ret = ERROR_STATE; break; }

#define DIR_WRITE_LOCK( _p_ ) \
    CDirWriteLock _oLock_( _p_ ); \
    if( (_p_)->IsStoppedNoLock() ) \
    { ret = ERROR_STATE; break; }

#define UNLOCK( _p_ ) \
    _oLock_.Unlock()

#define RFHANDLE    guint64
#define FLAG_FLUSH_CHILD 0x01
#define FLAG_FLUSH_DATA 0x02
#define FLAG_FLUSH_INODE 0x04
#define FLAG_FLUSH_SINGLE_BNODE 0x8
#define FLAG_FLUSH_DEFAULT \
    ( FLAG_FLUSH_DATA | FLAG_FLUSH_INODE )

#define UID_ADMIN       10000

#define GID_ADMIN       80000
#define GID_DEFAULT     80001

#define MAX_FS_SIZE     \
    ( ( BLOCKS_PER_GROUP * BLKGRP_NUMBER ) + SUPER_BLOCK_SIZE + GRPBMP_BLKNUM  )

#define BLKIDX_BEYOND_MAX( _blkidx_  ) \
    ( ( _blkidx_ ) >= BLKGRP_NUMBER_FULL * BLOCKS_PER_GROUP_FULL )

#define CACHE_LIFE_CYCLE 60
#define CACHE_MAX_BLOCKS BLOCKS_PER_GROUP_FULL 

namespace rpcf{

extern bool g_bSafeMode;

inline bool IsSafeMode()
{ return g_bSafeMode; }

inline void SetSafeMode( bool bSafe )
{ g_bSafeMode = bSafe; }

extern guint32 g_dwCacheLife;

inline guint32 GetCacheLifeCycle()
{ return g_dwCacheLife; }

inline void SetCacheLifeCycle( guint32 dwTimeSec )
{ g_dwCacheLife = dwTimeSec; }

struct ISynchronize
{
    virtual gint32 Flush( guint32 dwFlags ) = 0;
    virtual gint32 Format() = 0;
    virtual gint32 Reload() = 0;
};

class CBlockAllocator;

struct CSuperBlock : public ISynchronize
{
    guint32     m_dwMagic =  *( guint32* )"regi";
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
{ return g_dwRegFsPageSize; }

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

    inline bool IsEmpty() const
    {
        return BLOCKS_PER_GROUP - BLKBMP_BLKNUM ==
            GetFreeCount();
    }

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

    gint32 IsBlockFree( guint32 dwBlkIdx ) const
    {
        return m_pBlockBitmap->IsBlockFree(
            dwBlkIdx );
    }

    CBlockBitmap* GetBlkBmp()
    { return m_pBlockBitmap.get(); };

    const CBlockBitmap* GetBlkBmp() const
    { return m_pBlockBitmap.get(); };

};

typedef CAutoPtr< clsid( CBlockAllocator ), CBlockAllocator > AllocPtr;

using BlkGrpUPtr = typename std::unique_ptr< CBlockGroup >;
struct CGroupBitmap :
    public ISynchronize
{
    guint8* m_arrBytes; //[ BLKGRP_NUMBER_FULL ];
    BufPtr m_pBytes;
    guint16 m_wFreeCount;
    CBlockAllocator* m_pAlloc;
    inline guint32 GetFreeCount() const
    { return m_wFreeCount; }

    inline guint32 GetAllocCount() const
    { return BLKGRP_NUMBER - m_wFreeCount; }

    CGroupBitmap( CBlockAllocator* pAlloc );

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

struct CONTBLKS
{
    union{
        guint32 first;
        guint32 dwBlkIdx;
    };
    union{
        guint32 second;
        guint32 dwCount;
        guint32 dwBlkIdxEnd;
    };
    union{
        guint32 third;
        guint32 dwOff; // buffer offset in block index
    };
    guint32 dwKey = ( guint32 )-1;

    CONTBLKS( guint32 a1, guint32 a2,
        guint32 a3 = 0, guint32 a4 = 0 )
    { first = a1, second = a2, third = a3, dwKey = a4; }
};

using DirtyBlks = typename std::map< guint32, BufPtr >;

class CBlockAllocator :
    public CObjBase,
    public ISynchronize
{
    mutable stdrmutex   m_oLock;
    gint32              m_iFd = -1;

    SblkUPtr            m_pSuperBlock;
    GrpBmpUPtr          m_pGroupBitmap;
    std::map< guint32, BlkGrpUPtr > m_mapBlkGrps;

    // cache related members
    DirtyBlks           m_mapDirtyBlks;
    guint32             m_dwDirtyBlkCount = 0;
    guint32             m_dwCacheAge = 0;
    guint32             m_dwTransCount = 0;
    timespec            m_oStartTime = {0};
    bool                m_bStopped = false;

    gint32 ReadWriteBlocks(
        bool bRead, const guint32* pBlocks,
        guint32 dwNumBlocks, guint8* pBuf,
        bool bContigous = false );

    gint32 ReadWriteFile(
        char* pBuf, guint32 dwSize,
        guint32 dwOff, bool bRead );

    gint32 CacheBlocks(
        const guint32* pBlocks,
        guint32 dwNumBlocks, guint8* pBuf,
        bool bContigous = false );

    gint32 ReadCache( const guint32* pBlocks,
        guint32 dwNumBlocks, guint8* pBuf,
        bool bContigous = false );

    gint32 CommitCache();

    public:
    CBlockAllocator( const IConfigDb* pCfg );
    ~CBlockAllocator();

    inline stdrmutex& GetLock() const
    { return m_oLock; }

    gint32 CheckAndCommit( bool bEntering = true );
    inline gint32 IncTransactCount()
    { return ++m_dwTransCount; }

    inline gint32 DecTransactCount()
    { return --m_dwTransCount; }

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

    gint32 GetFd() const
    { return m_iFd; }

    CGroupBitmap* GetGroupBitmap() const
    { return m_pGroupBitmap.get(); }

    std::map< guint32, BlkGrpUPtr >& GetBlkGrps()
    { return m_mapBlkGrps; }
    const std::map< guint32, BlkGrpUPtr >& GetBlkGrps() const
    { return m_mapBlkGrps; }

    inline void SetStopped()
    {
        CStdRMutex oLock( GetLock() );
        m_bStopped = true;
    }

    inline bool IsStopped() 
    {
        CStdRMutex oLock( GetLock() );
        return m_bStopped;
    }

};

struct BATransact
{
    AllocPtr m_pAlloc;
    BATransact( AllocPtr& pAlloc )
    {
        if( !IsSafeMode() )
            return;
        m_pAlloc = pAlloc;
        CStdRMutex oLock( pAlloc->GetLock() );
        pAlloc->CheckAndCommit();
        pAlloc->IncTransactCount();
    }
    ~BATransact()
    {
        if( !IsSafeMode() || m_pAlloc.IsEmpty() )
            return;
        CStdRMutex oLock( m_pAlloc->GetLock() );
        m_pAlloc->DecTransactCount();
        m_pAlloc->CheckAndCommit( false );
    }
};


#define VALUE_SIZE 95

#if BUILD_64==1
struct timespec32
{
    guint32 tv_sec;
    guint32 tv_nsec;
    operator timespec() const
    {
        timespec a;
        a.tv_sec = tv_sec;
        a.tv_nsec = tv_nsec;
        return a;
    }
    timespec32 operator=( const timespec& tv )
    {
        tv_sec = tv.tv_sec;
        tv_nsec = tv.tv_nsec;
        return *this;
    }
};
#endif

struct RegFSInode
{
    // file size in bytes
    guint32     m_dwSize;
#if BUILD_64==0
    // time of last modification.
    timespec    m_mtime;
    // time of last access.
    timespec    m_atime;
    // time of creation
    timespec    m_ctime;
#else
    // time of last modification.
    timespec32    m_mtime;
    // time of last access.
    timespec32    m_atime;
    // time of creation
    timespec32    m_ctime;
#endif

    // file type
    mode_t     m_dwMode;
    // uid
    uid_t       m_wuid;
    // gid
    gid_t       m_wgid;

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
    guint32     m_dwRootBNode;

    gint32 Serialize( BufPtr& pBuf,
        const Variant& oVar ) const;

} __attribute__((aligned (4)));

#define BNODE_SIZE ( REGFS_PAGE_SIZE )
#define BNODE_BLKNUM ( BNODE_SIZE / BLOCK_SIZE )

#define REGFS_NAME_LENGTH 96

// #define DIR_ENTRY_SIZE 128

#define MAX_PTRS_PER_NODE \
    ( BNODE_SIZE / sizeof( KEYPTR_SLOT ) - 2 )

#define MAX_KEYS_PER_NODE ( MAX_PTRS_PER_NODE - 1 )

#define LEAST_NUM_CHILD ( ( MAX_PTRS_PER_NODE + 1 ) >> 1 )
#define LEASE_NUM_KEY ( LEAST_NUM_CHILD - 1 )

#define DIRECT_BLOCKS 13

#define INDIRECT_BLOCKS ( BLOCK_SIZE >> 2 )

#define SEC_INDIRECT_BLOCKS \
    ({ int _blks_ = INDIRECT_BLOCKS; _blks_*_blks_;}) 

#define MAX_FILE_SIZE ( ( 13 + INDIRECT_BLOCKS + \
    SEC_INDIRECT_BLOCKS ) * BLOCK_SIZE )

#define BEYOND_MAX_LIMIT( _offset_ ) \
    ( ( _offset_ ) >= MAX_FILE_SIZE )

#define INDIRECT_BLOCK_START ( DIRECT_BLOCKS * BLOCK_SIZE )

#define SEC_INDIRECT_BLOCK_START \
    ( INDIRECT_BLOCK_START + INDIRECT_BLOCKS * BLOCK_SIZE )

#define WITHIN_DIRECT_BLOCK( _offset_ ) \
    ( ( _offset_ ) < INDIRECT_BLOCK_START )

#define WITHIN_INDIRECT_BLOCK( _offset_ ) \
    ( ( _offset_ ) >= INDIRECT_BLOCK_START && \
     ( _offset_ ) < SEC_INDIRECT_BLOCK_START  )

// secondary indirect mapping block
#define WITHIN_SEC_INDIRECT_BLOCK( _offset_ ) \
    ( ( _offset_ ) >= SEC_INDIRECT_BLOCK_START  && \
     ( _offset_ ) < MAX_FILE_SIZE )

#define BLKIDX_PER_TABLE ( BLOCK_SIZE >> 2 )

#define BLKIDX_PER_TABLE_SHIFT \
    ( find_shift( BLOCK_SIZE >> 2 ) )

#define BLKIDX_PER_TABLE_MASK \
    ( BLKIDX_PER_TABLE - 1 )

#define BIT_IDX  13
#define BITD_IDX  14

#define INDIRECT_BLK_IDX( _offset_ ) \
    ( ( ( _offset_ ) - INDIRECT_BLOCK_START ) >> BLOCK_SHIFT )

#define SEC_BIT_IDX( _offset_ ) \
    ( ( ( ( _offset_ ) - SEC_INDIRECT_BLOCK_START ) >> \
        ( BLOCK_SHIFT + BLKIDX_PER_TABLE_SHIFT ) ) & \
        BLKIDX_PER_TABLE_MASK )

#define SEC_BLKIDX_IDX( _offset_ ) \
    ( ( ( ( _offset_ ) - SEC_INDIRECT_BLOCK_START ) >> \
        BLOCK_SHIFT ) & BLKIDX_PER_TABLE_MASK )

#define INVALID_UID USHRT_MAX
#define INVALID_GID USHRT_MAX
struct CAccessContext
{
    uid_t dwUid = INVALID_UID;
    gid_t dwGid = INVALID_GID;
    IntSetPtr pGids = nullptr;
    inline bool IsInitialized() const
    {
        return ( dwUid != INVALID_UID );
    }
    inline bool IsMemberOf( gint32 gid ) const
    {
        if( dwGid == gid )
            return true;
        if( !pGids.IsEmpty() &&
            (*pGids)().find( gid ) != (*pGids)().end() )
            return true;
        return false;
    }
};

typedef enum : guint8
{
    ftRegular = 1,
    ftDirectory = 2,
    ftLink = 3
} EnumFileType;

class CFileImage;
typedef CAutoPtr< clsid( Invalid ), CFileImage > FImgSPtr;

class CDirImage;

struct CFileImage : 
    public CObjBase,
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
    guint32 m_dwOpenCount = 0;
    CDirImage* m_pParentDir = nullptr;

    RegFSInode  m_oInodeStore;
    Variant     m_oValue;
    EnumIfState m_dwState = stateStarted;
    std::atomic< bool > m_bRemoved = {false};

    stdstr      m_strName;

    mutable CSharedLock m_oLock;
    inline CSharedLock& GetLock() const
    { return m_oLock; }

    inline void SetState( guint32 dwState )
    {
        CStdRMutex oLock( GetExclLock() );
        m_dwState = ( EnumIfState )dwState;
    }

    inline void SetName( const stdstr& strName )
    { m_strName = strName; }

    inline const stdstr& GetName() const
    { return m_strName; }

    inline guint32 GetState() const
    {
        CStdRMutex oLock( GetExclLock() );
        return ( guint32 )m_dwState;
    }

    inline bool IsStopped() const
    {
        CStdRMutex oLock( GetExclLock() );
        return m_dwState == stateStopped;
    }

    inline bool IsStoppedNoLock() const
    { return m_dwState == stateStopped; }

    inline bool IsStopping()
    {
        CStdRMutex oLock( GetExclLock() );
        return m_dwState == stateStopping;
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

    inline void UpdateMtime()
    {
        timespec mtime;
        clock_gettime( CLOCK_REALTIME, &mtime );
        CStdRMutex oLock( GetExclLock() );
        this->m_oInodeStore.m_mtime = mtime;
        this->m_oInodeStore.m_atime = mtime;
        this->m_oInodeStore.m_ctime = mtime;
    }

    inline void UpdateCtime()
    {
        timespec ctime;
        clock_gettime( CLOCK_REALTIME, &ctime );
        CStdRMutex oLock( GetExclLock() );
        this->m_oInodeStore.m_ctime = ctime;
    }

    inline void UpdateAtime()
    {
        timespec atime;
        clock_gettime( CLOCK_REALTIME, &atime );
        CStdRMutex oLock( GetExclLock() );
        this->m_oInodeStore.m_atime = atime;
    }

    static gint32 Create(
        EnumFileType byType, FImgSPtr& pFile,
        AllocPtr& pAlloc, guint32 dwInodeIdx,
        guint32 dwParentInode,
        FImgSPtr pDir );

    gint32 Flush( guint32 dwFlags =
        FLAG_FLUSH_DEFAULT ) override;
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

    virtual gint32 WriteFile( guint32 dwOff,
        guint32& dwSize, guint8* pBuf );

    gint32 WriteFileNoLock( guint32 dwOff,
        guint32& dwSize, guint8* pBuf );

    gint32 ReadValue( Variant& oVar ) const;
    gint32 WriteValue( const Variant& oVar );

    gint32 TruncBlkDirect( guint32 lablkidx );
    gint32 TruncBlkIndirect( guint32 lablkidx );
    gint32 TruncBlkSecIndirect( guint32 lablkidx );

    gint32 TruncateNoLock( guint32 dwOff );
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


    gint32 CheckAccess( mode_t dwMode,
        const CAccessContext* pac = nullptr ) const;

    gint32 GetAttr( struct stat& stBuf );
    void SetTimes( const struct timespec tv[ 2 ] );
    gint32 FreeBlocks();
    gint32 FreeBlocksNoLock();

    inline CDirImage* GetParentDir()
    { return m_pParentDir; }

    inline const CDirImage* GetParentDir() const
    { return m_pParentDir; }

    virtual EnumFileType GetType() const
    { return ftRegular; }
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
    gint32 WriteFile( guint32 dwOff,
        guint32& dwSize, guint8* pBuf ) override;
    EnumFileType GetType() const override final
    { return ftLink; }
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
using BNodeUPtr = typename std::shared_ptr< CBPlusNode >; 
using ChildMap = typename std::hashmap< guint32, BNodeUPtr >;
using FileMap = typename std::hashmap< guint32, FImgSPtr >;

struct CFreeBNodePool :
    public ISynchronize
{
    CDirImage* m_pDir = nullptr;
    CFreeBNodePool( CDirImage* pDir ):
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
    gint32 FlushSingleBNode( guint32 dwBNodeIdx );
};

struct CDirImage : 
    public CFileImage
{
    typedef CFileImage super;
    BNodeUPtr m_pRootNode;
    std::unique_ptr< CFreeBNodePool > m_pFreePool;
    ChildMap m_mapChilds;
    FileMap m_mapFiles;
    std::set< guint32 > m_setDirtyNodes;

    CDirImage( const IConfigDb* pCfg );

    gint32 Format() override;
    gint32 Reload() override;
    gint32 Flush( guint32 dwFlags =
        FLAG_FLUSH_DEFAULT ) override;

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

    gint32 SearchNoLock( const char* szKey,
        FImgSPtr& pFile, CBPlusNode*& pNode );

    // B+ tree methods
    // Search through this dir to find the key, if
    // found, return pFile, otherwise, return the leaf
    // node pNode where the key should be inserted.
    gint32 Search( const char* szKey,
        FImgSPtr& pFile );

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
    guint32 GetHeadFreeBNode() const;
    void SetHeadFreeBNode( guint32 dwBNodeIdx );

    gint32 InsertFile(
        const char* szName, FImgSPtr& pImg );

    gint32 CreateFile( const char* szName,
        EnumFileType iType, FImgSPtr& pImg );

    gint32 CreateFile( const char* szName,
        mode_t dwMode, FImgSPtr& pImg );

    gint32 CreateDir( const char* szName,
        mode_t dwMode, FImgSPtr& pImg );

    gint32 CreateLink( const char* szName,
        const char* szLink, FImgSPtr& pImg );

    gint32 RemoveFileNoFree(
        const char* szKey, FImgSPtr& pFile,
        bool bForce = false );

    gint32 RemoveFile( const char* szName );

    gint32 Rename( const char* szFrom,
        const char* szTo);
    gint32 ListDir(
        std::vector< KEYPTR_SLOT >& vecDirEnt );

    gint32 UnloadFile( const char* szName );
    gint32 UnloadDirImage();

    gint32 PrintBNode();
    gint32 PrintBNodeNoLock();

    EnumFileType GetType() const override final
    { return ftDirectory; }

    void SetDirty( guint32 dwBNodeIdx );
    gint32 CommitDirtyNodes();
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

    inline const CBPlusNode* GetParent() const
    { return m_pParent; }

    inline CBPlusNode* GetParent()
    { return m_pParent; }

    bool IsRoot() const
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

    inline guint32 GetChildCount() const
    { return m_oBNodeStore.m_wNumPtrs; }

    void SetChildCount( guint32 dwBNode )
    {
        m_oBNodeStore.m_wNumPtrs =
            ( guint16 )dwBNode;
    }

    const char* GetKey( gint32 idx ) const
    {
        if( idx >= ( gint32 )GetKeyCount() )
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
        if( idx >= ( gint32 )GetChildCount() )
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
        auto& mapFiles = GetFileMap();
        auto itr = mapFiles.find( dwInodeIdx );
        if( itr == mapFiles.end() )
            return -ENOENT;
        pFile = itr->second;
        mapFiles.erase( itr );
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
        const char* szKey, FImgSPtr& pFile,
        bool bForce = false );

    gint32 AddFileDirect(
        guint32 dwInodeIdx, FImgSPtr& pFile )
    {
        auto& mapFiles = GetFileMap();
        auto itr = mapFiles.find( dwInodeIdx );
        if( itr != mapFiles.end() )
            return -EEXIST;
        if( pFile.IsEmpty() )
        {
            DebugPrint( -EFAULT, "Error the "
            "file object to add is empty" );
            return -EFAULT;
        }
        mapFiles[ dwInodeIdx ] = pFile;
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
        if( idx >= ( gint32 )GetChildCount() ||
            idx < 0 )
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
        itr->second->SetParent( nullptr );
        pChild = itr->second;
        oMap.erase( itr );
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
        if( this != pChild.get() )
            pChild->SetParent( this );
        oMap[ dwBNodeIdx ] = pChild;
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

        if( pChild->GetBNodeIndex() !=
            ps->dwBNodeIdx )
        {
            DebugPrint( -EINVAL, "Error the "
                "child to add is not the "
                "expected one" );
            return -EINVAL;
        }

        return AddChildDirect(
            ps->dwBNodeIdx, pChild );
    }

    inline const KEYPTR_SLOT* GetSlot(
        gint32 idx ) const
    {
        if( IsRoot() )
        {
            if( IsLeaf() && idx >
                ( gint32 )MAX_KEYS_PER_NODE )
                return nullptr;
            if( !IsLeaf() && idx >
                ( gint32 )MAX_PTRS_PER_NODE )
                return nullptr;
            return m_oBNodeStore.m_vecSlots[ idx ];
        }
        if( IsLeaf() &&
            idx > ( gint32 )GetKeyCount() )
            return nullptr;
        if( !IsLeaf() &&
            idx > ( gint32 )GetChildCount() )
            return nullptr;
        return m_oBNodeStore.m_vecSlots[ idx ];
    }

    inline KEYPTR_SLOT* GetSlot( gint32 idx )
    {
        if( IsRoot() )
        {
            if( IsLeaf() && idx > 
                ( gint32 )MAX_KEYS_PER_NODE )
                return nullptr;
            if( !IsLeaf() && idx >
                ( gint32 )MAX_PTRS_PER_NODE )
                return nullptr;
            return m_oBNodeStore.m_vecSlots[ idx ];
        }
        if( ( IsLeaf() && idx >
            ( gint32 )GetKeyCount() ) ||
            ( !IsLeaf() && idx >
            ( gint32 )GetChildCount() ) )
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

    gint32 InsertOnly( KEYPTR_SLOT* pKey );

    gint32 Split( guint32 dwParentSlotIdx );

    gint32 StealFromLeft( gint32 i );
    gint32 StealFromRight( gint32 i );

    // get the dwIdx'th predecessor of this current
    // BNode
    const char* GetPredKey(
        guint32 dwIdx = 0 ) const;

    // get the dwIdx'th successor
    const char* GetSuccKey(
        guint32 dwIdx = 0 ) const;

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
    gint32 ShiftKeyPtr( guint32 dwIdx );

    gint32 PrintTree(
        std::vector< std::pair< guint32, stdstr > >& vecLines,
        guint32 dwLevel );
    gint32 PrintTree();
};

struct FREE_BNODES
{
    guint16 m_wBNCount = 0;
    bool m_bNextBNode = false;
    guint8 m_byReserved = 0;
    guint16 m_arrFreeBNIdx[ 0 ];

    static guint16 GetMaxCount()
    { return ( ( BNODE_SIZE - 4 ) >> 1 ); }

    guint16 GetCount() const
    { return m_wBNCount; };

    bool IsFull()
    { return m_wBNCount == GetMaxCount(); }

    bool IsEmpty()
    { return m_wBNCount == 0; }

    guint16 GetLastBNodeIdx()
    {
        guint16* p = m_arrFreeBNIdx;
        if( m_wBNCount > 0 )
            return p[ m_wBNCount - 1 ];
        return INVALID_BNODE_IDX;
    }
    gint32 PushFreeBNode( guint16 wBNodeIdx )
    {
        if( IsFull() )
            return -ENOMEM;
        guint16* p = m_arrFreeBNIdx;
        p[ m_wBNCount++ ] = wBNodeIdx;
        return 0;
    }

    gint32 PopFreeBNode( guint16& wBNodeIdx )
    {
        if( IsEmpty() )
            return -ENOENT;
        guint16* p = m_arrFreeBNIdx;
        wBNodeIdx = p[ m_wBNCount ];
        p[ m_wBNCount ] = INVALID_BNODE_IDX;
        --m_wBNCount;
        return 0;
    }

    gint32 hton( guint8* pDest, guint32 dwSize );
    gint32 ntoh( guint8* pSrc, guint32 dwSize );

}__attribute__((aligned (8)));

class CDirWriteLock : public CWriteLock
{
    CDirImage* m_pDir; 
    public:
    typedef CWriteLock super;
    CDirWriteLock( CDirImage* pDir ) :
        super( pDir->GetLock() ),
        m_pDir( pDir )
    {}

    ~CDirWriteLock()
    {
        CWriteLock::~CWriteLock();
        if( m_pDir )
            m_pDir->CommitDirtyNodes();
    }
};

class COpenFileEntry;
typedef CAutoPtr< clsid( Invalid ), COpenFileEntry > FileSPtr;

struct COpenFileEntry :
    public CObjBase,
    public ISynchronize
{
    AllocPtr        m_pAlloc;
    stdstr          m_strPath;
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
        oCfg.GetStrProp( 2, m_strPath );
    }

    static gint32 Create(
        EnumFileType byType, FileSPtr& pOpenFile,
        FImgSPtr& pFile, AllocPtr& pAlloc,
        const stdstr& strFile );

    gint32 ReadFile( guint32& size,
        guint8* pBuf, guint32 dwOff = UINT_MAX );

    gint32 WriteFile( guint32& size,
        guint8* pBuf, guint32 dwOff = UINT_MAX );

    inline gint32 ReadValue( Variant& oVar )
    { return m_pFileImage->ReadValue( oVar ); }

    inline gint32 WriteValue( const Variant& oVar )
    { return m_pFileImage->WriteValue( oVar ); }

    gint32 Flush( guint32 dwFlags =
        FLAG_FLUSH_DEFAULT ) override;

    gint32 Format() override
    { return 0; }

    gint32 Reload() override
    { return 0; }

    virtual gint32 Truncate( guint32 dwOff );

    gint32 Open( guint32 dwFlags, CAccessContext* pac );

    gint32 Close();

    inline void SetFlags( guint32 dwFlags )
    {   m_dwFlags = dwFlags; }

    inline guint32 GetFlags() const
    {   return m_dwFlags; }

    inline void SetTimes( const struct timespec tv[ 2 ] )
    { m_pFileImage->SetTimes( tv ); }

    inline const stdstr& GetPath() const
    { return m_strPath; }

    inline FImgSPtr& GetImage()
    { return m_pFileImage; }

    inline const FImgSPtr& GetImage() const
    { return m_pFileImage; }
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
        std::vector< KEYPTR_SLOT >& vecDirEnt );
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
    bool        m_bFormat = false;

    gint32 CreateRootDir();
    gint32 OpenRootDir();

    mutable CSharedLock m_oLock;

    public:
    CRegistryFs( const IConfigDb* pCfg );
    ~CRegistryFs();
    gint32 Start() override;
    gint32 Stop() override;

    inline bool IsStopped() const
    {
        CStdRMutex oLock( GetExclLock() );
        return m_dwState == stateStopped;
    }

    inline bool IsStoppedNoLock() const
    { return m_dwState == stateStopped; }

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
        RFHANDLE& hFile,
        CAccessContext* pac = nullptr );

    gint32 MakeDir( const stdstr& strPath,
        mode_t dwMode,
        CAccessContext* pac = nullptr );

    gint32 GetFile( RFHANDLE hFile,
        FileSPtr& pFile );

    gint32 OpenFile( const stdstr& strPath,
        guint32 dwFlags, RFHANDLE& hFile,
        CAccessContext* pac = nullptr );

    gint32 CloseFileNoLock( RFHANDLE hFile );
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
        const stdstr& strPath, gid_t& gid,
        CAccessContext* pac = nullptr );

    gint32 GetUid(
        const stdstr& strPath, uid_t& uid,
        CAccessContext* pac = nullptr );

    gint32 SymLink( const stdstr& strSrcPath,
        const stdstr& strDestPath,
        CAccessContext* pac = nullptr );

    gint32 GetValue(
        const stdstr&, Variant& oVar,
        CAccessContext* pac = nullptr );

    gint32 SetValue( const stdstr&,
        const Variant& oVar,
        CAccessContext* pac = nullptr );

    gint32 Chmod(
        const stdstr& strPath, mode_t dwMode,
        CAccessContext* pac = nullptr );

    gint32 Chown(
        const stdstr& strPath,
        uid_t dwUid, gid_t dwGid,
        CAccessContext* pac = nullptr );

    gint32 ReadLink(
        const stdstr& strPath,
        char* buf, guint32& dwSize,
        CAccessContext* pac = nullptr );

    gint32 Rename(
        const stdstr& szFrom, const stdstr& szTo,
        CAccessContext* pac = nullptr );

    gint32 Flush( guint32 dwFlags =
        FLAG_FLUSH_DEFAULT ) override;

    gint32 Format() override;
    gint32 Reload() override;

    gint32 Access( const stdstr& strPath,
        guint32 dwFlags,
        CAccessContext* pac = nullptr );

    gint32 GetAttr( const stdstr& strPath,
        struct stat& stBuf,
        CAccessContext* pac = nullptr );

    gint32 ReadDir( RFHANDLE hDir,
        std::vector< KEYPTR_SLOT >& vecDirEnt );

    gint32 OpenDir( const stdstr& strPath,
        guint32 dwFlags, RFHANDLE& hDir,
        CAccessContext* pac = nullptr );

    gint32 GetParentDir(
        const stdstr& strPath, FImgSPtr& pDir,
        stdstr* pstrNormPath = nullptr,
        CAccessContext* pac = nullptr );

    gint32 GetParentDir(
        RFHANDLE hDir,
        const stdstr& strPath, FImgSPtr& pDir,
        stdstr* pstrNormPath = nullptr,
        CAccessContext* pac = nullptr );

    gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData ) override
    { return -ENOTSUP; }

    gint32 GetSize( RFHANDLE hFile,
        guint32 dwSize ) const;

    gint32 GetFd() const
    { return m_pAlloc->GetFd(); }

    gint32 GetFileImage(
        RFHANDLE hDir, const stdstr&,
        FImgSPtr& pFile, mode_t,
        CAccessContext* pac = nullptr );

    gint32 GetDirImage(
        RFHANDLE hDir, FImgSPtr& pDir );

    gint32 GetValue( RFHANDLE hDir,
        const stdstr&, Variant& oVar,
        CAccessContext* pac = nullptr );

    gint32 SetValue(
        RFHANDLE hDir, const stdstr&,
        const Variant& oVar,
        CAccessContext* pac = nullptr );

    gint32 GetAttr(
        RFHANDLE hDir, const stdstr&,
        struct stat& stBuf,
        CAccessContext* pac = nullptr );

    gint32 Access(
        RFHANDLE hDir, const stdstr&,
        guint32 dwFlags,
        CAccessContext* pac = nullptr );

    gint32 RemoveFile(
        RFHANDLE hDir, const stdstr&,
        CAccessContext* pac = nullptr );

    gint32 Truncate(
        const stdstr& strPath,
        guint32 dwsize = 0,
        CAccessContext* pac = nullptr );

    gint32 OpenFile(
        RFHANDLE hDir, const stdstr&,
        guint32 dwFlags,
        RFHANDLE& hFile,
        CAccessContext* pac = nullptr );

    gint32 SetGid(
        RFHANDLE hFile, gid_t wGid,
        CAccessContext* pac = nullptr );

    gint32 SetUid(
        RFHANDLE hFile, uid_t wUid,
        CAccessContext* pac = nullptr );

    gint32 GetGid(
        RFHANDLE hFile, gid_t& gid,
        CAccessContext* pac = nullptr );

    gint32 GetUid(
        RFHANDLE hFile, uid_t& uid,
        CAccessContext* pac = nullptr );

    gint32 CreateFile(
        RFHANDLE hDir, const stdstr& strFile,
        mode_t dwMode, guint32 dwFlags,
        RFHANDLE& hFile,
        CAccessContext* pac = nullptr );

    gint32 GetAttr( RFHANDLE hFile,
        struct stat& stBuf );

    gint32 GetPathFromHandle(
        RFHANDLE hFile, stdstr& strPath );

    AllocPtr GetAllocator() const
    { return m_pAlloc; }
};

typedef CAutoPtr< clsid( CRegistryFs ), CRegistryFs > RegFsPtr;

struct CFileHandle
{
    RFHANDLE m_hFile = INVALID_HANDLE;
    ObjPtr m_pFs;
    public:
    CFileHandle( ObjPtr pFs, RFHANDLE hFile )
    { m_hFile = hFile; m_pFs = pFs; }
    ~CFileHandle();
};

gint32 GetPathFromImg(
    CFileImage* pFile,
    stdstr& strPath );

gint32 GetPathFromImg(
    FImgSPtr& pFile,
    stdstr& strPath );

}
