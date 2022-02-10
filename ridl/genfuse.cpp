/*
 * =====================================================================================
 *
 *       Filename:  genfuse.cpp
 *
 *    Description:  implementation of FUSE integration related methods 
 *
 *        Version:  1.0
 *        Created:  02/10/2022 05:01:42 PM
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
#include <sys/stat.h>
#include "rpc.h"
using namespace rpcf;
#include "astnode.h"
#include "gencpp.h"
#include "sha1.h"
#include "proxy.h"

extern std::string g_strAppName;
extern guint32 g_dwFlags;

extern gint32 FuseDeclareMsgSet(
    CCppWriter* m_pWriter, ObjPtr& pRoot )
{

    if( m_pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStatements* pStmts = pRoot;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        std::string strAppName =
            pStmts->GetName();
        // declare the g_setMsgIds
        std::vector< ObjPtr > vecStructs;
        pStmts->GetStructDecls( vecStructs );
        std::vector< stdstr > vecMsgIds;
        for( auto& elem : vecStructs )
        {
            CStructDecl* psd = elem;
            if( psd->RefCount() == 0 )
                continue;

            std::string strName = psd->GetName();
            std::string strMsgId =
                strAppName + "::" + strName;
            guint32 dwMsgId = GenClsid( strMsgId );

            stdstr strClsid =
                FormatClsid( dwMsgId );

            vecMsgIds.push_back( strClsid );
        }

        if( vecMsgIds.empty() )
            break;

        Wa( "std::set< guint32 > g_setMsgIds = " );        
        BLOCK_OPEN;
        size_t i = 0, count = vecMsgIds.size();
        for( ;i < count; i++ )
        {
            CCOUT << vecMsgIds[ i ];
            if( i < count - 1 ) 
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }
        BLOCK_CLOSE;
        Wa( ";" );
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CDeclareStruct::OutputFuse()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "struct " ); 
        CCOUT << strName;
        INDENT_UPL;
        CCOUT <<  ": public CJsonStructBase";
        INDENT_DOWNL;
        BLOCK_OPEN;
    
        Wa( "typedef CJsonStructBase super;" );
        NEW_LINE;

        bool bAll = ( ( bFuseP != bFuseS ) || !bFuse );

        ObjPtr pFields =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFields;
        guint32 dwCount = pfl->GetCount();
        if( dwCount == 0 )
        {
            ret = -ENOENT;
            break;
        }

        ObjPtr pParent = m_pNode->GetParent();
        CStatements* pstmts = pParent;

        std::string strAppName =
            pstmts->GetName();

        std::string strMsgId =
            strAppName + "::" + strName;

        guint32 dwMsgId = GenClsid( strMsgId );

        Wa( "//Message identity" );
        CCOUT << "guint32 m_dwMsgId = clsid( "
            << strName << " );";
        NEW_LINE;

        CCOUT << "std::string m_strMsgId = "
            << "\"" << strMsgId << "\";";
        NEW_LINE;

        Variant oVar;
        oVar = dwMsgId;
        m_pNode->SetProperty(
            PROP_MESSAGE_ID, oVar );

        Wa( "// data members" );
        guint32 i = 0;
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pfl->GetChild( i );
            CFieldDecl* pfd = pObj;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            std::string strLine =
                pfd->ToStringCpp();
            CCOUT<< strLine << ";"; 
            NEW_LINE;
        }

        NEW_LINE;
        Wa( "// Constructor" );
        CCOUT << strName << "() : super()";
        NEW_LINE;
        CCOUT << "{ SetClassId( ( EnumClsid )m_dwMsgId ); }";
        NEW_LINES( 2 );
        Wa( "// methods" );
        // declare two methods to implement
        CCOUT<< "gint32 Serialize(";
        if( bAll )
        {
            CCOUT << "    BufPtr& pBuf_ ) override;";
            NEW_LINE;
        }
        else
        {
            CCOUT << "    BufPtr& pBuf_ ) override";
            NEW_LINE;
            Wa( "{ return -ENOTSUP; }" );
        }
        NEW_LINE;
        CCOUT << "gint32 Deserialize(";
        if( bAll )
        {
            CCOUT <<"    BufPtr& pBuf_ ) override;"; 
            NEW_LINE;
        }
        else
        {
            CCOUT <<"    BufPtr& pBuf_ ) override";
            NEW_LINE;
            Wa( "{ return -ENOTSUP; }" );
        }
        Wa( "guint32 GetMsgId() const override" );
        Wa( "{ return m_dwMsgId; }" );
        NEW_LINE;
        CCOUT << "const std::string&";
        INDENT_UPL;
        CCOUT << "GetMsgName() const override";
        INDENT_DOWNL;
        Wa( "{ return m_strMsgId; }" );

        NEW_LINE;
        CCOUT << strName << "& " << "operator=(";
        if( bAll )
        {
            CCOUT << "    const " << strName << "& rhs );";
        }
        else
        {
            CCOUT << "    const " << strName << "& rhs )";
            NEW_LINE;
            CCOUT << "{ return *this; }";
        }
        NEW_LINES(2);

        Wa( "gint32 JsonSerialize(" );
        Wa( "    BufPtr& pBuf, const Value& val ) override;" );
        NEW_LINE;
        Wa( "gint32 JsonDeserialize(" );
        CCOUT << "    BufPtr& pBuf, Value& val ) override;";
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CImplSerialStruct::OutputFuse()
{
    gint32 ret = 0;
    do{
        ret = OutputSerialFuse();
        if( ERROR( ret ) )
            break;
        NEW_LINE;
        ret = OutputDeserialFuse();
        if( ERROR( ret ) )
            break;
        NEW_LINE;
    }while( 0 );
    return ret;
}

gint32 CImplSerialStruct::OutputSerialFuse()
{
    gint32 ret = 0;
    do{
        CCOUT << "gint32 " << m_pNode->GetName()
            << "::" << "JsonSerialize(";
        NEW_LINE;
        Wa( "    BufPtr& pBuf_, const Value& val_ )" );
        BLOCK_OPEN;
        CCOUT << "if( pBuf_.IsEmpty() )";
        INDENT_UPL;
        CCOUT << "return -EINVAL;";
        INDENT_DOWNL;
        CCOUT << "gint32 ret = 0;";
        NEW_LINE;
        CCOUT << "do";
        BLOCK_OPEN;

        CCOUT << "ret = CSerialBase::Serialize(";
        INDENT_UPL;
        CCOUT << "pBuf_, m_dwMsgId );";
        INDENT_DOWNL;
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        ObjPtr pFields =
            m_pNode->GetFieldList();

        CEmitSerialCode oesc(
            m_pWriter, pFields );

        oesc.OutputSerial( "", "pBuf_" );

        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );

        Wa( "return ret;" );

        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplSerialStruct::OutputDeserialFuse()
{
    return -ENOTSUP;
}
