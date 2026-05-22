/*
 * =====================================================================================
 *
 *       Filename:  stast.h
 *
 *    Description:  declaration of classes for ast nodes
 *
 *        Version:  1.0
 *        Created:  05/06/2026 08:38:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

#include "stscript.h"

/* the node is a constant value */
#define NODE_FLAG_CONST         0x1
/* the node is a pointer variable */
#define NODE_FLAG_POINTER       0x2
/* the node is a reference variable */
#define NODE_FLAG_REF           0x4
/* the node is for input parameter, read-only*/
#define NODE_FLAG_INPUT         0x8
/* the node is for output parameter, write-only*/
#define NODE_FLAG_OUTPUT        0x10
/* the node is for a block of memory */
#define NODE_FLAG_MEM           0x20
/* target code is generated */
#define NODE_FLAG_GENERATED     0x40

namespace rpcf
{

struct CSTAstNodeBase :
    public CObjBase
{
    guint32 m_dwFlags = 0;
    typedef CObjBase super;
    CSTAstNodeBase():super() {} 
    CSTAstNodeBase* m_pParent;
    gint32 m_iToken = YYUNDEF;
    YYLTYPE2  m_oLocation;
    // the file index in the included file stack. -1 means the main file.
    gint32 m_iFileIdx = -1;
    YYSTYPE m_oValue;
    
    std::vector< CSTAstNodeBase > m_vecChildren;

    inline void SetParent(
        CSTAstNodeBase* pParent )
    {
        if( pParent != nullptr )
            m_pParent = pParent;
    }

    CSTAstNodeBase* GetParent() const
    { return m_pParent; }

    virtual std::string ToString() const
    { return std::string( "" ); }

    virtual std::string GetSignature() const
    { return std::string( "" ); }

    inline void SetLocation( YYLTYPE& oLoc )
    { m_oLocation = oLoc; }
};


}
