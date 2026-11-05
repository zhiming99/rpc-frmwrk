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
 *      Copyright:  2026 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

namespace rpcf
{

struct CStAstNodeBase :
    public CObjBase
{
    typedef CObjBase super;
    guint32 m_dwFlags = 0;
    CStAstNodeBase():super()
    {} 
    CStAstNodeBase* m_pParent = nullptr;

    // the file index in the include file stack.
    // -1 means the file to parse.
    gint32 m_iFileIdx = -1;
    
    inline void SetParent(
        CStAstNodeBase* pParent )
    {
        if( pParent != nullptr )
            m_pParent = pParent;
    }

    CStAstNodeBase* GetParent() const
    { return m_pParent; }

    virtual std::string ToString() const
    { return std::string( "" ); }

    virtual std::string GetSignature() const
    { return std::string( "" ); }

    virtual std::string GetNodeInfo() const 
    { return ""; }
};


}
