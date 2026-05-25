/*
 * =====================================================================================
 *
 *       Filename:  lvalvar.h
 *
 *    Description:   declarations of classes for l-value variables 
 *
 *        Version:  1.0
 *        Created:  05/25/2026 01:49:35 PM
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

#include "stast.h"

struct CLValueVariable:
    public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;
};

struct CInstancePath:
    public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;
    std::vector< CAstNodeBase* > m_vecIds;
};

struct CLValueVariableInstPath
    public CLValueVariable
{
    typedef CLValueVariable super;
    CInstancePath* m_pInstPath = nullptr;
};

struct CLValueVariableDataMember
    public CLValueVariable
{
    typedef CLValueVariable super;
    CLValueVariable* m_pLValue;
    CInstancePath* m_pInstPath
};

struct CLValueVariableDefPtr
    public CLValueVariable
{
    typedef CLValueVariable super;
    CLValueVariable* m_pLValue;
};

struct CExpressionBase:
    public CSTAstNodeBase
{
    typedef CSTAstNodeBase super;
};

struct CLValueVariableArrayAccess
    public CLValueVariable
{
    typedef CLValueVariable super;
    CLValueVariable* m_pLValue;
    CExpressionBase* m_pExpr;
};
