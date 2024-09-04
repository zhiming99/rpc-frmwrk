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
#include "defines.h"
#include <arpa/inet.h>
// byte offset
// 0                             64
// -------------------------------
// bytes_in_blk|block_idx_grp| group_idx

#define find_shift( _x )                    ffs( _x )

#define SUPER_BLOCK_SIZE                    512
#define BLOCK_SHIFT                         9
#define BLOCK_SIZE                          ( 1 << BLOCK_SHIFT )
#define BLOCKS_PER_GROUP                    ( 64 * 1024 )
#define BLKBMP_BLKNUM                       ( BLOCKS_PER_GROUP / ( BLOCK_SIZE * 8 ) )

#define GROUP_SHIFT                         ( find_shift( PAGE_SIZE ) - 1 )
#define BLKGRP_NUMBER                       ( 1 << GROUP_SHIFT )
#define GRPBMP_BLKNUM                       ( BLKGRP_NUMBER / ( BLOCK_SIZE * 8 ) )
#define GRPBMP_SIZE                         ( BLKGRP_NUMBER / 8 )
#define GRPBMP_START                        ( SUPER_BLOCK_SIZE )
#define BLKBMP_START                        ( GRPBMP_START + GRPBMP_SIZE )
#define DATABLK_START                       ( BLKBMP_START + BLKBMP_BLKNUM * BLOCK_SIZE )
#define GROUP_SIZE                          ( BLOCKS_PER_GROUP * BLOCK_SIZE )

// group index in a block index
#define GROUP_IDX_SHIFT                     ( 1 << ( find_shift( GROUP_SIZE ) - 1 ) )

#define GROUP_OFFSET( _blk_idx )            ( DATABLK_START + ( _grp_idx )  * GROUP_SIZE )
#define GROUP_INDEX( _blk_idx )           ( ( _byte_addr >> ( BLOCK_SHIFT )
#define BLOCK_INDEX( _blk_idx )
#define BLOCK_OFFSET( _blk_idx ) 

template< int BLKSIZE >
struct CSuperBlock
{
    guint32 m_dwMagic = 
        htonl( *( guint32* )"regi" );
    guint32 m_dwVersion = 0x01;
};
