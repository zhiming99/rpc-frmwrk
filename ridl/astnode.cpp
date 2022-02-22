/*
 * =====================================================================================
 *
 *       Filename:  astnode.cpp
 *
 *    Description:  implentations of the tree nodes for abstract syntax tree
 *
 *        Version:  1.0
 *        Created:  03/01/2021 02:00:00 PM
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
#include "rpc.h"
using namespace rpcf;
#include "astnode.h"
#include "gencpp.h"
extern guint32 g_dwFlags;
extern stdstr g_strLang;
extern std::string GetTypeSigJava( ObjPtr& pObj );
std::string GetTypeSig( ObjPtr& pObj )
{
    if( g_strLang == "java" )
        return GetTypeSigJava( pObj );

    std::string strSig;
    gint32 ret = 0;
    if( pObj.IsEmpty() )
        return strSig;
    do{
        CAstNodeBase* pType = pObj;
        if( pType->GetClsid() !=
            clsid( CStructRef ) )
        {
            strSig = pType->GetSignature();
        }
        else
        {
            ObjPtr pRefType;
            CStructRef* pRef = pObj;
            stdstr strName = pRef->GetName();
            ret = g_mapDecls.GetDeclNode(
                strName, pRefType );
            if( SUCCEEDED( ret ) )
            {
                strSig = "O";
                break;
            }
            ret = g_mapAliases.GetAliasType(
                strName, pRefType );
            if( ERROR( ret ) )
                break;
            CAstNodeBase* pNode = pRefType;
            strSig = pNode->GetSignature();
        }
    }while( 0 );

    return strSig;
}

guint32 CAttrExps::GetAsyncFlags() const
{
    guint32 dwFlags = 0;
    if( bFuseP && bFuseS )
        return NODE_FLAG_ASYNC;

    for( auto elem : m_queChilds )
    {
        CAttrExp* pExp = elem;
        if( pExp == nullptr )
            continue;
        guint32 dwToken = pExp->GetName();
        if( dwToken == TOK_ASYNC )
            dwFlags = NODE_FLAG_ASYNC;
        else if( dwToken == TOK_ASYNCP )
            dwFlags = NODE_FLAG_ASYNCP;
        else if( dwToken == TOK_ASYNCS )
            dwFlags = NODE_FLAG_ASYNCS;
        if( dwFlags > 0 )
            break;
    }

    if( bFuseP )
        dwFlags |= NODE_FLAG_ASYNCP;
    else if( bFuseS )
        dwFlags |= NODE_FLAG_ASYNCS;

    return dwFlags;
}
