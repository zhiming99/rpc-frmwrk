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

namespace rpcf
{

struct CSTAstNodeBase :
    public CObjBase
{
    guint32 m_dwFlags = 0;
    typedef CObjBase super;
    CSTAstNodeBase():super() {} 
    CSTAstNodeBase* m_pParent;

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
};


}
