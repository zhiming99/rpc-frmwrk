/*
 * =====================================================================================
 *
 *       Filename:  varlist.cpp
 *
 *    Description:  Implementation of a simple variant list for debug purpose 
 *
 *        Version:  1.0
 *        Created:  12/25/2023 08:50:14 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "varlist.h"
#include "configdb.h"

#define ITR_ROW_END( __idx ) \
    ( m_pParent->m_vecVarArrs[ (__idx ) ] + ROW_COUNT( (__idx) ) )

namespace rpcf{

CVarList::iterator::iterator( const iterator& rhs )
{
    m_iIdx = rhs.m_iIdx;
    m_pCurVar = rhs.m_pCurVar;
    m_pParent = rhs.m_pParent;
}

CVarList::iterator::iterator( iterator&& rhs )
{
    m_iIdx = rhs.m_iIdx;
    m_pCurVar = rhs.m_pCurVar;
    rhs.m_pCurVar = nullptr;
    rhs.m_iIdx = -1;
    m_pParent = rhs.m_pParent;
}

CVarList::iterator::iterator( gint32 iIdx, CFG_ELEM* pCurElem )
{
    m_iIdx = iIdx;
    m_pCurVar = pCurElem;
}

CVarList::iterator& CVarList::iterator::operator++()
{
    bool bFound = false;
    while( m_iIdx < m_pParent->m_dwRowCount )
    {
        while( m_pCurVar + 1 < ITR_ROW_END( m_iIdx ) )
        {
            m_pCurVar += 1;
            if( EMPTY_ENTRY( m_pCurVar ) )
                continue;
            bFound = true;
            break;
        }
        if( bFound )
            break;
        m_iIdx++;
        if( m_iIdx < m_pParent->m_dwRowCount )
            m_pCurVar = m_pParent->m_vecVarArrs[ m_iIdx ] - 1;
    }
    if( !bFound )
    {
        m_iIdx = -1;
        m_pCurVar = nullptr;
    }
    return *this;
}

CVarList::iterator& CVarList::iterator::operator++( int )
{
    bool bFound = false;
    while( m_iIdx < m_pParent->m_dwRowCount )
    {
        while( m_pCurVar + 1 < ITR_ROW_END( m_iIdx ) )
        {
            m_pCurVar += 1;
            if( EMPTY_ENTRY( m_pCurVar ) )
                continue;
            bFound = true;
            break;
        }
        if( bFound )
            break;
        m_iIdx++;
        if( m_iIdx < m_pParent->m_dwRowCount )
            m_pCurVar = m_pParent->m_vecVarArrs[ m_iIdx ] - 1;
    }
    if( !bFound )
    {
        m_iIdx = -1;
        m_pCurVar = nullptr;
    }
    return *this;
}

bool CVarList::iterator::operator==( const iterator& rhs )
{
    if( m_iIdx == rhs.m_iIdx &&
        m_pCurVar == rhs.m_pCurVar )
        return true;
    return false;
}

CVarList::iterator& CVarList::iterator::operator=( iterator&& rhs )
{
    m_iIdx = rhs.m_iIdx;
    m_pCurVar = rhs.m_pCurVar;
    rhs.m_pCurVar = nullptr;
    rhs.m_iIdx = -1;
    m_pParent = rhs.m_pParent;
    return *this;
}

CVarList::iterator& CVarList::iterator::operator=( const iterator& rhs )
{
    m_iIdx = rhs.m_iIdx;
    m_pCurVar = rhs.m_pCurVar;
    return *this;
}

CVarList::CVarList()
{
    guint32 i = 0;
    guint32 dwCount =
        sizeof( m_vecVarArrs ) / sizeof( CFG_ELEM* );

    for( ; i < dwCount; i++ )
        m_vecVarArrs[ i ] = nullptr;

    CFG_ELEM* pVars = new CFG_ELEM[ BASE_NUM_VAR ];
    m_vecVarArrs[ m_dwRowCount++ ] = pVars;
    for( int i = 0; i < BASE_NUM_VAR; i++ )
        ( pVars++ )->first = -1;
}

CVarList::~CVarList()
{
    if( m_dwRowCount > 0 )
    {
        for( int i = 0; i < m_dwRowCount; i++ )
            delete[] m_vecVarArrs[ i ];
    }
    m_dwRowCount = 0;
    memset( m_vecVarArrs,
        0, sizeof( m_vecVarArrs ) );
    return;
}

void CVarList::clear()
{
    if( empty() )
        return;
    for( int i = 0; i < m_dwRowCount; i++ )
        delete[] m_vecVarArrs[ i ];
    memset( m_vecVarArrs,
        0, sizeof( m_vecVarArrs ) );
    m_dwRowCount = 0;
    CFG_ELEM* pVars = new CFG_ELEM[ BASE_NUM_VAR ];
    m_vecVarArrs[ m_dwRowCount++ ] = pVars;
    for( int i = 0; i < BASE_NUM_VAR; i++ )
    {
        pVars->first = -1;
        pVars++;
    }
}

CVarList::iterator CVarList::begin()
{
    if( empty() )
        return end();

    auto itr = iterator( 0, m_vecVarArrs[ 0 ] - 1 );
    itr.m_pParent = this;
    itr++;
    return itr;
}

CVarList::iterator CVarList::end()
{
    auto itr = iterator( -1, nullptr );
    itr.m_pParent = this;
    return itr;
}

const CVarList::iterator CVarList::begin() const
{
    auto itr = iterator( 0, m_vecVarArrs[ 0 ] );
    itr.m_pParent = const_cast<CVarList*>( this );
    return itr;
}

const CVarList::iterator CVarList::end() const
{
    auto itr = iterator( -1, nullptr );
    itr.m_pParent = const_cast<CVarList*>( this );
    return itr;
}

CVarList::const_iterator CVarList::cbegin() const
{
    auto itr = const_iterator( 0, m_vecVarArrs[ 0 ] );
    itr.m_pParent = const_cast<CVarList*>( this );
    return itr;
}

CVarList::const_iterator CVarList::cend() const
{
    auto itr = const_iterator( -1, nullptr );
    itr.m_pParent = const_cast<CVarList*>( this );
    return itr;
}

CVarList::iterator CVarList::find( gint32 iProp )
{
    bool bFound = false;
    auto itr = begin();
    while( itr != end() )
    {
        if( itr->first == iProp )
            break;
        itr++;
    }
    if( bFound )
        return itr;
    return end();
}

CVarList::iterator CVarList::erase( gint32 iProp )
{
    auto itr = find( iProp );
    if( itr == end() )
        return itr;
    itr->first = -1;
    itr->second.Clear();
    m_dwSize--;
    itr++;
    return itr;
}

CVarList::iterator CVarList::erase(
    CVarList::iterator itr )
{
    itr->first = -1;
    itr->second.Clear();
    m_dwSize--;
    itr++;
    return itr;
}

gint32 CVarList::push_row()
{
    guint32 dwSize =
        ROW_COUNT( m_dwRowCount );
    CFG_ELEM* pNewRow = new CFG_ELEM[ dwSize ];
    m_vecVarArrs[ m_dwRowCount++ ] = pNewRow;
    for( int i = 0; i < dwSize; i++ )
        ( pNewRow++ )->first = -1;
    return 0;
}

gint32 CVarList::pop_row()
{
    if( m_dwRowCount <= 1 )
        return -ENOENT;
    guint32 dwSize = LAST_ROW_COUNT;

    m_dwRowCount--;
    CFG_ELEM* pLast = m_vecVarArrs[ m_dwRowCount ];
    m_vecVarArrs[ m_dwRowCount ] = nullptr;

    for( guint32 i = 0; i < dwSize; i++ ) 
    {
        pLast[ i ].first = -1;
        pLast[ i ].second.Clear();
    }

    delete[] pLast;
    return STATUS_SUCCESS;
}

size_t CVarList::size() const
{ return m_dwSize; }

gint32 CVarList::push_back( const CFG_ELEM& elem )
{
    CFG_ELEM* pPos =
        LAST_ROW + LAST_ROW_COUNT - 1;
    CFG_ELEM* pEnd = pPos;

    do{
        for( int i = LAST_ROW_COUNT - 1; i >= 0; i-- )
        {
            if( pPos->first == -1 )
            {
                pPos--;
                continue;
            }
            break;
        }
        if( pPos == pEnd )
        {
            push_row();
            pPos = LAST_ROW;
            *pPos = elem;
            break;
        }
        pPos++;
        *pPos = elem;
        break;
    }while( true );
    m_dwSize++;
    return 0;
}

gint32 CVarList::pop_back()
{
    if( empty() )
        return -ENOENT;

    CFG_ELEM* pPos =
        LAST_ROW + LAST_ROW_COUNT - 1;

    gint32 ret = -ENOENT;
    do{
        for( int i = LAST_ROW_COUNT - 1; i >= 0; i-- )
        {
            if( pPos->first == -1 )
            {
                pPos--;
                continue;
            }
        }

        if( pPos < LIST_FIRST )
            break;

        if( pPos < LAST_ROW )
        {
            pop_row();
            continue;
        }
        pPos->first = -1;
        pPos->second.Clear();
        ret = STATUS_SUCCESS;
        break;

    }while( true );
    return ret;
}

CFG_ELEM& CVarList::back() {
    bool bFound = false;
    if( empty() )
    {
        std::string strMsg = DebugMsg( -ENOENT,
            std::string( "no elem in the list" ) );
        throw std::invalid_argument( strMsg );
    }

    CFG_ELEM* pPos =
        LAST_ROW + LAST_ROW_COUNT - 1;
    do{
        for( int i = LAST_ROW_COUNT - 1; i >= 0; i-- )
        {
            if( pPos->first == -1 )
            {
                pPos--;
                continue;
            }
        }

        if( pPos < LAST_ROW )
        {
            if( LAST_ROW_COUNT == BASE_NUM_VAR )
            {
                std::string strMsg = DebugMsg( -ENOENT,
                    std::string( "no elem in the list" ) );
                throw std::invalid_argument( strMsg );
            }

            pop_row();
            pPos = 
                LAST_ROW + LAST_ROW_COUNT - 1;
            continue;
        }
        break;
    }while( true );
    return *pPos;
}

CVarList::const_iterator& CVarList::const_iterator::operator++()
{
    bool bFound = false;
    while( m_iIdx < m_pParent->m_dwRowCount )
    {
        while( m_pCurVar + 1 < ITR_ROW_END( m_iIdx ) )
        {
            m_pCurVar += 1;
            if( EMPTY_ENTRY( m_pCurVar ) )
                continue;
            bFound = true;
            break;
        }
        if( bFound )
            break;
        m_iIdx++;
        if( m_iIdx < m_pParent->m_dwRowCount )
            m_pCurVar = m_pParent->m_vecVarArrs[ m_iIdx ] - 1;
    }
    if( !bFound )
    {
        m_iIdx = -1;
        m_pCurVar = nullptr;
    }
    return *this;
}

CVarList::const_iterator&
CVarList::const_iterator::operator++( int )
{
    bool bFound = false;
    while( m_iIdx < m_pParent->m_dwRowCount )
    {
        while( m_pCurVar + 1 < ITR_ROW_END( m_iIdx ) )
        {
            m_pCurVar += 1;
            if( EMPTY_ENTRY( m_pCurVar ) )
                continue;
            bFound = true;
            break;
        }
        if( bFound )
            break;
        m_iIdx++;
        if( m_iIdx < m_pParent->m_dwRowCount )
            m_pCurVar = m_pParent->m_vecVarArrs[ m_iIdx ] - 1;
    }
    if( !bFound )
    {
        m_iIdx = -1;
        m_pCurVar = nullptr;
    }
    return *this;
}

bool CVarList::const_iterator::operator==(
    const_iterator& rhs )
{
    if( m_iIdx == rhs.m_iIdx &&
        m_pCurVar == rhs.m_pCurVar )
        return true;
    return false;
}

CVarList::const_iterator::const_iterator(
    const_iterator& rhs )
{
    m_iIdx = rhs.m_iIdx;
    m_pCurVar = rhs.m_pCurVar;
    m_pParent = rhs.m_pParent;
}

CVarList::const_iterator::const_iterator(
    gint32 iIdx, CFG_ELEM* pCurElem )
{
    m_iIdx = iIdx;
    m_pCurVar = pCurElem;
}

CVarList::const_iterator::const_iterator(
    const_iterator&& rhs )
{
    m_iIdx = rhs.m_iIdx;
    m_pCurVar = rhs.m_pCurVar;
    rhs.m_pCurVar = nullptr;
    rhs.m_iIdx = -1;
    m_pParent = rhs.m_pParent;
}

}
