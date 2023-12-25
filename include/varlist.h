/*
 * =====================================================================================
 *
 *       Filename:  varlist.h
 *
 *    Description:  declaration of a simple variant list for debug purpose 
 *
 *        Version:  1.0
 *        Created:  12/25/2023 02:04:21 AM
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
#pragma once
#include "variant.h"
#include "configdb.h"
#define BASE_NUM_VAR 8

#define std::pair< gint32, Variant > LIST_ELEM;

#define ROW_COUNT( __idx ) \
    ( ( 1 << (__idx) ) * BASE_NUM_VAR )

#define ROW_END( __idx ) \
    ( m_vecVarArrs[ (__idx ) ] + ROW_COUNT( (__idx) ) )

#define ROW_LAST( __idx ) \
    ( m_vecVarArrs[ (__idx) ] + ROW_COUNT( (__idx) ) - 1 )

#define ROW_FIRST( __idx ) ( m_vecVarArrs[ (__idx) ] )

#define LIST_LAST() \
    ( ROW_END( m_vecVarArrs.size() - 1 ) )

#define LAST_ROW() \
    ( m_vecVarArrs.back() )

#define LAST_ROW_COUNT() \
    ( ROW_COUNT( m_vecVarArrs.size() - 1 ) )

#define EMPTY_ENTRY( __elem ) \
    ( __elem->first == -1 )

struct CVarList
{
    std::vector< LIST_ELEM* > m_vecVarArrs;
    class iterator
    {
        gint32 m_iIdx = -1;
        LIST_ELEM* m_pCurVar = nullptr;
        guint32 m_dwSize = 0;

        iterator(){}

        iterator( const iterator& rhs )
        {
            m_iIdx = rhs.m_iIdx;
            m_pCurVar = rhs.m_pCurVar;
        }

        iterator( iterator&& rhs )
        {
            m_iIdx = rhs.m_iIdx;
            m_pCurVar = rhs.m_pCurVar;
            rhs.m_pCurVar = nullptr;
            rhs.m_iIdx = -1;
        }

        iterator( gint32 iIdx, LIST_ELEM* pCurElem )
        {
            m_iIdx = iIdx;
            m_pCurVar = pCurElem;
        }

        iterator& operator++() volatile
        {
            bool bFound = false;
            while( m_iIdx < m_vecVarArrs.size() )
            {
                while( m_pCurVar + 1 < ROW_END( m_iIdx ) )
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
                if( m_iIdx < m_vecVarArrs.size() )
                    m_pCurVar = m_vecArrs[ m_iIdx ];
            }
            if( !bFound )
            {
                m_iIdx == -1;
                m_pCurVar = nullptr;
            }
            return *this;
        }

        iterator& operator++( int ) volatile
        {
            bool bFound = false;
            while( m_iIdx < m_vecVarArrs.size() )
            {
                while( m_pCurVar + 1 < ROW_END( m_iIdx ) )
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
                if( m_iIdx < m_vecVarArrs.size() )
                    m_pCurVar = m_vecArrs[ m_iIdx ];
            }
            if( !bFound )
            {
                m_iIdx == -1;
                m_pCurVar = nullptr;
            }
            return *this;
        }

        bool operator==( const iterator& rhs )
        {
            if( m_iIdx == rhs.m_iIdx &&
                m_pCurVar == rhs.m_pCurVar )
                return true;
            return false;
        }

        iterator& operator=( iterator&& rhs )
        {
            m_iIdx = rhs.m_iIdx;
            m_pCurVar = rhs.m_pCurVar;
            rhs.m_pCurVar = nullptr;
            rhs.m_iIdx = -1;
        }

        iterator& operator=( const iterator& rhs )
        {
            m_iIdx = rhs.m_iIdx;
            m_pCurVar = rhs.m_pCurVar;
        }

        LIST_ELEM& operator->()
        { return *m_pCurVar; }
    };

    CVarList()
    {
        Variant* pVars = new LIST_ELEM(
            { -1, Variant(0)})[ BASE_NUM_VAR ];
        m_vecVarArrs.push_back( pVars );
    }

    ~CVarList()
    {
        clear();
        return
    }

    void clear()
    {
        for( auto elem : m_vecVarArrs )
            delete[] elem;
        m_vecVarArrs.clear();
        Variant* pVars = new LIST_ELEM(
            { -1, Variant(0)})[ BASE_NUM_VAR ];
        m_vecVarArrs.push_back( pVars );
    }

    iterator begin()
    { return iterator( 0, m_vecArrs[ 0 ] ); }

    iterator end()
    { return iterator( -1, nullptr ); }

    const iterator cbegin() const
    { return iterator( 0, m_vecArrs[ 0 ] ); }

    const iterator cend() const
    { return iterator( -1, nullptr ); }

    iterator find( gint32 iProp )
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

    iterator erase( gint32 iProp )
    {
        auto itr = find( iProp );
        if( itr == end() )
            return itr;
        itr->first = -1;
        itr->second.Clear();
        m_dwSize--;
        return itr++;
    }

    void add_row()
    {
        guint32 dwSize =
            ROW_COUNT( m_vecVarArrs.size() );
        LIST_ELEM* pNewRow = new LIST_ELEM(
            { -1, Variant(0)})[ dwSize ];
    }

    size_t size()
    { return m_dwSize; }

    void push_back( const LIST_ELEM& elem )
    {
        bool bFound = false;
        LIST_ELEM* pRow = LAST_ROW();
        do{
            for( int i = 0; i < LAST_ROW_COUNT(); i++ )
            {
                if( pRow->first == -1 )
                {
                    *pRow = elem;
                    bFound = true;
                    break;
                }
                pRow++;
            }
            add_row();

        }while( !bFound );
        m_dwSize++;
    }
};
