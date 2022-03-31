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

gint32 EmitBuildJsonReq( 
    CWriterBase* m_pWriter )
{
    Wa( "gint32 BuildJsonReq(" );
    Wa( "    IConfigDb* pReqCtx_," );
    Wa( "    const Json::Value& oJsParams," );
    Wa( "    const std::string& strMethod," );
    Wa( "    const std::string& strIfName, " );
    Wa( "    gint32 iRet," );
    Wa( "    std::string& strReq," );
    Wa( "    bool bProxy," );
    Wa( "    bool bResp )" );
    BLOCK_OPEN;
    Wa( "gint32 ret = 0;" );
    Wa( "if( pReqCtx_ == nullptr )" );
    Wa( "    return -EINVAL;" );
    Wa( "do" );
    BLOCK_OPEN;
    Wa( "CCfgOpener oReqCtx( pReqCtx_ );" );
    Wa( "IEventSink* pCallback = nullptr;" );
    Wa( "guint64 qwReqId = 0;" );
    Wa( "if( bProxy )" );
    BLOCK_OPEN;
    Wa( "ret = oReqCtx.GetQwordProp(" );
    Wa( "    1, qwReqId );" );
    CCOUT << "if( ERROR( ret ) ) break;";
    BLOCK_CLOSE;
    CCOUT << "else";
    NEW_LINE;
    BLOCK_OPEN;
    Wa( "ret = oReqCtx.GetPointer( " );
    Wa( "    propEventSink, pCallback );" );
    Wa( "if( ERROR( ret ) ) break;" );
    CCOUT << "qwReqId = pCallback->GetObjId();";
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "Json::Value oJsReq( objectValue );" );
    Wa( "if( SUCCEEDED( iRet ) &&" );
    Wa( "    !oJsParams.empty() &&" );
    Wa( "    oJsParams.isObject() )" );
    Wa( "    oJsReq[ JSON_ATTR_PARAMS ] = oJsParams;" );
    Wa( "oJsReq[ JSON_ATTR_METHOD ] = strMethod;" );
    Wa( "oJsReq[ JSON_ATTR_IFNAME ] = strIfName;" );
    Wa( "if( bProxy )" );
    BLOCK_OPEN;
    Wa( "if( bResp )" );
    Wa( "{" );
    Wa( "    oJsReq[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
    Wa( "    oJsReq[ JSON_ATTR_RETCODE ] = iRet;");
    Wa( "}" );
    Wa( "else" );
    Wa( "    oJsReq[ JSON_ATTR_MSGTYPE ] = \"evt\";" );
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "else" );
    Wa( "    oJsReq[ JSON_ATTR_MSGTYPE ] = \"req\";" );

    Wa( "oJsReq[ JSON_ATTR_REQCTXID ] = qwReqId;" );
    NEW_LINE;
    Wa( "Json::StreamWriterBuilder oBuilder;" );
    Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
    CCOUT <<  "strReq = Json::writeString( "
        << "oBuilder, oJsReq );";
    BLOCK_CLOSE;
    Wa( "while( 0 );" );
    CCOUT << "return ret;";
    BLOCK_CLOSE;
    NEW_LINE;
    return 0;
}

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

            CCOUT << "if( !val_.isMember( \""
                << strName << "\" ) )";
            NEW_LINE;
            Wa( "{ ret = -ENOENT; break; }" );
            CCOUT << "_oMember = val_[ \""
                << strName << "\" ];";
            NEW_LINE;

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
                    Wa( "if( !_oMember.isString() )" );
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
                    Wa( "if( !_oMember.isString() )" );
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
                    Wa( "if( !_oMember.isString() )" );
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

        Wa( "Value _oMember, _oVal;" );

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
                    NEW_LINE;
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
                    CCOUT << "ret = " << strObj
                        << "DeserializeHStream( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'Q':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeUInt64( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'q':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeUInt64( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oVal );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    Wa( "_oMember = _oVal.asInt64();" );
                    break;
                }
            case 'D':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeUInt( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'd':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeUInt( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    Wa( "_oMember = _oVal.asInt();" );
                    break;
                }
            case 'W':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeUShort( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'w':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeUShort( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    Wa( "_oMember = _oVal.asInt();" );
                    break;
                }
            case 'f':
               {
                    CCOUT << "ret = " << strObj
                        << "DeserializeFloat( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'F':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeDouble( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'b':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeBool( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'B':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeByte( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 's':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeString( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'a':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeByteArray( ";
                    NEW_LINE;
                    CCOUT << "    " <<  strBuf
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'o':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeObjPtr( ";
                    NEW_LINE;
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
            std::string strMsgId = "clsid( ";
            strMsgId += strName + " )";
            vecMsgIds.push_back( strMsgId );
        }

        if( vecMsgIds.empty() )
        {
            Wa( "std::set< guint32 > g_setMsgIds;" );
            break;
        }

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
        NEW_LINE;
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
        NEW_LINE;
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
        NEW_LINE;
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

        Wa( "ret = CSerialBase::Serialize(" );
        Wa( "    pBuf_, m_dwMsgId );" );
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
        CCOUT << "ret = CSerialBase::Deserialize(";
        NEW_LINE;
        CCOUT << "    pBuf_, dwMsgId );";
        NEW_LINE;
        Wa( "if( ERROR( ret ) ) return ret;" );
        Wa( "if( m_dwMsgId != dwMsgId )" );
        Wa( "    return -EINVAL;" );
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
        Wa( "virtual gint32 OnReqComplete( IEventSink*," );
        Wa( "    IConfigDb* pReqCtx, Json::Value& val_," );
        Wa( "    const std::string&, const std::string&, gint32 ) = 0;" );
        NEW_LINE;
        Wa( "const EnumClsid GetIid() const override" );
        CCOUT << "{ return iid( "
            << strName << " ); }";
        NEW_LINES( 2 );
        Wa( "// Dispatch the Json request messages" );
        Wa( "gint32 DispatchIfReq(" );
        Wa( "    IConfigDb* pContext," );
        Wa( "    const Json::Value& oReq," );
        Wa( "    Json::Value& oResp );" );
        NEW_LINE;

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

    gint32 ret = 0;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        if( !pmd->IsSerialize() )
        {
            ret = ERROR_NOT_IMPL;
            break;
        }

        guint32 dwCount = GetArgCount( pArgs );
        CCOUT<< "//RPC event handler '" << strName <<"'";
        NEW_LINE;
        std::string strDecl =
            std::string( "virtual gint32 " ) +
            strName + "(";

        pmd->SetAbstDecl( strDecl );
        pmd->SetAbstFlag( MF_IN_ARG | MF_IN_AS_IN );

        if( dwCount == 0 )
        {
            NEW_LINE;
            // deserialization method before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper( IEventSink* pCallback );";
        }
        else
        { 
            NEW_LINE;
            // deserialization before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName <<"Wrapper(";
            NEW_LINE;
            CCOUT << "    IEventSink* pCallback, BufPtr& pBuf_ );";
        }
        NEW_LINE;

    }while( 0 );

    return ret;
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

        std::string strDecl;
        strDecl += "virtual gint32 " + strName
            + "Callback( IConfigDb* context, gint32 iRet";

        if( dwOutCount > 0 )
        {
            strDecl.push_back( ',' );
        }

        pmd->SetAbstDecl( strDecl );
        pmd->SetAbstFlag(
            MF_OUT_ARG | MF_OUT_AS_IN );

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
        Wa( "gint32 DispatchIfMsg(" );
        Wa( "    const Json::Value& oMsg );" );
        NEW_LINE;

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
            else
            {
                ret = OutputAsync( pmd );
                if( ERROR( ret ) )
                    break;
            }
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

        if( !pmd->IsSerialize() )
        {
            ret = ERROR_NOT_IMPL;
            break;
        }

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
        else
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

        std::string strDecl =
            std::string( "virtual gint32 " ) +
            strName + "(";

        strDecl += "IConfigDb* pReqCtx_";
        if( dwInCount > 0 )
            strDecl += ",";

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

            Wa( "std::hashmap< guint64, guint64 >" );
            Wa( "    m_mapReq2Task, m_mapTask2Req;" );
            Wa( "gint32 inline AddReq(" );
            Wa( "    guint64 qwReq, guint64 qwTask )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrReq = m_mapReq2Task.find( qwReq );" );
            Wa( "auto itrTask = m_mapTask2Req.find( qwTask );" );
            Wa( "if( itrReq != m_mapReq2Task.end() ||" );
            Wa( "    itrTask != m_mapTask2Req.end() )" );
            Wa( "    return -EEXIST;" );
            Wa( "m_mapReq2Task[ qwReq ] = qwTask;" );
            Wa( "m_mapTask2Req[ qwTask ] = qwReq;" );
            Wa( "return STATUS_SUCCESS;" );
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 GetReqId(" );
            Wa( "    guint64 qwTask, guint64& qwReq )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrTask = m_mapTask2Req.find( qwTask );" );
            Wa( "if( itrTask == m_mapTask2Req.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "qwReq = itrTask->second;" );
            Wa( "return STATUS_SUCCESS;" );
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 GetTaskId(" );
            Wa( "    guint64 qwReq, guint64& qwTask )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrReq = m_mapReq2Task.find( qwReq );" );
            Wa( "if( itrReq == m_mapReq2Task.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "qwTask = itrReq->second;" );
            Wa( "return STATUS_SUCCESS;" );
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 RemoveReq( guint64 qwReq )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrReq = m_mapReq2Task.find( qwReq );" );
            Wa( "if( itrReq == m_mapReq2Task.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "guint64 qwTask = itrReq->second;" );
            Wa( "m_mapReq2Task.erase( itrReq );" );
            Wa( "m_mapTask2Req.erase( qwTask );" );
            Wa( "return STATUS_SUCCESS;" );
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 RemoveTask(" );
            Wa( "    guint64 qwTask )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrTask = m_mapTask2Req.find( qwTask );" );
            Wa( "if( itrTask == m_mapTask2Req.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "guint64 qwReq = itrTask->second;" );
            Wa( "m_mapReq2Task.erase( qwReq );" );
            Wa( "m_mapTask2Req.erase( itrTask );" );
            Wa( "return STATUS_SUCCESS;" );
            BLOCK_CLOSE;
            NEW_LINE;

            CCOUT << "gint32 OnEvent( EnumEventId iEvent,";
            NEW_LINE;
            Wa( "    LONGWORD dwParam1," );
            Wa( "    LONGWORD dwParam2," );
            Wa( "    LONGWORD* pData ) override;" );

            Wa( "//Request Dispatcher" );
            Wa( "gint32 DispatchReq(" );
            Wa( "    IConfigDb* pContext," );
            Wa( "    const Json::Value& valReq," );
            Wa( "    Json::Value& valResp ) override;" );
            NEW_LINE;
            Wa( "gint32 CancelRequestByReqId(" );
            Wa( "    guint64 qwReqId );" );

            NEW_LINE;
            Wa( "gint32 OnReqComplete(" );
            Wa( "    IEventSink* pCallback," );
            Wa( "    IConfigDb* pReqCtx," );
            Wa( "    Json::Value& valResp," );
            Wa( "    const std::string& strIfName," );
            Wa( "    const std::string& strMethod," );
            Wa( "    gint32 iRet ) override;" );
            NEW_LINE;

            Wa( "/* The following 2 methods are important for */" );
            Wa( "/* streaming transfer. rewrite them if necessary */" );
            Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
            BLOCK_OPEN;
            Wa( "gint32 ret = super::OnStreamReady( hChannel );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    return ret;" );
            CCOUT << "return OnStreamReadyFuse( hChannel );";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
            BLOCK_OPEN;
            Wa( "OnStmClosingFuse( hChannel );" );
            CCOUT << "return super::OnStmClosing( hChannel );";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "gint32 OnReadStreamComplete( HANDLE hChannel," );
            Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
            Wa( "{ return OnReadStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
            NEW_LINE;

            Wa( "gint32 OnWriteStreamComplete( HANDLE hChannel," );
            Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
            Wa( "{ return OnWriteStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
            NEW_LINE;

            Wa( "gint32 OnWriteResumed( HANDLE hChannel ) override" );
            CCOUT << "{ return OnWriteResumedFuse( hChannel ); } ";

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

        Wa( "//Response&Event Dispatcher" );
        Wa( "gint32 DispatchMsg(" );
        CCOUT << "    const Json::Value& valMsg ) override;";

        NEW_LINE;
        Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
        BLOCK_OPEN;
        Wa( "gint32 ret = super::OnStreamReady( hChannel );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    return ret;" );
        CCOUT << "return OnStreamReadyFuse( hChannel );";
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
        BLOCK_OPEN;
        Wa( "OnStmClosingFuse( hChannel );" );
        CCOUT << "return super::OnStmClosing( hChannel );";
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "gint32 AcceptNewStream(" );
        Wa( "    IEventSink* pCb, IConfigDb* pDataDesc ) override" );
        Wa( "{ return CFuseSvcServer::AcceptNewStreamFuse( pCb, pDataDesc ); }");
        NEW_LINE;

        Wa( "gint32 OnReadStreamComplete( HANDLE hChannel," );
        Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
        Wa( "{ return OnReadStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
        NEW_LINE;

        Wa( "gint32 OnWriteStreamComplete( HANDLE hChannel," );
        Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
        Wa( "{ return OnWriteStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
        NEW_LINE;

        Wa( "gint32 OnWriteResumed( HANDLE hChannel ) override" );
        CCOUT << "{ return OnWriteResumedFuse( hChannel ); } ";

        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

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
        CEmitSerialCodeFuse oedsc(
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
            CCOUT << "break;";
        }
        else
        {
            CCOUT << "return ret;";
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
        Wa( "IConfigDb* pResp_ = nullptr;" );
        Wa( "CCfgOpenerObj oReq_( pIoReq );" );
        CCOUT << "ret = oReq_.GetPointer(";
        NEW_LINE;
        CCOUT << "    propRespPtr, pResp_ );";
        NEW_LINE;
        CCOUT << "if( ERROR( ret ) )";
        NEW_LINE;
        CCOUT << "    break;";
        NEW_LINE;
        NEW_LINE;

        Wa( "CCfgOpener oResp_( pResp_ );" );
        Wa( "gint32 iRet = 0;" );

        CCOUT << "ret = oResp_.GetIntProp( ";
        INDENT_UPL;
        CCOUT << "propReturnValue, ( guint32& ) iRet );";
        INDENT_DOWNL;

        CCOUT << "if( ERROR( ret ) ) break;";
        NEW_LINES( 2 );

        // gen the param list
        Wa( "Json::Value val_( objectValue );" );
        if( dwOutCount > 0 )
        {
            /* need deserialization */
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
            ret = GenDeserialArgs( pOutArgs,
                "pBuf_", false, false, true );
            if( ERROR( ret ) )
                break;
            BLOCK_CLOSE;
            NEW_LINE;

        }
        CMethodDecls* pmds =
            ObjPtr( m_pNode->GetParent() );
        CInterfaceDecl* pifd =
            ObjPtr( pmds->GetParent() );

        Wa( "OnReqComplete( pCallback, pReqCtx, val_," ); 
        CCOUT << "    \"" << m_pNode->GetName() << "\",";
        NEW_LINE;
        CCOUT << "    \"" << pifd->GetName() << "\",";
        NEW_LINE;
        CCOUT << "    iRet );";
        NEW_LINE;

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
    if( !bSerial )
        return  ERROR_NOT_IMPL;

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IEventSink* pCallback";
        if( dwCount > 0 )
            CCOUT << ", BufPtr& pBuf_";

        CCOUT << " )";
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        Wa( "do" );
        BLOCK_OPEN;
        Wa( "Json::Value val_( Json::objectValue );" );
        if( dwCount > 0 )
        {
            ret = GenDeserialArgs(
                pInArgs, "pBuf_", false, true );
            if( ERROR( ret ) )
                break;
        }

        CMethodDecls* pmds =
            ObjPtr( m_pNode->GetParent() );
        CInterfaceDecl* pifd =
            ObjPtr( pmds->GetParent() );

        Wa( "stdstr strEvent;" );
        Wa( "CCfgOpener oReqCtx;" );
        Wa( "guint32 qwReqId = pCallback->GetObjId();" );
        Wa( "oReqCtx.SetQwordProp( 1, qwReqId );" );
        Wa( "ret = BuildJsonReq( oReqCtx.GetCfg(), val_," ); 
        CCOUT << "    \"" << m_pNode->GetName() << "\",";
        NEW_LINE;
        CCOUT << "    \"" << pifd->GetName() << "\",";
        NEW_LINE;
        Wa( "    0, strEvent, true, false );" );
        Wa( "//TODO: pass strEvent to FUSE" );
        Wa( "CFuseSvcProxy* pFuse = ObjPtr( this );" );
        Wa( "pFuse->ReceiveEvtJson( strEvent );" );
        Wa( "if( ERROR( ret ) ) break;" );

        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        NEW_LINE;
        CCOUT << "return ret;";

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
        Wa( "if( !oJsReq.isMember( JSON_ATTR_REQCTXID ) ||" );
        Wa( "    !oJsReq[ JSON_ATTR_REQCTXID ].isUInt64() )" );
        Wa( "    return -EINVAL;" );
        Wa( "guint64 qwReqId = " );
        Wa( "    oJsReq[ JSON_ATTR_REQCTXID ].asUInt64();" );
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
        Wa( "ObjPtr pT_ = this;" );
        NEW_LINE;
        CCOUT << "ret = NEW_PROXY_RESP_HANDLER2(";
        INDENT_UPL;
        CCOUT <<  "pRespCb_, pT_, ";
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
        else
        {
            Wa( "//Serialize the input parameters" );
            Wa( "BufPtr pBuf_( true );" );
            Wa( "Json::Value val_;" );
            Wa( "if( !oJsReq.isMember( JSON_ATTR_PARAMS) )" );
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
        NEW_LINE;
        Wa( "CCfgOpener oResp_(" );
        Wa( "    ( IConfigDb* )pResp_ );" );
        if( !bNoReply )
        {
            Wa( "if( ret == STATUS_PENDING )" );
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = oResp_[ propTaskId ];" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] = qwReqId;" );
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
            Wa( "if( ret == STATUS_PENDING )" );
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = oResp_[ propTaskId ];" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] = qwReqId;" );
            CCOUT << "CCfgOpener oContext( context );";
            NEW_LINE;
            CCOUT <<"oContext.SetQwordProp(";
            NEW_LINE;
            CCOUT << "    propTaskId, qwTaskId );";
            NEW_LINE;
            CCOUT << "break;";
            BLOCK_CLOSE;
        }
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        Wa( "// oJsResp[ JSON_ATTR_RETCODE ]= ret;" );
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
        else
        {
            ret = OutputAsync();
            if( ERROR( ret ) )
                break;

            ret = OutputAsyncCbWrapper();
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
        else
        {
            ret = OutputAsync();
            if( ERROR( ret ) )
                break;

            ret = OutputAsyncCallback();
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
            Wa( "if( !oJsEvt.isMember(" );
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

        Wa( "Json::Value val_( objectValue );" );
        if( dwInCount > 0 )
        {
            ret = GenDeserialArgs(
                pInArgs, "pBuf_",
                false, false, true );
            if( ERROR( ret ) )
                break;
        }

        if( dwOutCount > 0 )
            Wa( "Json::Value oJsResp_( objectValue );" );

        Wa( "stdstr strReq;" );
        Wa( "ret = BuildJsonReq( pReqCtx_, val_," );
        CCOUT << "    \"" << strMethod << "\",";
        NEW_LINE;
        CCOUT << "    \"" << m_pIf->GetName() << "\",";
        NEW_LINE;
        Wa( "    0, strReq, false, false );" );
        Wa( "if( ERROR( ret ) ) break;" );
        CCOUT << "//TODO: pass strReq to FUSE";
        NEW_LINE;
        Wa( "CFuseSvcServer* pFuse = ObjPtr( this );" );
        Wa( "pFuse->ReceiveMsgJson( strReq," );
        CCOUT << "    pCallback->GetObjId() );";

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
                Wa( "val_ = oJsResp_[ JSON_ATTR_PARAMS ];" );
                Wa( "BufPtr pBuf2( true );" );
                ret = GenSerialArgs(
                    pOutArgs, "pBuf2", false,
                    false, true );
                CCOUT << "oResp_.Push( pBuf2 );";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            NEW_LINE;
            Wa( "this->SetResponse( pCallback," );
            CCOUT << "    oResp_.GetCfg() );";
        }
        BLOCK_CLOSE; // do
        NEW_LINE;
        Wa( "while( 0 );" );
        NEW_LINE;
        CCOUT << "if( ret != STATUS_PENDING &&";
        INDENT_UPL;
        CCOUT << "!pNewCb.IsEmpty() )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "CIfRetryTask* pTask = pNewCb;" );
        Wa( "pTask->ClearClientNotify();" );
        Wa( "if( pCallback != nullptr )" );
        Wa( "    rpcf::RemoveInterceptCallback(" );
        Wa( "        pTask, pCallback );" );
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
        CCOUT << "    IEventSink* pCallback, gint32 iRet,";
        NEW_LINE;
        CCOUT << "    IConfigDb* pReqCtx_";
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

        Wa( "//TODO: clean up the FUSE resources if any" );

        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplServiceImplFuse::Output()
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

        Wa( "/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/" );
        Wa( "// Implement the following methods" );
        Wa( "// to get the RPC proxy/server work" );
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
        Wa( "#include \"rpc.h\"" );
        Wa( "using namespace rpcf;" );
        CCOUT << "#include \""
            << strAppName << ".h\"" ;
        NEW_LINE;
        if( IsServer() )
            CCOUT << "#include \""
                << strSvcName << "svr.h\"";
        else
            CCOUT << "#include \""
                << strSvcName << "cli.h\"";

        NEW_LINES( 2 );

        std::string strClass, strBase;

        if( IsServer() )
        {
            strClass = "C";
            strClass += strSvcName + "_SvrImpl";

            // implement the DispatchMsg
            CCOUT << "gint32 " << strClass
                << "::DispatchMsg(";
            NEW_LINE;
            Wa( "    const Json::Value& oMsg )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "if( oMsg.empty() || !oMsg.isObject() )" );
            Wa( "    return -EINVAL;" );
            Wa( "do" );
            BLOCK_OPEN;
            Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||");
            Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
            Wa( "if( strType != \"resp\" &&" );
            Wa( "    strType != \"evt\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_IFNAME ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_IFNAME ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strIfName =" );
            Wa( "    oMsg[ JSON_ATTR_IFNAME ].asString();" );
            std::vector< ObjPtr > vecIfRefs;
            m_pNode->GetIfRefs( vecIfRefs );
            for( auto& elem : vecIfRefs )
            {
                CInterfRef* pifr = elem;
                ObjPtr pObj;
                pifr->GetIfDecl( pObj );
                CInterfaceDecl* pifd = pObj;
                stdstr strIfName = pifd->GetName();
                CCOUT << "if( strIfName == \""
                    << strIfName << "\" )";
                BLOCK_OPEN;
                CCOUT << "ret = " << "I" << strIfName
                    << "_SImpl::DispatchIfMsg( oMsg );";
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;
        }
        else
        {
            strClass = "C";
            strClass += strSvcName + "_CliImpl";
            Wa( "#ifdef ADD_REQTASK" );
            Wa( "#undef ADD_REQTASK" );
            Wa( "#endif" );
            Wa( "#define ADD_REQTASK \\" );
            CCOUT << "{\\";
            INDENT_UPL;
            Wa( "TaskGrpPtr pGrp;\\" );
            Wa( "ret = this->GetParallelGrp( pGrp );\\" );
            Wa( "if( ERROR( ret ) )\\" );
            Wa( "    break;\\" );
            Wa( "TaskletPtr pTask;\\" );
            Wa( "ret = pGrp->FindTask( qwTaskId, pTask );\\" );
            Wa( "if( ERROR( ret ) )\\" );
            Wa( "    break;\\" );
            Wa( "CIfParallelTask* pParaTask = pTask;\\" );
            Wa( "if( pParaTask == nullptr )\\" );
            Wa( "{ ret = -EFAULT; break;}\\" );
            Wa( "CStdRTMutex oTaskLock(\\" );
            Wa( "    pParaTask->GetLock() );\\" );
            Wa( "if( pParaTask->GetTaskState() != stateStarted )\\" );
            Wa( "{ ret = ERROR_STATE; break; }\\" );
            CCOUT << "AddReq( qwReqId, qwTaskId); }";
            INDENT_DOWN;
            NEW_LINES( 2 );

            CCOUT << "gint32 " << strClass
                << "::OnEvent( EnumEventId iEvent,";
            NEW_LINE;
            Wa( "    LONGWORD dwParam1," );
            Wa( "    LONGWORD dwParam2," );
            Wa( "    LONGWORD* pData )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "switch( ( guint32 )iEvent )" );
            BLOCK_OPEN;
            Wa( "case eventRemoveReq:" );
            BLOCK_OPEN;
            Wa( "IConfigDb* pCfg = ( IConfigDb* )pData;" );
            Wa( "if( pCfg == nullptr )" );
            Wa( "{ ret = -EFAULT; break; }" );
            Wa( "CCfgOpener oCfg( pCfg );" );
            Wa( "guint64 qwReqId = 0;" );
            Wa( "ret = oCfg.GetQwordProp( 1, qwReqId );" );
            Wa( "if( ERROR( ret ) ) break;" );
            Wa( "RemoveReq( qwReqId );" );
            Wa( "break;" );
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "default:" );
            Wa( "    ret = super::OnEvent( iEvent," );
            Wa( "        dwParam1, dwParam1, pData );" );
            CCOUT << "    break;";
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;
            // implement the DispatchReq
            CCOUT << "gint32 " << strClass
                << "::DispatchReq(";
            NEW_LINE;
            Wa( "    IConfigDb* pContext," );
            Wa( "    const Json::Value& oReq," );
            Wa( "    Json::Value& oResp )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "if( oReq.empty() || !oReq.isObject() )" );
            Wa( "    return -EINVAL;" );
            Wa( "do" );
            BLOCK_OPEN;
            Wa( "if( !oReq.isMember( JSON_ATTR_METHOD ) ||");
            Wa( "    !oReq[ JSON_ATTR_METHOD ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strMethod =" );
            Wa( "    oReq[ JSON_ATTR_METHOD ].asString();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_IFNAME ) ||");
            Wa( "    !oReq[ JSON_ATTR_IFNAME ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strIfName =" );
            Wa( "    oReq[ JSON_ATTR_IFNAME ].asString();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_MSGTYPE ) ||");
            Wa( "    !oReq[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oReq[ JSON_ATTR_MSGTYPE ].asString();" );
            Wa( "if( strType != \"req\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oReq.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !oReq[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 qwReqId =" );
            Wa( "    oReq[ JSON_ATTR_REQCTXID ].asUInt64();" );
            Wa( "// init the response value" );
            Wa( "oResp[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
            Wa( "oResp[ JSON_ATTR_METHOD ] = strMethod;" );
            Wa( "oResp[ JSON_ATTR_IFNAME ] = strIfName;" );
            Wa( "oResp[ JSON_ATTR_REQCTXID ] = qwReqId;" );
            Wa( "CParamList oCtx_( pContext );" );
            std::vector< ObjPtr > vecIfRefs;
            m_pNode->GetIfRefs( vecIfRefs );
            for( auto& elem : vecIfRefs )
            {
                CInterfRef* pifr = elem;
                ObjPtr pObj;
                pifr->GetIfDecl( pObj );
                CInterfaceDecl* pifd = pObj;
                stdstr strIfName = pifd->GetName();
                CCOUT << "if( strIfName == \""
                    << strIfName << "\" )";
                BLOCK_OPEN;
                CCOUT << "ret = " << "I" << strIfName
                    << "_PImpl::DispatchIfReq( ";
                NEW_LINE;
                CCOUT << "    oCtx_.GetCfg(), oReq, oResp );";
                NEW_LINE;
                Wa( "if( ret == STATUS_PENDING )" );
                BLOCK_OPEN;
                Wa( "guint64 qwTaskId = oCtx_[ propTaskId ];" );
                Wa( "ADD_REQTASK;" );
                BLOCK_CLOSE;
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            Wa( "if( strIfName == \"IInterfaceServer\" )" );
            BLOCK_OPEN;
            Wa( "if( strMethod != \"UserCancelRequest\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oReq.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !oReq[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 qwReqId =" );
            Wa( "    oReq[ JSON_ATTR_REQCTXID ].asUInt64();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_PARAMS ) ||");
            Wa( "    !oReq[ JSON_ATTR_PARAMS ].isObject() ||" );
            Wa( "    oReq[ JSON_ATTR_PARAMS ].empty() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "Json::Value val_ = oReq[ JSON_ATTR_PARAMS ];" );
            Wa( "if( !val_.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !val_[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "ret = CancelRequestByReqId( qwReqId );" );
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "ret = -ENOENT;";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            Wa( "oResp[ JSON_ATTR_RETCODE ] = ret;" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;

            // implement the CancelRequestByReqId
            CCOUT << "gint32 " << strClass
                << "::CancelRequestByReqId( ";
            NEW_LINE;
            CCOUT << "    guint64 qwReqId )";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "do" );
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = 0;" );
            Wa( "ret = this->GetTaskId( qwReqId, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "gint32 (*func)( CRpcServices* pIf, IEventSink*, guint64) =" );
            Wa("    ([]( CRpcServices* pIf, IEventSink* pThis, guint64 qwReqId )->gint32" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "do" );
            BLOCK_OPEN;
            Wa( "IConfigDb* pResp = nullptr;" );
            Wa( "CCfgOpenerObj oCfg( pThis );" );
            Wa( "ret = oCfg.GetPointer(" );
            Wa( "    propRespPtr, pResp );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CCfgOpener oResp( pResp );" );
            Wa( "gint32 iRet = 0;" );
            Wa( "ret = oResp.GetIntProp(" );
            Wa( "    propReturnValue, ( guint32& )iRet );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "Json::Value oJsResp( objectValue );" );
            Wa( "oJsResp[ JSON_ATTR_IFNAME ] =" );
            Wa( "    \"IInterfaceServer\";" );
            Wa( "oJsResp[ JSON_ATTR_METHOD ] =" );
            Wa( "    \"UserCancelRequest\";" );
            Wa( "oJsResp[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
            Wa( "oJsResp[ JSON_ATTR_RETCODE ] = iRet;" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] = qwReqId;" );
            Wa( "Json::StreamWriterBuilder oBuilder;" );
            Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
            CCOUT <<  "stdstr strResp = Json::writeString( "
                << "oBuilder, oJsResp );";
            NEW_LINE;
            CCOUT << strClass << "* pClient = ObjPtr( pIf );";
            NEW_LINE;
            Wa( "pClient->ReceiveMsgJson( strResp, qwReqId );" );
            Wa( "pClient->RemoveReq( qwReqId );" );
            CCOUT << "";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( ");" );
            Wa( "TaskletPtr pCb;" );
            Wa( "ret = NEW_FUNCCALL_TASK( pCb," );
            Wa( "    this->GetIoMgr(), func," );
            Wa( "    this, nullptr, qwReqId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CDeferredFuncCallBase< CIfRetryTask >*" );
            Wa( "    pCall = pCb;" );
            Wa( "ObjPtr pObj( pCall );" );
            Wa( "Variant oArg0( pObj );" );
            Wa( "pCall->UpdateParamAt( 1, oArg0 );" );
            NEW_LINE;
            Wa( "ret = this->CancelReqAsync(" );
            Wa( "    pCb, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "   ( *pCb )( eventCancelTask );" );
            Wa( "break;" );
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;

            NEW_LINE;
            Wa( "extern gint32 BuildJsonReq(" );
            Wa( "    IConfigDb* pReqCtx_," );
            Wa( "    const Json::Value& oJsParams," );
            Wa( "    const std::string& strMethod," );
            Wa( "    const std::string& strIfName, " );
            Wa( "    gint32 iRet," );
            Wa( "    std::string& strReq," );
            Wa( "    bool bProxy," );
            Wa( "    bool bResp );" );
            NEW_LINE;

            // implement OnReqComplete
            CCOUT << "gint32 " << strClass;
            Wa( "::OnReqComplete(" );
            Wa( "    IEventSink* pCallback," );
            Wa( "    IConfigDb* pReqCtx," );
            Wa( "    Json::Value& valResp," );
            Wa( "    const std::string& strIfName," );
            Wa( "    const std::string& strMethod," );
            Wa( "    gint32 iRet )" );
            BLOCK_OPEN;
            Wa( "UNREFERENCED( pCallback );" );
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "stdstr strReq;" );
            Wa( "ret = BuildJsonReq( pReqCtx, valResp," ); 
            Wa( "    strMethod, strIfName, iRet," );
            Wa( "    strReq, true, true );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "LONGWORD* pData = ( LONGWORD* )pReqCtx;" );
            Wa( "this->OnEvent( eventRemoveReq, 0, 0, pData );" );
            Wa( "CCfgOpener oReqCtx_( pReqCtx );" );
            Wa( "guint64 qwReqId = 0;" );
            Wa( "ret = oReqCtx_.GetQwordProp( 1, qwReqId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "this->ReceiveMsgJson( strReq, qwReqId );" );
            Wa( "this->RemoveReq( qwReqId );" );
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;
        }

    }while( 0 );

    return ret;
}

gint32 CImplIufProxyFuse::OutputDispatch()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        strClass += m_pNode->GetName() + "_PImpl";
        std::string strIfName = m_pNode->GetName();
        CCOUT << "gint32 " << strClass
            << "::" << "DispatchIfReq(";
        NEW_LINE;
        Wa( "    IConfigDb* pContext," );
        Wa( "    const Json::Value& oReq," );
        CCOUT << "    Json::Value& oResp )";
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        Wa( "do" );
        BLOCK_OPEN;
        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        pmds->GetCount();
        guint32 dwCount = pmds->GetCount();

        Wa( "if( !oReq.isMember( JSON_ATTR_METHOD ) ||");
        Wa( "    !oReq[ JSON_ATTR_METHOD ].isString() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "stdstr strMethod =" );
        Wa( "    oReq[ JSON_ATTR_METHOD ].asString();" );
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
                continue;

            ObjPtr pInArgs = pmd->GetInArgs();
            guint32 dwInCount = GetArgCount( pInArgs );
            ObjPtr pOutArgs = pmd->GetOutArgs();
            guint32 dwOutCount = GetArgCount( pOutArgs );

            CCOUT << "if( strMethod == \""
                << pmd->GetName() << "\" )";
            NEW_LINE;
            BLOCK_OPEN;
            CCOUT << "ret = this->" << pmd->GetName() <<"(";
            NEW_LINE;
            Wa( "    pContext, oReq, oResp );" );
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
        }
        Wa( "ret = -ENOENT;" );
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplIufSvrFuse::OutputDispatch()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        strClass += m_pNode->GetName() + "_SImpl";
        std::string strIfName = m_pNode->GetName();
        CCOUT << "gint32 " << strClass
            << "::" << "DispatchIfMsg(";
        NEW_LINE;
        CCOUT << "    const Json::Value& oMsg )";
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        Wa( "do" );
        BLOCK_OPEN;
        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        pmds->GetCount();
        guint32 dwCount = pmds->GetCount();

        Wa( "if( !oMsg.isMember( JSON_ATTR_METHOD ) ||");
        Wa( "    !oMsg[ JSON_ATTR_METHOD ].isString() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "stdstr strMethod =" );
        Wa( "    oMsg[ JSON_ATTR_METHOD ].asString();" );
        Wa( "if( !oMsg.isMember( JSON_ATTR_RETCODE ) ||");
        Wa( "    !oMsg[ JSON_ATTR_RETCODE ].isUInt() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "gint32 iRet =" );
        Wa( "    oMsg[ JSON_ATTR_RETCODE ].asInt();" );
        Wa( "if( !oMsg.isMember( JSON_ATTR_REQCTXID ) ||");
        Wa( "    !oMsg[ JSON_ATTR_REQCTXID ].isUInt64() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "guint64 qwTaskId =" );
        Wa( "    oMsg[ JSON_ATTR_REQCTXID ].asUInt64();" );
        Wa( "TaskGrpPtr pGrp;" );
        Wa( "ret = this->GetParallelGrp( pGrp );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        Wa( "TaskletPtr pTask;" );
        Wa( "ret = pGrp->FindTask( qwTaskId, pTask );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        Wa( "CParamList oReqCtx;" );
        Wa( "oReqCtx[ propEventSink ] = ObjPtr( pTask );" );

        Wa( "stdstr strType =" );
        Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pmd = pObj;
            if( pmd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ObjPtr pInArgs = pmd->GetInArgs();
            guint32 dwInCount = GetArgCount( pInArgs );
            ObjPtr pOutArgs = pmd->GetOutArgs();
            guint32 dwOutCount = GetArgCount( pOutArgs );
            CCOUT << "if( strMethod == \""
                << pmd->GetName() << "\" )";
            NEW_LINE;
            BLOCK_OPEN;
            if( !pmd->IsEvent() && !pmd->IsNoReply() )
            {
                Wa( "if( strType != \"resp\" )" );
                Wa( "{ ret = -EINVAL; break; }" );
                CCOUT << "ret = " << strClass << "::"
                    << pmd->GetName() <<"Complete(";
                NEW_LINE;
                CCOUT << "    oReqCtx.GetCfg(), iRet";
                if( dwOutCount > 0 ) 
                    CCOUT << ", oMsg";
                CCOUT << " );";
                NEW_LINE;
                Wa( "if( ret == STATUS_PENDING )" );
                Wa( "    ret = STATUS_SUCCESS;" );
            }
            else if( pmd->IsEvent() )
            {
                Wa( "if( strType != \"evt\" )" );
                Wa( "{ ret = -EINVAL; break; }" );
                CCOUT << "ret = " << strClass << "::"
                    << pmd->GetName() <<"(";
                if( dwInCount > 0 ) 
                    CCOUT << " oMsg );";
                else
                    CCOUT << ");";
                NEW_LINE;
                Wa( "if( ret == STATUS_PENDING )" );
                Wa( "    ret = STATUS_SUCCESS;" );
            }
            else
            {
                Wa( "// no reply" );
                Wa( "ret = -EINVAL;" );
            }
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
        }
        Wa( "ret = -ENOENT;" );
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

CImplMainFuncFuse::CImplMainFuncFuse(
    CCppWriter* pWriter,
    ObjPtr& pNode,
    bool bProxy )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    m_bProxy = bProxy;
}

extern std::string g_strTarget;
gint32 CImplMainFuncFuse::Output()
{
    gint32 ret = 0;

    do{
        CStatements* pStmts = m_pNode;
        std::vector< ObjPtr > vecSvcs;
        pStmts->GetSvcDecls( vecSvcs );
        std::string strModName = g_strTarget;
        stdstr strSuffix;
        if( m_bProxy )
            strSuffix = "cli";
        else
            strSuffix = "svr";
        {
            bool bProxy = ( strSuffix == "cli" );
            if( bProxy )
            {
                m_pWriter->SelectMainCli();
            }
            else
            {
                m_pWriter->SelectMainSvr();
            }

            Wa( "// Generated by ridlc" );
            Wa( "#include \"rpc.h\"" );
            Wa( "#include \"proxy.h\"" );
            Wa( "#include \"fuseif.h\"" );
            Wa( "using namespace rpcf;" );
            for( auto elem : vecSvcs )
            {
                CServiceDecl* pSvc = elem;
                if( bProxy )
                {
                    CCOUT << "#include \""
                        << pSvc->GetName() << "cli.h\"";
                }
                else
                {
                    CCOUT << "#include \""
                        << pSvc->GetName() << "svr.h\"";
                }
                NEW_LINE;
            }
            NEW_LINES( 1 );
            Wa( "ObjPtr g_pIoMgr;" );
            NEW_LINE;

            // InitContext
            Wa( "extern FactoryPtr InitClassFactory();" );
            Wa( "gint32 InitContext()" );
            BLOCK_OPEN;
            Wa( "gint32 ret = CoInitialize( 0 );" );
            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "return ret;";
            INDENT_DOWNL;

            CCOUT << "do";
            BLOCK_OPEN;
            CCOUT << "// load class factory for '"
                << strModName << "'";
            NEW_LINE;
            CCOUT << "FactoryPtr p = "
                << "InitClassFactory();";
            NEW_LINE;
            CCOUT << "ret = CoAddClassFactory( p );";
            NEW_LINE;
            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;
            NEW_LINE;
            Wa( "CParamList oParams;" );
            CCOUT << "oParams.Push( \""
                << strModName + strSuffix << "\" );";
            NEW_LINES( 2 );

            Wa( "// adjust the thread number if necessary" );
            if( bProxy )
            {
                Wa( "oParams[ propMaxIrpThrd ] = 0;" );
                Wa( "oParams[ propMaxTaskThrd ] = 1;" );
            }
            else
            {
                Wa( "oParams[ propMaxIrpThrd ] = 2;" );
                Wa( "oParams[ propMaxTaskThrd ] = 2;" );
            }
            NEW_LINE;

            CCOUT << "ret = g_pIoMgr.NewObj(";
            INDENT_UPL;
            CCOUT << "clsid( CIoManager ), ";
            NEW_LINE;
            CCOUT << "oParams.GetCfg() );";
            INDENT_DOWNL;
            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;
            NEW_LINE;
            CCOUT << "IService* pSvc = g_pIoMgr;";
            NEW_LINE;
            CCOUT << "ret = pSvc->Start();";
            NEW_LINE;
            BLOCK_CLOSE;
            CCOUT << "while( 0 );";
            NEW_LINES( 2 );
            Wa( "if( ERROR( ret ) )" );
            BLOCK_OPEN;
            CCOUT << "g_pIoMgr.Clear();";
            NEW_LINE;
            CCOUT << "CoUninitialize();";
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            // DestroyContext
            Wa( "gint32 DestroyContext()" );
            BLOCK_OPEN;
            Wa( "IService* pSvc = g_pIoMgr;" );
            Wa( "if( pSvc != nullptr )" );
            BLOCK_OPEN;
            Wa( "pSvc->Stop();" );
            CCOUT << "g_pIoMgr.Clear();";
            BLOCK_CLOSE;
            NEW_LINES( 2 );
            Wa( "CoUninitialize();" );

            CCOUT << "DebugPrintEx( logErr, 0,";
            INDENT_UPL;
            CCOUT << "\"#Leaked objects is %d\",";
            NEW_LINE;
            CCOUT << "CObjBase::GetActCount() );";
            INDENT_DOWNL;
            CCOUT << "return STATUS_SUCCESS;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            ObjPtr pRoot( m_pNode );
            CImplClassFactory oicf(
                m_pWriter, pRoot, !bProxy );

            ret = oicf.Output();
            if( ERROR( ret ) )
                break;
            NEW_LINE;

            // main function
            Wa( "int main( int argc, char** argv)" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "ret = InitContext();" );
            CCOUT << "if( ERROR( ret ) )";
            NEW_LINE;
            CCOUT << "    break;";
            NEW_LINE;
            Wa( "fuse_args args = FUSE_ARGS_INIT(argc, argv);" );
            Wa( "fuse_cmdline_opts opts;" );
            Wa( "ret = fuseif_daemonize( args, opts, argc, argv );" );
            CCOUT << "if( ERROR( ret ) )";
            NEW_LINE;
            CCOUT << "    break;";
            NEW_LINES( 2 );
            CCOUT << "ret = InitRootIf( g_pIoMgr, "
                << ( bProxy ? "true" : "false" ) << " );";
            NEW_LINE;
            CCOUT << "if( ERROR( ret ) )";
            NEW_LINE;
            CCOUT << "    break;";
            NEW_LINE;
            Wa( "CRpcServices* pRoot = GetRootIf();" );
            for( auto& elem : vecSvcs )
            {
                CServiceDecl* pSvc = elem;
                stdstr strSvcName = pSvc->GetName();

                std::string strClass =
                    std::string( "C" ) + strSvcName;
                if( bProxy )
                    strClass += "_CliImpl";
                else
                    strClass += "_SvrImpl";

                CCOUT << "// add the "
                    << strSvcName << " directory";
                NEW_LINE;
                CCOUT << "ret = AddSvcPoint(";
                NEW_LINE;
                CCOUT << "    \""
                    << strSvcName << "\",";
                NEW_LINE;
                CCOUT << "    \"./"
                    << g_strAppName << "desc.json\",";
                NEW_LINE;
                CCOUT << "    clsid( " << strClass << " ),";
                NEW_LINE;
                CCOUT << "    " 
                    << ( bProxy ? "true" : "false" )
                    << " );";
                NEW_LINE;
                CCOUT << "if( ERROR( ret ) )";
                NEW_LINE;
                CCOUT << "    break;";
                NEW_LINE;
            }
            NEW_LINE;
            Wa( "args = FUSE_ARGS_INIT(argc, argv);" );
            Wa( "ret = fuseif_main( args, opts );" );

            NEW_LINE;
            Wa( "// Stop the root object" );
            CCOUT << "pRoot->Stop();";
            NEW_LINE;
            BLOCK_CLOSE;
            CCOUT<< "while( 0 );";
            NEW_LINES( 2 );
            Wa( "DestroyContext();" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES(2);
        }

    }while( 0 );

    return ret;
}
