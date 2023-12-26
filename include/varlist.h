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
#include <string>
#include <vector>
#define BASE_NUM_VAR 8

#define ROW_COUNT( __idx ) \
    ( ( 1 << (__idx) ) * BASE_NUM_VAR )

#define ROW_END( __idx ) \
    ( m_vecVarArrs[ (__idx ) ] + ROW_COUNT( (__idx) ) )

#define ROW_LAST( __idx ) \
    ( m_vecVarArrs[ (__idx) ] + ROW_COUNT( (__idx) ) - 1 )

#define ROW_FIRST( __idx ) ( m_vecVarArrs[ (__idx) ] )

#define LIST_FIRST \
    ( m_vecVarArrs.front() )

#define LIST_LAST \
    ( ROW_END( m_vecVarArrs.size() - 1 ) )

#define LAST_ROW \
    ( m_vecVarArrs.back() )

#define LAST_ROW_COUNT \
    ( ROW_COUNT( m_vecVarArrs.size() - 1 ) )

#define EMPTY_ENTRY( __elem ) \
    ( __elem->first == -1 )

namespace rpcf{

typedef std::pair< gint32, Variant > CFG_ELEM;

struct CVarList
{
    std::vector< CFG_ELEM* > m_vecVarArrs;
    guint32 m_dwSize = 0;
    struct iterator
    {
        gint32 m_iIdx = -1;
        CFG_ELEM* m_pCurVar = nullptr;
        guint32 m_dwSize = 0;
        CVarList* m_pParent = nullptr;

        iterator(){}
        iterator( const iterator& rhs );
        iterator( iterator&& rhs );
        iterator( gint32 iIdx, CFG_ELEM* pCurElem );
        iterator& operator++();
        iterator& operator++( int );
        bool operator==( const iterator& rhs );
        bool operator!=( const iterator& rhs )
        { return !( *this == rhs ); }
        iterator& operator=( iterator&& rhs );
        iterator& operator=( const iterator& rhs );
        CFG_ELEM* operator->()
        { return m_pCurVar; }
        CFG_ELEM& operator*()
        { return *m_pCurVar; }
    };
    struct const_iterator : public iterator
    {
        typedef iterator super;
        const_iterator() {}
        const_iterator( const_iterator& rhs );
        const_iterator( const_iterator&& rhs );
        const_iterator( gint32 iIdx, CFG_ELEM* pCurElem );

        const_iterator& operator++();
        const_iterator& operator++( int );
        bool operator==( const_iterator& rhs );
        bool operator!=( const_iterator& rhs )
        { return !( *this == rhs ); }

        bool operator!=( const_iterator&& rhs )
        { return !( *this == rhs ); }

        const CFG_ELEM* operator->()
        { return m_pCurVar; }
        const CFG_ELEM& operator*()
        { return *m_pCurVar; }
    };

    CVarList();
    ~CVarList();
    void clear();
    iterator begin();
    iterator end();
    const iterator begin() const;
    const iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
    iterator find( gint32 iProp );
    iterator erase( gint32 iProp );
    iterator erase( iterator itr );
    gint32 push_row();
    gint32 pop_row();
    size_t size() const;
    gint32 push_back( const CFG_ELEM& elem );
    gint32 pop_back();
    CFG_ELEM& back();
    bool empty() const
    { return ( m_dwSize == 0 ); }
};

}
