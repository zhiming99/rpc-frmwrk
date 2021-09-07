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
std::string GetTypeSig( ObjPtr& pObj )
{
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
            std::string strName = pRef->GetName();
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
