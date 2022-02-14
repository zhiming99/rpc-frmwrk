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
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
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
#include "genfuse.h"
#include <json/json.h>

CEmitSerialCodeFuse::CEmitSerialCodeFuse(
    CWriterBase* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pArgs = pNode;
    if( m_pArgs == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CEmitSerialCodeFuse::OutputSerial(
    const std::string& strObjc,
    const std::string strBuf )
{
    gint32 ret = 0;
    do{

        guint32 i = 0;
        guint32 dwCount = m_pArgs->GetCount();
        Wa( "Value _oMember;" );
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = m_pArgs->GetChild( i );
            CFieldDecl* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            pObj = pfa->GetType();
            std::string strName = pfa->GetName();
            CAstNodeBase* pTypeNode = pObj;

            std::string strSig =
                pTypeNode->GetSignature();

            if( strSig.empty() )
            {
                ret = -EINVAL;
                break;
            }

            std::string strObj;
            if( !strObjc.empty() )
                strObj = strObjc + '.';

            CCOUT << "if( !val_.hasMember( \""
                << strName << "\" ) )";
            NEW_LINE;
            Wa( "{ ret = -ENOENT; break; }" );
            CCOUT << "_oMember = val_[ \""
                << strName << "\" ];";

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    Wa( "if( !_oMember.isArray() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeArray(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember," << "\""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case '[' :
                {
                    Wa( "if( !_oMember.isObject() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = "<< strObj
                        << "SerializeMap(";
                    INDENT_UPL;
                    CCOUT << strBuf << ", _oMember,"
                        << "\"" << strSig << "\" );";
                    INDENT_DOWNL;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }

            case 'O' :
                {
                    Wa( "if( !_oMember.isObject() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeStruct( ";
                    INDENT_UPL;
                    CCOUT << strBuf << ", _oMember );";
                    INDENT_DOWNL;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'h':
                {
                    Wa( "if( !_oMember.isUInt64() &&" );
                    Wa( "    !_oMember.isInt64() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        <<"SerializeHStream( "
                        << strBuf << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'Q':
            case 'q':
                {
                    Wa( "if( !_oMember.isUInt64() &&" );
                    Wa( "    !_oMember.isInt64() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeUInt64(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'D':
            case 'd':
                {
                    Wa( "if( !_oMember.isUInt() &&" );
                    Wa( "    !_oMember.isInt() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeUInt(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'W':
            case 'w':
                {
                    Wa( "if( !_oMember.isUInt() &&" );
                    Wa( "    !_oMember.isInt() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeUShort(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'f':
                {
                    Wa( "if( !_oMember.isDouble() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeFloat(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'F':
                {
                    Wa( "if( !_oMember.isDouble() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeDouble(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'b':
                {
                    Wa( "if( !_oMember.isBool() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeBool(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'B':
                {
                    Wa( "if( !_oMember.isUInt() &&" );
                    Wa( "    !_oMember.isInt() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeByte(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 's':
                {
                    Wa( "if( !_oMember.isString()" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeString(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'a':
                {
                    Wa( "if( !_oMember.isString()" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeByteArray(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'o':
                {
                    Wa( "if( !_oMember.isString()" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeObjPtr(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
            if( i + 1 < dwCount )
                NEW_LINE;
        }

    }while( 0 );

    return ret;
}

gint32 CEmitSerialCodeFuse::OutputDeserial(
    const std::string& strObjc,
    const std::string strBuf )
{
    gint32 ret = 0;
    do{
        guint32 i = 0;
        guint32 dwCount = m_pArgs->GetCount();

        Wa( "val_.setType( Json::objectValue );" );
        Wa( "Value _oMember;" );

        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = m_pArgs->GetChild( i );
            CFieldDecl* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            pObj = pfa->GetType();
            std::string strName = pfa->GetName();
            CAstNodeBase* pTypeNode = pObj;

            std::string strSig =
                pTypeNode->GetSignature();

            if( strSig.empty() )
            {
                ret = -EINVAL;
                break;
            }

            std::string strObj;
            if( !strObjc.empty() )
                strObj = strObjc + '.';

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeArray(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf << ", _oMember, \""
                        << strSig << "\" );";
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            case '[' :
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeMap(";
                    CCOUT << "    " << strBuf << ", _oMember, \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }

            case 'O' :
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeStruct(";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'h':
                {
                    Wa( "ret = DeserializeHStream( " );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'Q':
                {
                    Wa( "ret = DeserializeUInt64(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'q':
                {
                    Wa( "ret = DeserializeUInt64(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    Wa( "_oMember.setType( intValue );" );
                    break;
                }
            case 'D':
                {
                    Wa( "ret = DeserializeUInt(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'd':
                {
                    Wa( "ret = DeserializeUInt(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    Wa( "_oMember.setType( intValue );" );
                    break;
                }
            case 'W':
                {
                    Wa( "ret = DeserializeUShort(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'w':
                {
                    Wa( "ret = DeserializeUShort(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    Wa( "_oMember.setType( intValue );" );
                    break;
                }
            case 'f':
               {
                    Wa( "ret = DeserializeFloat(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'F':
                {
                    Wa( "ret = DeserializeDouble(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'b':
                {
                    Wa( "ret = DeserializeBool(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'B':
                {
                    Wa( "ret = DeserializeByte(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 's':
                {
                    Wa( "ret = DeserializeString(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'a':
                {
                    Wa( "ret = DeserializeByteArray(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'o':
                {
                    Wa( "ret = DeserializeObjPtr(" );
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }

            if( ERROR( ret ) )
                break;

            CCOUT << "val_[ \"" << strName
                << "\" ] = _oMember;";
            NEW_LINE;
            CCOUT << "_oMember = 0;";
            if( i + 1 < dwCount )
                NEW_LINE;
        }
    }while( 0 );

    return ret;
}

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
        ret = OutputSerial();
        if( ERROR( ret ) )
            break;
        NEW_LINE;
        ret = OutputDeserial();
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

        CCOUT << "ret = Serialize( pBuf_, m_dwMsgId );";
        NEW_LINE;
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        ObjPtr pFields =
            m_pNode->GetFieldList();

        CEmitSerialCodeFuse oesc(
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
    gint32 ret = 0;
    do{
        CCOUT << "gint32 " << m_pNode->GetName()
            << "::" << "JsonDeserialize( ";
        CCOUT << "BufPtr& pBuf_, Value& val_ )";
        NEW_LINE;
        BLOCK_OPEN;
        CCOUT << "if( pBuf_.IsEmpty() )";
        INDENT_UPL;
        CCOUT << "return -EINVAL;";
        INDENT_DOWNL;
        CCOUT << "gint32 ret = 0;";
        NEW_LINE;
        CCOUT << "do";
        BLOCK_OPEN;

        ObjPtr pFields =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFields;
        guint32 dwCount = pfl->GetCount();
        if( dwCount == 0 )
        {
            ret = -ENOENT;
            break;
        }

        Wa( "guint32 dwMsgId = 0;" );
        CCOUT << "ret = Deserialize(";
        NEW_LINE;
        CCOUT << "    pBuf_, dwMsgId );";
        NEW_LINE;
        Wa( "if( ERROR( ret ) ) return ret;" );
        Wa( "if( m_dwMsgId != dwMsgId ) return -EINVAL;" );
        NEW_LINE;

        CEmitSerialCodeFuse odesc( m_pWriter, pFields );
        odesc.OutputDeserial( "", "pBuf_" );
        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );

        Wa( "return ret;" );

        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

CDeclInterfProxyFuse::CDeclInterfProxyFuse(
    CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter )
{
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "interface declaration" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CDeclInterfProxyFuse::Output()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_PImpl";
        CCOUT << strClass;
        std::string strBase;
        bool bStream = m_pNode->IsStream();
        if( bStream )
        {
            strBase = "CStreamProxyWrapper"; 
        }
        else
        {
            strBase = "CAggInterfaceProxy"; 
        }

        INDENT_UP;
        NEW_LINE;
        if( bStream )
            CCOUT<< ": public " << strBase;
        else
            CCOUT<< ": public virtual " << strBase;

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;

        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";

        INDENT_UP;
        NEW_LINE;
        if( m_pNode->IsStream() )
        {
            CCOUT << "super::super( pCfg ), "
                << "super( pCfg )";
            NEW_LINE;
            CCOUT << "{}";
        }
        else
        {
            CCOUT << "super( pCfg )";
            NEW_LINE;
            CCOUT << "{}";
        }
        INDENT_DOWN;
        NEW_LINE;
        Wa( "gint32 InitUserFuncs();" );
        NEW_LINE;
        Wa( "const EnumClsid GetIid() const override" );
        CCOUT << "{ return iid( "
            << strName << " ); }";
        NEW_LINES( 2 );
        Wa( "// Dispatch the Json request messages" );
        CCOUT << "gint32 DispatchIfReq(" 
            << "    const Json::Value& oReq );";
        NEW_LINES( 2 );

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        pmds->GetCount();
        guint32 dwCount = pmds->GetCount();
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pmd = pObj;
            if( pmd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( pmd->IsEvent() )
            {
                ret = OutputEvent( pmd );
                if( ERROR( ret ) )
                    break;
            }
            // FUSE support does not have synchronous
            // method calls
            ret = OutputAsync( pmd );
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfProxyFuse::OutputEvent(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;

    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        guint32 dwCount = GetArgCount( pArgs );
        CCOUT<< "//RPC event handler '" << strName <<"'";
        NEW_LINE;
        Wa( "//TODO: implement me" );
        std::string strDecl =
            std::string( "virtual gint32 " ) +
            strName + "(";
        CCOUT << strDecl;
        if( dwCount > 0 )
        {
            NEW_LINE;
            CCOUT << "    const Json::Value& oEvent ) = 0;";
        }
        else
        {
            CCOUT << ") = 0;";
        }

        pmd->SetAbstDecl( strDecl );
        pmd->SetAbstFlag( MF_IN_ARG | MF_IN_AS_IN );

        if( pmd->IsSerialize() && dwCount > 0 )
        { 
            NEW_LINE;
            // deserialization before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName <<"Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback, BufPtr& pBuf_ );";
            INDENT_DOWN;
        }
        else if( dwCount == 0 )
        {
            NEW_LINE;
            // deserialization method before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper( IEventSink* pCallback );";
            NEW_LINE;
            CCOUT << "{ return this->" << strName << "(); }";
        }
        else if( !pmd->IsSerialize() )
        {
            break;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxyFuse::OutputAsync(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount = GetArgCount( pInArgs );
        guint32 dwOutCount = GetArgCount( pOutArgs );
        bool bNoReply = pmd->IsNoReply();

        Wa( "//RPC Async Req Sender" );
        CCOUT << "gint32 " << strName << "( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IConfigDb* context,";
        NEW_LINE;
        CCOUT << "const Json::Value& oJsReq,";
        NEW_LINE;
        CCOUT << "Json::Value& oJsResp );";
        INDENT_DOWN;

        if( pmd->IsSerialize() && dwInCount > 0 )
        {
            NEW_LINES( 2 );
            CCOUT << "gint32 " << strName
                << "Dummy( BufPtr& pBuf_ )";
            NEW_LINE;
            CCOUT << "{ return STATUS_SUCCESS; }";
        }

        NEW_LINES(2);
        Wa( "//Async callback wrapper" );
        CCOUT << "gint32 " << strName
            << "CbWrapper" << "( ";
        INDENT_UPL;

        Wa( "IEventSink* pCallback, " );
        Wa( "IEventSink* pIoReq," );
        CCOUT <<  "IConfigDb* pReqCtx );";

        INDENT_DOWNL;
        NEW_LINE;

        Wa( "//RPC Async Req Callback" );
        Wa( "//TODO: implement me by adding" );
        Wa( "//response processing code" );

        std::string strDecl;
        strDecl += "virtual gint32 " + strName
            + "Callback" + "(";
        CCOUT << strDecl;
        strDecl += "IConfigDb* context, gint32 iRet";

        if( dwOutCount > 0 )
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet,";
            strDecl += ",";
            CCOUT << "const Json::Value& oResp";
            CCOUT << " ) = 0;";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet ) = 0;";
            INDENT_DOWN;
        }

        pmd->SetAbstDecl( strDecl );
        pmd->SetAbstFlag(
            MF_OUT_ARG | MF_OUT_AS_IN );

        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

CDeclInterfSvrFuse::CDeclInterfSvrFuse(
    CCppWriter* pWriter, ObjPtr& pNode )
    : super( pWriter )
{
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "interface declaration" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CDeclInterfSvrFuse::Output()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_SImpl";
        CCOUT << strClass;
        std::string strBase;
        bool bStream = m_pNode->IsStream();
        if( bStream )
        {
            strBase = "CStreamServerWrapper"; 
        }
        else
        {
            strBase = "CAggInterfaceServer"; 
        }

        INDENT_UP;
        NEW_LINE;
        if( bStream )
            CCOUT<< ": public " << strBase;
        else
            CCOUT<< ": public virtual " << strBase;

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;
        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";
        INDENT_UP;
        NEW_LINE;
        if( m_pNode->IsStream() )
        {
            CCOUT << "super::super( pCfg ), "
                << "super( pCfg )";
            NEW_LINE;
            CCOUT << "{}";
        }
        else
        {
            CCOUT << "super( pCfg )";
            NEW_LINE;
            CCOUT << "{}";
        }
        INDENT_DOWN;
        NEW_LINE;
        Wa( "gint32 InitUserFuncs();" );
        NEW_LINE;
        Wa( "const EnumClsid GetIid() const override" );
        CCOUT << "{ return iid( "
            << strName << " ); }";
        NEW_LINES( 2 );
        Wa( "// Dispatch the Json response/event messages" );
        CCOUT << "gint32 DispatchIfMsg(" 
            << "    const Json::Value& oMsg );";
        NEW_LINES( 2 );

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        guint32 dwCount = pmds->GetCount();
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pmd = pObj;
            if( pmd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( pmd->IsEvent() )
            {
                ret = OutputEvent( pmd );
                if( ERROR( ret ) )
                    break;
            }
            ret = OutputAsync( pmd );
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfSvrFuse::OutputEvent(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;

    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        Wa( "//RPC event sender" );
        CCOUT << "gint32 " << strName << "(";
        guint32 dwInCount = GetArgCount( pArgs );
        if( dwInCount > 0 )
        {
            NEW_LINE;
            CCOUT << "    const Json::Value& oEvent );";
        }
        else
        {
            CCOUT << " );";
        }

        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvrFuse::OutputAsync(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;
        if( dwInCount == 0 )
        {
            Wa( "//RPC Async Req Handler wrapper" );
            CCOUT << "gint32 "
                << strName << "Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback );";
            INDENT_DOWNL;
            NEW_LINE;

            CCOUT << "gint32 "
                << strName << "CancelWrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback,";
            NEW_LINE;
            Wa( "gint32 iRet," );
            CCOUT << "IConfigDb* pReqCtx_ );";
            INDENT_DOWNL;
            NEW_LINE;
        }
        else if( pmd->IsSerialize() )
        {
            Wa( "//RPC Async Req Handler wrapper" );
            CCOUT << "gint32 "
                << strName << "Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback"
                << ", BufPtr& pBuf_ );";
            INDENT_DOWNL;
            NEW_LINE;

            CCOUT << "gint32 "
                << strName << "CancelWrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback,";
            NEW_LINE;
            Wa( "gint32 iRet," );
            CCOUT << "IConfigDb* pReqCtx_, BufPtr& pBuf_ );";
            INDENT_DOWNL;
            NEW_LINE;
        }
        else
        {
            ret = ERROR_NOT_IMPL;
            break;
        }

        Wa( "//RPC Async Req Cancel Handler" );
        CCOUT << "virtual gint32 "
            << "On" << strName << "Canceled(";
        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_, gint32 iRet";
        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            CCOUT << "const Json::Value& oReq";
        }
        CCOUT << " ) = 0;";
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "//RPC Async Req Callback" );
        Wa( "//Call this method when you have" );
        Wa( "//finished the async operation" );
        Wa( "//with all the return value set" );
        Wa( "//or an error code" );

        CCOUT << "virtual gint32 " << strName
            << "Complete" << "( ";

        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_, "
            << "gint32 iRet";

        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            CCOUT << "const Json::Value& oResp";
        }
        CCOUT << " );";
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "//RPC Async Req Handler" );
        Wa( "//TODO: adding code to emit your async" );
        Wa( "//operation, keep a copy of pCallback and" );
        Wa( "//return STATUS_PENDING" );
        std::string strDecl =
            std::string( "virtual gint32 " ) +
            strName + "(";

        CCOUT << strDecl;
        strDecl += "IConfigDb* pReqCtx_";
        if( dwInCount > 0 )
            strDecl += ",";

        INDENT_UPL;
        CCOUT << strDecl;
        NEW_LINE;
        if( dwInCount > 0 )
            CCOUT << "const Json::Value& oReq";

        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            CCOUT << "Json::Value& oResp";
        }

        CCOUT << " ) = 0;";
        INDENT_DOWNL;

        pmd->SetAbstDecl( strDecl, false );

        guint32 dwFlags = 0;
        if( dwInCount > 0 )
            dwFlags = MF_IN_AS_IN | MF_IN_ARG;

        pmd->SetAbstFlag( dwFlags, false );

    }while( 0 );

    return ret;
}

gint32 CDeclServiceImplFuse::Output()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();

        std::vector< ABSTE > vecPMethods;
        ret = FindAbstMethod( vecPMethods, true );
        if( ERROR( ret ) )
            break;

        std::vector< ABSTE > vecSMethods;
        ret = FindAbstMethod( vecSMethods, false );
        if( ERROR( ret ) )
            break;
        
        if( vecSMethods.empty() &&
            vecPMethods.empty() )
            break;

        Wa( "//Generated by ridlc" );
        Wa( "//Your task is to implement the following classes" );
        Wa( "//to get your rpc server work" );
        Wa( "#pragma once" );
        CAstNodeBase* pParent =
            m_pNode->GetParent();
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CStatements* pStmts = ObjPtr( pParent );
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        std::string strAppName = pStmts->GetName();
        CCOUT << "#include \"" << strAppName << ".h\"" ;
        NEW_LINES( 2 );

        std::string strClass, strBase;
        if( !vecPMethods.empty() && !IsServer() )
        {
            strClass = "C";
            strClass += strSvcName + "_CliImpl";
            strBase = "C";
            strBase += strSvcName + "_CliSkel";
            CCOUT << "class " << strClass;

            INDENT_UP;
            NEW_LINE;

            CCOUT << ": public " << strBase;

            INDENT_DOWN;
            NEW_LINE;
            BLOCK_OPEN;

            Wa( "public:" );
            CCOUT << "typedef "
                << strBase << " super;";
            NEW_LINE;

            CCOUT << strClass
                << "( const IConfigDb* pCfg ) :";
            INDENT_UP;
            NEW_LINE;
            CCOUT << "super::virtbase( pCfg ), "
                << "super( pCfg )";
            INDENT_DOWN;
            NEW_LINE;
            CCOUT << "{}";
            NEW_LINES( 2 );

            Wa( "//Request Dispatcher" );
            Wa( "gint32 DispatchRequest(" );
            Wa( "    IEventSink* pCb," );
            Wa( "    const std::string& strJsReq );" );
            NEW_LINE;
            if( m_pNode->IsStream() )
            {
                Wa( "/* The following 2 methods are important for */" );
                Wa( "/* streaming transfer. rewrite them if necessary */" );
                Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
                Wa( "{ return super::OnStreamReady( hChannel ); } " );
                NEW_LINE;
                Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
                Wa( "{ return super::OnStmClosing( hChannel ); }" );
                NEW_LINE;
            }

            for( guint32 i = 0;
                i < vecPMethods.size(); i++ )
            {
                ABSTE& elem = vecPMethods[ i ];
                if( elem.first[ 0 ] == '/' &&
                    elem.first[ 1 ] == '/' )
                {
                    CCOUT << elem.first;
                    NEW_LINE;
                    continue;
                }
                DeclAbstMethod( elem, true );
                if( i + 1 < vecPMethods.size() )
                    NEW_LINE;
            }

            BLOCK_CLOSE;
            CCOUT << ";";
            NEW_LINES( 2 );
        }

        if( vecSMethods.empty() || !IsServer() )
            break;

        strClass = "C";
        strClass += strSvcName + "_SvrImpl";
        strBase = "C";
        strBase += strSvcName + "_SvrSkel";

        CCOUT << "class " << strClass;

        INDENT_UP;
        NEW_LINE;

        CCOUT << ": public " << strBase;

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;

        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "super::virtbase( pCfg ), "
            << "super( pCfg )";
        INDENT_DOWN;
        NEW_LINE;
        CCOUT << "{}";
        NEW_LINES( 2 );

        Wa( "//Response/Event Dispatcher" );
        Wa( "gint32 DispatchMsg(" );
        Wa( "    const std::string& strJsMsg );" );
        NEW_LINE;

        if( m_pNode->IsStream() )
        {
            Wa( "/* The following 3 methods are important for */" );
            Wa( "/* streaming transfer. rewrite them if necessary */" );
            Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
            Wa( "{ return super::OnStreamReady( hChannel ); } " );
            NEW_LINE;
            Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
            Wa( "{ return super::OnStmClosing( hChannel ); }" );
            NEW_LINE;
            Wa( "gint32 AcceptNewStream(" );
            Wa( "    IEventSink* pCb, IConfigDb* pDataDesc ) override" );
            Wa( "{ return STATUS_SUCCESS; }" );
            NEW_LINE;
        }

        for( guint32 i = 0;
            i < vecSMethods.size(); i++ )
        {
            ABSTE& elem = vecSMethods[ i ];
            if( elem.first[ 0 ] == '/' &&
                elem.first[ 1 ] == '/' )
            {
                CCOUT << elem.first;
                NEW_LINE;
                continue;
            }
            DeclAbstMethod( elem, false );

            CMethodDecl* pmd = elem.second;
            if( !pmd->IsAsyncs() )
            {
                if( i + 1 < vecSMethods.size() )
                    NEW_LINE;
                continue;
            }
            NEW_LINE;
            std::string strName = pmd->GetName();
            ObjPtr pInArgs = pmd->GetInArgs();
            guint32 dwInCount =
                GetArgCount( pInArgs );
            Wa( "// RPC Async Req Cancel Handler" );
            Wa( "// Rewrite this method to release the resources" );
            CCOUT << "gint32 "
                << "On" << strName << "Canceled(";
            INDENT_UPL;
            Wa( "IConfigDb* pReqCtx," );
            CCOUT << "gint32 iRet";
            if( dwInCount > 0 )
            {
                CCOUT << ",";
                NEW_LINE;
                CCOUT << "BufPtr& pBuf_";
            }
            CCOUT << " )";
            INDENT_DOWNL;
            BLOCK_OPEN;
            CCOUT << "DebugPrintEx( logErr, iRet,";
            INDENT_UPL;
            CCOUT << "\"request '" << strName
                << "' is canceled.\" );";
            INDENT_DOWNL;
            CCOUT << "return 0;";
            BLOCK_CLOSE;

            if( i + 1 < vecSMethods.size() )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclServiceImplFuse::DeclAbstMethod(
    ABSTE oMethod, bool bProxy, bool bComma )
{
    CMethodDecl* pmd = oMethod.second;    
    if( pmd == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = 0;

        guint32 dwFlags = 0;
        pmd->GetAbstFlag( dwFlags, bProxy );

        bool bInArg = false, bOutArg = false;

        if( dwFlags & MF_IN_ARG )
            bInArg = true;

        if( dwFlags & MF_OUT_ARG )
            bOutArg = true;

        bool bInAsIn = false, bOutAsIn = false;
        if( dwFlags & MF_IN_AS_IN )
            bInAsIn = true;

        if( dwFlags & MF_OUT_AS_IN )
            bOutAsIn = true;

        if( bInArg )
            dwCount = dwInCount;

        if( bOutArg )
            dwCount += dwOutCount;

        std::string strRet = oMethod.first;
        if( !bComma )
        {
            char szHeader[] =
                "virtual gint32 ";
            strRet = strRet.substr(
                sizeof( szHeader ) - 1 );
        }

        if( dwCount == 0 )
        {
            if( strRet.back() == '(' )
            {
                CCOUT << strRet;
                CCOUT << ")";
                if( bComma )
                    CCOUT << ";";
                NEW_LINE;
                break;
            }
            // there are some fixed arguments
            // to indent
        }

        size_t pos = strRet.find_first_of( '(' );
        bool bAppend = false;
        if( pos != std::string::npos )
        {
            pos += 1;
            INDENT_UP;
            std::string strNewLine =
                m_pWriter->NewLineStr();
            INDENT_DOWN;
            if( pos >= strRet.size() )
            {
                strRet.append( strNewLine );
                bAppend = true;
            }
            else
                strRet.insert( pos, strNewLine );
        }
        else
        {
            printf( "error '%s' missing left paren\n",
                oMethod.first.c_str() );
            ret = -EINVAL;
            break;
        }

        CCOUT << strRet;
        if( dwCount == 0 )
        {
            CCOUT << " )";
            if( bComma )
                CCOUT << ";";
            NEW_LINE;
            break;
        }

        if( !bAppend )
        {
            INDENT_UPL;
        }
        else
        {
            INDENT_UP;
        }
        if( bInArg && bInAsIn ) 
        {
            CCOUT << "const Json::Value& oReq";
        }
        else if( bInArg )
        {
            CCOUT << "Json::Value& oReq";
        }

        if( dwInCount > 0 && dwOutCount > 0 &&
            bInArg && bOutArg )
        {
            CCOUT << ",";
            NEW_LINE;
        }

        if( bOutArg && bOutAsIn )
        {
            CCOUT << "Json::Value& oResp";
        }
        else if( bOutArg )
        {
            CCOUT << "const Json::Value& oResp";
        }
        CCOUT << " )";
        if( bComma )
            CCOUT << ";";

        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CMethodWriterFuse::GenSerialArgs(
    ObjPtr& pArgList,
    const std::string& strBuf,
    bool bDeclare, bool bAssign,
    bool bNoRet )
{
    gint32 ret = 0;
    if( GetArgCount( pArgList ) == 0 )
        return 0;

    do{
        NEW_LINE;
        Wa( "CJsonSerialBase oSerial_(" );
        Wa( "    ObjPtr( this ) );" );

        CEmitSerialCodeFuse oesc(
            m_pWriter, pArgList );

        CCOUT << "do";
        BLOCK_OPEN;
        ret = oesc.OutputSerial(
            "oSerial_", strBuf );
        if( ERROR( ret ) )
            break;
        NEW_LINE;
        CCOUT << strBuf << "->SetOffset( 0 );";
        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );
        CCOUT<< "if( ERROR( ret ) )";
        INDENT_UPL;
        if( bNoRet )
            CCOUT << "break;";
        else
            CCOUT << "return ret;";
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CMethodWriterFuse::GenDeserialArgs(
    ObjPtr& pArgList,
    const std::string& strBuf,
    bool bDeclare, bool bAssign,
    bool bNoRet )
{
    gint32 ret = 0;
    if( GetArgCount( pArgList ) == 0 )
        return 0;

    do{
        NEW_LINE;
        Wa( "CJsonSerialBase oDeserial_(" );
        Wa( "    ObjPtr( this ) );" );
        CEmitSerialCode oedsc(
            m_pWriter, pArgList );

        CCOUT << "do";
        BLOCK_OPEN;
        CCOUT << "guint32 dwOrigOff = "
            << strBuf << "->offset();";
        NEW_LINE;
        ret = oedsc.OutputDeserial(
            "oDeserial_", strBuf );
        if( ERROR( ret ) )
            break;
        NEW_LINE;
        CCOUT << strBuf
            << "->SetOffset( dwOrigOff );";
        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );
        CCOUT<< "if( ERROR( ret ) )";
        INDENT_UPL;
        if( bNoRet )
        {
            Wa( "break;" );
        }
        else
        {
            Wa( "return ret;" );
        }
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplIfMethodProxyFuse::CImplIfMethodProxyFuse(
    CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter )
{
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    CAstNodeBase* pParent = m_pNode->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method_decls' node" );
        throw std::runtime_error( strMsg );
    }
    pParent = pParent->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    m_pIf = ObjPtr( pParent );
    if( m_pIf == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplIfMethodProxyFuse::OutputAsyncCbWrapper()
{
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bSerial = m_pNode->IsSerialize();
    bool bNoReply = m_pNode->IsNoReply();
    if( !bSerial )
        return ERROR_NOT_IMPL;

    do{
        Wa( "//Async callback wrapper" );
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "CbWrapper" << "( ";
        INDENT_UP;
        NEW_LINE;
        Wa( "IEventSink* pCallback, " );
        Wa( "IEventSink* pIoReq," );
        CCOUT << "IConfigDb* pReqCtx )";
        INDENT_DOWNL;
        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;
        NEW_LINE; 
        Wa( "IConfigDb* pResp_ = nullptr;" );
        Wa( "CCfgOpenerObj oReq_( pIoReq );" );
        CCOUT << "ret = oReq_.GetPointer(";
        INDENT_UPL;
        CCOUT << "propRespPtr, pResp_ );";
        INDENT_DOWNL;
        CCOUT << "if( ERROR( ret ) )";
        INDENT_UPL;
        CCOUT << "break;";
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "CCfgOpener oResp_( pResp_ );" );
        Wa( "gint32 iRet = 0;" );

        CCOUT << "ret = oResp_.GetIntProp( ";
        INDENT_UPL;
        CCOUT << "propReturnValue, ( guint32& ) iRet );";
        INDENT_DOWNL;

        CCOUT << "if( ERROR( ret ) ) break;";
        NEW_LINES( 2 );

        Wa( "IConfigDb* context = nullptr;" );
        Wa( "CCfgOpener oReqCtx_( pReqCtx );" );
        Wa( "ret = oReqCtx_.GetPointer( 0, context );" );
        Wa( "if( ERROR( ret ) ) break;" );
        Wa( "CCfgOpener oCtx_( context );" );
        Wa( "guint64 qwReqId = oReqCtx[ 1 ];" );
        Wa( "oCtx_[ propContext ] = qwReqId;" );
        NEW_LINE;

        // gen the param list
        if( dwOutCount == 0 || bNoReply )
        {
             CCOUT << strMethod
                 <<"Callback( context, iRet );";
        }
        else /* need deserialization */
        {
            Wa( "if( SUCCEEDED( iRet ) )" );
            BLOCK_OPEN;
            Wa( "guint32 dwSeriProto_ = 0;" );
            Wa( "ret = oResp_.GetIntProp(" );
            Wa( "    propSeriProto, dwSeriProto_ );" );
            Wa( "if( ERROR( ret ) ||" );
            Wa( "    dwSeriProto_ != seriRidl )" );
            Wa( "    break;" );
            Wa( "BufPtr pBuf_;" );
            Wa( "ret = oResp_.GetBufPtr( 0, pBuf_ );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "Json::Value val_( objectValue );" );
            ret = GenDeserialArgs( pOutArgs,
                "pBuf_", false, false, true );
            if( ERROR( ret ) )
                break;
            BLOCK_CLOSE;

            NEW_LINE;
            CCOUT << "this->" << strMethod <<"Callback(";
            NEW_LINE;
            CCOUT << "    context, iRet, val_ );";
        }
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        NEW_LINE;
        CCOUT << "return 0;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxyFuse::OutputEvent()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwCount = dwInCount;

    bool bSerial = m_pNode->IsSerialize();

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IEventSink* pCallback";
        if( dwCount == 0 )
        {
        }
        else if( bSerial )
        {
            CCOUT << ", BufPtr& pBuf_";
        }
        else
        {
            ret = ERROR_NOT_IMPL;
            break;
        }
        CCOUT << " )";
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        if( dwCount == 0 )
        {
            CCOUT << "return this->"
                << strMethod << "();";
        }
        else if( !bSerial )
        {
            ret = ERROR_NOT_IMPL;
        }
        else
        {
            Wa( "gint32 ret = 0;" );
            Wa( "Json::Value val_( Json::objectValue );" );
            Wa( "do" );
            BLOCK_OPEN;
            ret = GenDeserialArgs(
                pInArgs, "pBuf_", false, true );
            if( ERROR( ret ) )
                break;

            // call the user's handler
            CCOUT << strMethod << "(";

            if( dwCount == 0 )
                CCOUT << ");";
            else
                CCOUT << " val_ );";
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "return ret;";
        }

        BLOCK_CLOSE;
        NEW_LINES( 1 );

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxyFuse::OutputAsync()
{
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bSerial = m_pNode->IsSerialize();
    bool bNoReply = m_pNode->IsNoReply();

    if( !bSerial )
        return ERROR_NOT_IMPL;

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        INDENT_UPL;
        CCOUT << "IConfigDb* context,";
        NEW_LINE;
        CCOUT << "const Json::Value& oJsReq,";
        NEW_LINE;
        CCOUT << " Json::Value& oJsResp )";
        INDENT_DOWNL;

        BLOCK_OPEN;
        Wa( "if( oJsReq.empty() || !oJsReq.isObject() )" );
        Wa( "    return -EINVAL;" );
        Wa( "if( !oJsReq.isMember[ JSON_ATTR_REQCTXID ] ||" );
        Wa( "    !oJsReq[ JSON_ATTR_REQCTXID ].isUInt64() )" );
        Wa( "    return -EINVAL;" );
        Wa( "guint64 qwReqId = " );
        Wa( "    oJsReq[ JSON_ATTR_REQCTXID ];" );
        Wa( "if( !oJsResp.isObject() )" );
        Wa( "    return -EINVAL;" );
        Wa( "CCfgOpener oBackupCtx;" );
        Wa( "if( context == nullptr )" );
        Wa( "    context = oBackupCtx.GetCfg();" );
        Wa( "gint32 ret = 0;" );
        Wa( "TaskletPtr pRespCb_;" );
        CCOUT << "do";
        BLOCK_OPEN;
        Wa( "CParamList oOptions_;" );
        Wa( "CfgPtr pResp_;" );
        CCOUT << "oOptions_[ propIfName ] =";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "DBUS_IF_NAME( \""
            << strIfName << "\" );";
        INDENT_DOWN;
        if( bNoReply )
        {
            NEW_LINE;
            CCOUT << "oOptions_[ propNoReply ] = true;";
        }
        NEW_LINE;
        CCOUT << "oOptions_[ propSeriProto ] = ";
        NEW_LINE;
        CCOUT << "    ( guint32 )seriRidl;";
        guint32 dwTimeoutSec =
            m_pNode->GetTimeoutSec();
        guint32 dwKeepAliveSec =
            m_pNode->GetKeepAliveSec();
        if( dwKeepAliveSec == 0 )
        {
            dwKeepAliveSec =
                ( dwTimeoutSec >> 1 );
        }
        if( dwTimeoutSec > 0 &&
            dwKeepAliveSec > 0 &&
            dwTimeoutSec > dwKeepAliveSec )
        {
            CCOUT << "oOptions_[ propTimeoutSec ] = "
                << dwTimeoutSec << ";"; 
            NEW_LINE;
            CCOUT << "oOptions_[ propKeepAliveSec ] = "
                << dwKeepAliveSec << ";"; 
            NEW_LINE;
        }

        NEW_LINE;

        Wa( "CParamList oReqCtx_;" );
        Wa( "ObjPtr pTemp( context );" );
        Wa( "oReqCtx_.Push( pTemp );" );
        Wa( "oReqCtx_.Push( qwReqId );" );
        NEW_LINE;
        CCOUT << "ret = NEW_PROXY_RESP_HANDLER2(";
        INDENT_UPL;
        CCOUT <<  "pRespCb_, ObjPtr( this ), ";
        NEW_LINE;
        CCOUT << "&" << strClass << "::"
            << strMethod << "CbWrapper, ";
        NEW_LINE;
        CCOUT << "nullptr, oReqCtx_.GetCfg() );";
        INDENT_DOWN;
        NEW_LINES( 2 );
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        if( dwInCount == 0 )
        {
            CCOUT << "ret = this->AsyncCall(";
            INDENT_UPL;
            Wa( "( IEventSink* )pRespCb_, " );
            Wa( "oOptions_.GetCfg(), pResp_," );
            CCOUT << "\"" << strMethod << "\" );";
            INDENT_DOWNL;
        }
        else /* need serialize */
        {
            Wa( "//Serialize the input parameters" );
            Wa( "BufPtr pBuf_( true );" );
            Wa( "Json::Value val_;" );
            Wa( "if( !oJsReq.hasMember( JSON_ATTR_PARAMS)" );
            Wa( "{ ret = -ENOENT; break; }" );
            Wa( "val_ = oJsReq[ JSON_ATTR_PARAMS ];" );
            Wa( "if( val_.empty() || !val_.isObject() )" );
            Wa( "{ ret = -EINVAL; break; }" );

            ret = GenSerialArgs(
                pInArgs, "pBuf_", true, true );
            if( ERROR( ret ) )
                break;

            NEW_LINE;
            CCOUT << "ret = this->AsyncCall(";
            INDENT_UPL;
            Wa( "( IEventSink* )pRespCb_, " );
            Wa( "oOptions_.GetCfg(), pResp_," );
            CCOUT << "\"" << strMethod << "\",";
            NEW_LINE;
            CCOUT << "pBuf_ );";
            INDENT_DOWNL;
        }
        if( !bNoReply )
        {
            NEW_LINE;
            Wa( "CCfgOpener oResp_(" );
            Wa( "    ( IConfigDb* )pResp_ );" );
            Wa( "if( ret == STATUS_PENDING )" );
            BLOCK_OPEN;
            NEW_LINE;
            Wa( "guint64 qwTaskId = oResp_[ propTaskId ];" );
            Wa( "oJsResp[ JSON_ATTR_TASKID ] = qwTaskId;" );
            CCOUT << "CCfgOpener oContext( context );";
            NEW_LINE;
            CCOUT <<"oContext.SetQwordProp(";
            NEW_LINE;
            CCOUT << "    propTaskId, qwTaskId );";
            NEW_LINE;
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "else if( ERROR( ret ) )" );
            CCOUT << "    break;";
            NEW_LINE;
            Wa( "oResp_.GetIntProp(" );
            Wa( "    propReturnValue, ( guint32& )ret );" );
            if( dwOutCount > 0 )
            {
                if( dwInCount == 0 )
                    Wa( "Json::Value val_( objectValue );" );
                else
                    Wa( "val_ = Json::Value( objectValue );" );

                Wa("do" );
                BLOCK_OPEN;
                CCOUT << "if( ERROR( ret ) ) break;";
                NEW_LINE;
                Wa( "guint32 dwSeriProto_ = 0;" );
                Wa( "ret = oResp_.GetIntProp(" );
                Wa( "    propSeriProto, dwSeriProto_ );" );
                Wa( "if( ERROR( ret ) ||" );
                Wa( "    dwSeriProto_ != seriRidl )" );
                Wa( "    break;" );
                Wa( "BufPtr pBuf2;" );
                Wa( "ret = oResp_.GetBufPtr( 0, pBuf2 );" );
                Wa( "if( ERROR( ret ) )" );
                Wa( "    break;" );
                if( ERROR( ret ) )
                    break;

                ret = GenDeserialArgs( pOutArgs,
                    "pBuf2", true, true, true );
                if( ERROR( ret ) )
                    break;
                BLOCK_CLOSE;
                Wa( "while( 0 );" );
            }
            if( dwOutCount > 0 )
            {
                Wa( "if( !ERROR( ret ) && !val_.empty() )" );
                Wa( "    oJsResp[ JSON_ATTR_PARAMS ] = val_;" );
            }
        }
        else
        {
            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING )" );
            CCOUT << "    break;";
        }
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        Wa( "oJsResp[ JSON_ATTR_RETCODE ]= ret;" );
        Wa( "if( ERROR( ret ) && !pRespCb_.IsEmpty() )" );
        Wa( "    ( *pRespCb_ )( eventCancelTask );" );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxyFuse::Output()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEvent();
            break;
        }
        else if( m_pNode->IsAsyncp() )
        {
            ret = OutputAsync();
            if( ERROR( ret ) )
                break;

            ret = OutputAsyncCbWrapper();
        }
        else
        {
            ret = ERROR_NOT_IMPL;
        }

    }while( 0 );

    return ret;
}

CImplIfMethodSvrFuse::CImplIfMethodSvrFuse(
    CCppWriter* pWriter, ObjPtr& pNode )
    : super( pWriter )
{
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    CAstNodeBase* pParent = m_pNode->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method_decls' node" );
        throw std::runtime_error( strMsg );
    }
    pParent = pParent->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    m_pIf = ObjPtr( pParent );
    if( m_pIf == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplIfMethodSvrFuse::Output()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEvent();
            break;
        }
        else if( m_pNode->IsAsyncs() )
        {
            ret = OutputAsync();
            if( ERROR( ret ) )
                break;

            ret = OutputAsyncCallback();
        }
        else
        {
            ret = ERROR_NOT_IMPL;
        }

    }while( 0 );

    return ret;
}

gint32 CImplIfMethodSvrFuse::OutputEvent()
{
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );

    bool bSerial = m_pNode->IsSerialize();
    if( !bSerial )
        return ERROR_NOT_IMPL;

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "(";
        INDENT_UPL;

        // gen the param list
        if( dwInCount == 0 )
        {
            CCOUT << ")";
        }
        else
        {
            CCOUT << " const Json::Value& oJsEvt )";
        }

        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "if( oJsEvt.empty() ||" ); 
        Wa( "    oJsEvt.isObject() )" );
        Wa( "    return -EINVAL;" );
        Wa( "CParamList oOptions_;" );
        CCOUT << "oOptions_[ propSeriProto ] = ";
        NEW_LINE;
        CCOUT << "    ( guint32 )seriRidl;";
        NEW_LINE;

        if( 0 == dwInCount )
        {
            CCOUT << "ret = this->SendEventEx(";
            INDENT_UPL;
            Wa( "nullptr, oOptions_.GetCfg()," );
            CCOUT << "iid( "<< strIfName << " ), ";
            NEW_LINE;
            CCOUT << "USER_EVENT( \"" << strMethod << "\" ), \"\" ) ";
            INDENT_DOWN;
        }
        else /* need serialize */
        {
            Wa( "do" );
            BLOCK_OPEN;
            Wa( "//Serialize the input parameters" );
            Wa( "BufPtr pBuf_( true );" );
            Wa( "if( !oJsEvt.hasMember(" );
            Wa( "    JSON_ATTR_PARAMS ) )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oJsEvt[ JSON_ATTR_PARAMS ].isObject() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "Json::Value val_ =" );
            Wa( "    oJsEvt[ JSON_ATTR_PARAMS ];" );
            ret = GenSerialArgs(
                pInArgs, "pBuf_", true, true );
            if( ERROR( ret ) )
                break;

            CCOUT << "ret = this->SendEventEx(";
            INDENT_UPL;
            Wa( "nullptr, oOptions_.GetCfg()," );
            CCOUT << "iid( "<< strIfName << " ), ";
            NEW_LINE;
            CCOUT << "USER_EVENT( \"" << strMethod << "\" ), \"\",";
            NEW_LINE;
            CCOUT << "pBuf_ );";
            INDENT_DOWN;
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
        }
        NEW_LINES( 2 );
        CCOUT << "return ret;";

        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvrFuse::OutputAsync()
{
    gint32 ret = 0;

    bool bSerial = m_pNode->IsSerialize();
    if( !bSerial )
        return ERROR_NOT_IMPL;

    ret = OutputAsyncSerial();
    if( ERROR( ret ) )
        return ret;

    return OutputAsyncCancelWrapper();
}

gint32 CImplIfMethodSvrFuse::OutputAsyncSerial()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bNoReply = m_pNode->IsNoReply();
    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UPL;
        if( dwInCount > 0 )
            CCOUT << "IEventSink* pCallback"
                << ", BufPtr& pBuf_ )";
        else
            CCOUT << "IEventSink* pCallback )";

        INDENT_DOWNL;
        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        Wa( "TaskletPtr pNewCb;" );
        CCOUT << "do";
        BLOCK_OPEN;

        guint32 dwTimeoutSec =
            m_pNode->GetTimeoutSec();

        guint32 dwKeepAliveSec =
            m_pNode->GetKeepAliveSec();
        if( dwKeepAliveSec == 0 )
        {
            dwKeepAliveSec =
                ( dwTimeoutSec >> 1 );
        }

        if( dwTimeoutSec > 0 &&
            dwKeepAliveSec > 0 &&
            dwTimeoutSec > dwKeepAliveSec )
        {
            CCOUT << "ret = SetInvTimeout( pCallback, "
                << dwTimeoutSec << ", "
                << dwKeepAliveSec << " );";
            NEW_LINE;
            Wa( "if( ERROR( ret ) ) break;" );
        }
        else
        {
            Wa( "ret = SetInvTimeout( pCallback, 0 );" );
            Wa( "if( ERROR( ret ) ) break;" );
        }

        Wa( "CParamList oReqCtx_;" );
        Wa( "oReqCtx_.SetPointer(" );
        Wa( "    propEventSink, pCallback );" );
        Wa( "IConfigDb* pReqCtx_ = oReqCtx_.GetCfg();" );

        CCOUT << "ret = DEFER_CANCEL_HANDLER2(";
        INDENT_UPL;
        CCOUT << "-1, pNewCb, this,";
        NEW_LINE;
        CCOUT << "&" << strClass << "::"
            << strMethod << "CancelWrapper,";
        NEW_LINE;
        if( dwInCount > 0 )
            CCOUT << "pCallback, 0, pReqCtx_, pBuf_ );";
        else
            CCOUT << "pCallback, 0, pReqCtx_ );";
        INDENT_DOWNL;
        NEW_LINE;
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        if( dwInCount > 0 )
        {
            Wa( "Json::Value val_( objectValue );" );

            ret = GenDeserialArgs(
                pInArgs, "pBuf_",
                false, false, true );
            if( ERROR( ret ) )
                break;
        }

        if( dwOutCount > 0 )
        {
            Wa( "Json::Value oJsResp_( objectValue );" );
        }

        // call the user's handler
        CCOUT << "ret = "
            << strMethod << "(";
        NEW_LINE;
        CCOUT << "    pReqCtx_";
        if( dwInCount > 0 )
            CCOUT << ", val_";

        if( dwOutCount > 0 )
            CCOUT << ", oJsResp";

        CCOUT << " );";
        if( !bNoReply )
        {
            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING ) break;" );
            NEW_LINE;
            Wa( "CParamList oResp_;" );
            Wa( "oResp_[ propReturnValue ] = ret;" );
            Wa( "oResp_[ propSeriProto ] = seriRidl;" );
            if( dwOutCount > 0 )
            {
                Wa( "if( SUCCEEDED( ret ) )" );
                BLOCK_OPEN;
                Wa( "val_ = oJsResp;" );
                Wa( "BufPtr pBuf2( true );" );
                ret = GenSerialArgs(
                    pOutArgs, "pBuf2", false,
                    false, true );
                CCOUT << "oResp_.Push( pBuf2 );";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            NEW_LINE;
            CCOUT << "this->SetResponse( pCallback,";
            INDENT_UPL;
            CCOUT << "oResp_.GetCfg() );";
            INDENT_DOWNL;
        }
        BLOCK_CLOSE; // do
        Wa( "while( 0 );" );
        NEW_LINE;
        CCOUT << "if( ret != STATUS_PENDING &&";
        INDENT_UPL;
        CCOUT << "!pNewCb.IsEmpty() )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        CCOUT << "CIfRetryTask* pTask = pNewCb;";
        NEW_LINE;
        CCOUT << "pTask->ClearClientNotify();";
        NEW_LINE;
        CCOUT << "if( pCallback != nullptr )";
        NEW_LINE;
        CCOUT << "    rpcf::RemoveInterceptCallback(";
        NEW_LINE;
        CCOUT << "        pTask, pCallback );";
        NEW_LINE;
        CCOUT << "( *pNewCb )( eventCancelTask );";
        BLOCK_CLOSE;
        NEW_LINE;
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvrFuse::OutputAsyncCallback()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();
    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bSerial = m_pNode->IsSerialize();
    bool bNoReply = m_pNode->IsNoReply();
    if( !bSerial )
        return ERROR_NOT_IMPL;

    do{
        Wa( "// call me when you have completed" );
        Wa( "// the asynchronous operation" );

        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Complete( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IConfigDb* pReqCtx_"
            << ", gint32 iRet";

        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            CCOUT << "    const Json::Value& oJsResp";
        }

        CCOUT << " )";

        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "IEventSink* pCallback = nullptr;" );
        Wa( "CParamList oParams( pReqCtx_ );" );
        Wa( "ret = oParams.GetPointer(" );
        Wa( "    propEventSink, pCallback );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    return ret;" );
        if( bNoReply )
        {
            Wa( "pCallback->OnEvent( eventTaskComp, iRet, 0, 0 );" );
            Wa( "oParams.Clear();" );
            CCOUT << "if( SUCCEEDED( iRet ) ) return iRet;";
            NEW_LINE;
            CCOUT << "DebugPrint( iRet,";
            INDENT_UPL;
            CCOUT << "\"'"<< strMethod
                << "' completed with error\" );";
            INDENT_DOWNL;
            CCOUT << "return iRet;";
            BLOCK_CLOSE;
            NEW_LINE;
            break;
        }

        CCOUT << "if( iRet == STATUS_PENDING )";
        NEW_LINE;
        CCOUT << "    ret = ERROR_STATE;";
        NEW_LINE;
        CCOUT << "else";
        NEW_LINE;
        CCOUT << "    ret = iRet;";
        NEW_LINE;
        NEW_LINE;
        Wa( "CParamList oResp_;" );
        Wa( "oResp_[ propReturnValue ] = ret;" );
        Wa( "oResp_[ propSeriProto ] = seriRidl;" );

        if( dwOutCount > 0 )
        {
            Wa( "while( SUCCEEDED( ret ) )" );
            BLOCK_OPEN;
            Wa( "BufPtr pBuf_( true );" );
            Wa( "Json::Value val_ =" );
            Wa( "    oJsResp[ JSON_ATTR_PARAMS ];" );
            ret = GenSerialArgs(
                pOutArgs, "pBuf_", true, true );
            if( ERROR( ret ) )
                break;
            Wa( "oResp_.Push( pBuf_ );" );
            CCOUT << "break;";
            BLOCK_CLOSE;
        }
        NEW_LINE;

        CCOUT << "this->OnServiceComplete( ";
        INDENT_UPL;
        CCOUT << "( IConfigDb* )oResp_.GetCfg(), pCallback );";
        INDENT_DOWNL;
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvrFuse::OutputAsyncCancelWrapper()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );

    bool bSerial = m_pNode->IsSerialize();
    if( !bSerial )
        return ERROR_NOT_IMPL;

    do{
        Wa( "// this method is called when" );
        Wa( "// timeout happens or user cancels" );
        Wa( "// this pending request" );

        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "CancelWrapper(";
        NEW_LINE;
        CCOUT << "    IEventSink* pCallback, gint32 iRet";
        NEW_LINE;
        CCOUT << "    iRet, IConfigDb* pReqCtx_";
        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            CCOUT << "    BufPtr& pBuf_";
        }
        CCOUT << " )";
        NEW_LINE;

        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );

        CCOUT << "do";
        BLOCK_OPEN;

        if( dwInCount > 0 )
        {
            Wa( "Json::Value val_;" );
            ret = GenDeserialArgs( pInArgs,
                "pBuf_", false, false, true );
            if( ERROR( ret ) )
                break;
        }

        // call the user's handler
        CCOUT << "On" << strMethod
            << "Canceled( pReqCtx_,";

        if( dwInCount == 0 )
        {
            CCOUT << " iRet );";
        }
        else
        {
            CCOUT << "iRet, val_ );";
        }

        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}
