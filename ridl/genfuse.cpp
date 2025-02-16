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
extern bool g_bMklib;
extern bool g_bBuiltinRt;
extern stdstr g_strCmdLine;

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
    CCOUT << "do";
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
    Wa( "Json::Value oJsReq( Json::objectValue );" );
    Wa( "if( SUCCEEDED( iRet ) &&" );
    Wa( "    !oJsParams.empty() &&" );
    Wa( "    oJsParams.isObject() )" );
    Wa( "    oJsReq[ JSON_ATTR_PARAMS ] = oJsParams;" );
    Wa( "oJsReq[ JSON_ATTR_METHOD ] = strMethod;" );
    Wa( "oJsReq[ JSON_ATTR_IFNAME1 ] = strIfName;" );
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

    Wa( "oJsReq[ JSON_ATTR_REQCTXID ] =" );
    Wa( "    ( Json::UInt64& )qwReqId;" );
    NEW_LINE;
    Wa( "Json::StreamWriterBuilder oBuilder;" );
    Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
    Wa( "oBuilder[\"indentation\"] = \"\";" );
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
        Wa( "Json::Value _oMember;" );
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
                    Wa( "if( !_oMember.isString() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        <<"SerializeHStream( ";
                    NEW_LINE;
                    CCOUT << "    " << strBuf
                        << ", _oMember );";
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
            case 'v':
                {
                    Wa( "if( !_oMember.isObject() )" );
                    Wa( "{ ret = -EINVAL; break; }" );
                    CCOUT << "ret = " << strObj
                        << "SerializeVariant(";
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

        Wa( "Json::Value _oMember;" );

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
                    NEW_LINE;
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
                    Wa( "if( ERROR( ret ) )" );
                    Wa( "    break;" );
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
                        <<", _oMember );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
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
            case 'v':
                {
                    CCOUT << "ret = " << strObj
                        << "DeserializeVariant( ";
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
            if( i + 1 < dwCount )
                NEW_LINE;
            // CCOUT << "_oMember = 0;";
            // if( i + 1 < dwCount )
            //    NEW_LINE;
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
        if( !g_bMklib )
        {
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
        }
        else
        {
            Wa( "extern std::set< guint32 > g_setMsgIds; " );
            Wa( "void InitMsgIds() " );
            if( vecMsgIds.empty() )
            {
                Wa( "{}" );
                break;
            }
            BLOCK_OPEN;
            size_t i = 0, count = vecMsgIds.size();
            for( ;i < count; i++ )
            {
                Wa( "g_setMsgIds.insert( " );
                CCOUT << "    " <<  vecMsgIds[ i ] << " );";
                if( i < count - 1 ) 
                    NEW_LINE;
            }
            BLOCK_CLOSE;
            NEW_LINE;
        }

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
        Wa( "    BufPtr& pBuf, const Json::Value& val_ ) override;" );
        NEW_LINE;
        Wa( "gint32 JsonDeserialize(" );
        CCOUT << "    BufPtr& pBuf, Json::Value& val_ ) override;";
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
        Wa( "    BufPtr& pBuf_, const Json::Value& srcVal )" );
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
        Wa( "    pBuf_, SERIAL_STRUCT_MAGIC );" );
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        Wa( "ret = CSerialBase::Serialize(" );
        Wa( "    pBuf_, m_dwMsgId );" );
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        CCOUT << "if( !srcVal.isMember( JSON_ATTR_FIELDS ) )";
        NEW_LINE;
        Wa( "{ ret = -ENOENT; break; }" );
        CCOUT << "const Json::Value& val_ = srcVal[ JSON_ATTR_FIELDS ];";
        NEW_LINE;
        Wa( "if( !val_.isObject() )" );
        Wa( "{ ret = -EINVAL; break; }" );

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
        NEW_LINE;
        CCOUT << "    BufPtr& pBuf_, Json::Value& destVal )";
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

        Wa( "guint32 dwMagicId = 0;" );
        CCOUT << "ret = CSerialBase::Deserialize(";
        NEW_LINE;
        CCOUT << "    pBuf_, dwMagicId );";
        NEW_LINE;
        Wa( "if( ERROR( ret ) ) return ret;" );
        Wa( "if( dwMagicId != SERIAL_STRUCT_MAGIC )" );
        Wa( "    return -EINVAL;" );
        NEW_LINE;

        Wa( "guint32 dwMsgId = 0;" );
        CCOUT << "ret = CSerialBase::Deserialize(";
        NEW_LINE;
        CCOUT << "    pBuf_, dwMsgId );";
        NEW_LINE;
        Wa( "if( ERROR( ret ) ) return ret;" );
        Wa( "if( m_dwMsgId != dwMsgId )" );
        Wa( "    return -EINVAL;" );
        Wa( "destVal[ JSON_ATTR_STRUCTID ] = dwMsgId;" );
        Wa( "Json::Value val_;" );
        NEW_LINE;

        CEmitSerialCodeFuse odesc( m_pWriter, pFields );
        odesc.OutputDeserial( "", "pBuf_" );
        NEW_LINE;
        CCOUT << "destVal[ JSON_ATTR_FIELDS ] = val_;";
        NEW_LINE;
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
        Wa( "gint32 InitUserFuncs() override;" );
        NEW_LINE;
        Wa( "virtual gint32 OnReqComplete( IEventSink*," );
        Wa( "    IConfigDb* pReqCtx, Json::Value& val_," );
        Wa( "    const std::string&, const std::string&," );
        Wa( "    gint32, bool ) = 0;" );
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

        /*EMIT_DISCLAIMER;
        CCOUT << "// " << g_strCmdLine;
        NEW_LINE;
        Wa( "// Your task is to implement the following classes" );
        Wa( "// to get your rpc server work" );
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
        */

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
            CCOUT << "{ SetClassId( clsid(" << strClass << " ) ); }";
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

            NEW_LINE;
            Wa( "gint32 CustomizeRequest(" );
            Wa( "    IConfigDb* pReqCfg," );
            Wa( "    IEventSink* pCallback ) override" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "ret = super::CustomizeRequest(");
            Wa( "    pReqCfg, pCallback );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "guint64 qwTaskId = 0, qwReqId = 0;" );
            Wa( "CCfgOpener oReq( ( IConfigDb*)pReqCfg );" );
            Wa( "ret = oReq.GetQwordProp(" );
            Wa( "    propTaskId, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CCfgOpenerObj oCfg( ( IConfigDb*)pCallback );" );
            Wa( "ret = oCfg.GetQwordProp(" );
            Wa( "    propTaskId, qwReqId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "ret = AddReq( qwReqId, qwTaskId );" );
            CCOUT << "oCfg.RemoveProperty( propTaskId );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            NEW_LINE;
            Wa( "return ret;" );
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

            Wa( "//Request Dispatcher" );
            Wa( "gint32 DispatchReq(" );
            Wa( "    IConfigDb* pContext," );
            Wa( "    const Json::Value& valReq," );
            Wa( "    Json::Value& valResp ) override;" );
            NEW_LINE;
            Wa( "gint32 CancelRequestByReqId(" );
            Wa( "    guint64 qwReqId, guint64 qwThisReq );" );

            NEW_LINE;
            Wa( "gint32 OnReqComplete(" );
            Wa( "    IEventSink* pCallback," );
            Wa( "    IConfigDb* pReqCtx," );
            Wa( "    Json::Value& valResp," );
            Wa( "    const std::string& strIfName," );
            Wa( "    const std::string& strMethod," );
            Wa( "    gint32 iRet, bool bNoReply ) override;" );
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
            NEW_LINES(2);
            Wa( "gint32 DoRmtModEvent(" );
            Wa( "    EnumEventId iEvent," );
            Wa( "    const std::string& strModule," );
            CCOUT << "    IConfigDb* pEvtCtx ) override";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "gint32 ret = super::DoRmtModEvent(" );
            Wa( "     iEvent, strModule, pEvtCtx );");
            Wa( "if( SUCCEEDED( ret ) )" );
            Wa( "    ret = DoRmtModEventFuse(" );
            Wa( "        iEvent, strModule, pEvtCtx );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;

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
        CCOUT << "{ SetClassId( clsid(" << strClass << " ) ); }";
        NEW_LINES( 2 );

        Wa( "// Response&Event Dispatcher" );
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
        Wa( "{ return OnWriteResumedFuse( hChannel ); } " );
        NEW_LINE;

        Wa( "gint32 OnPostStart(" );
        Wa( "    IEventSink* pCallback ) override" );
        BLOCK_OPEN;
        Wa( "StartQpsTask();" );
        CCOUT << "return super::OnPostStart( pCallback );";
        BLOCK_CLOSE;
        NEW_LINES( 2 );

        Wa( "gint32 OnPreStop(" );
        Wa( "    IEventSink* pCallback ) override" );
        BLOCK_OPEN;
        Wa( "StopQpsTask();" );
        CCOUT << "return super::OnPreStop( pCallback );";
        BLOCK_CLOSE;
        NEW_LINES( 2 );

        Wa( "gint32 InvokeUserMethod(" );
        Wa( "    IConfigDb* pParams," );
        Wa( "    IEventSink* pCallback ) override" );
        BLOCK_OPEN;
        Wa( "gint32 ret = AllocReqToken();" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    return ret;" );
        Wa( "return super::InvokeUserMethod(" );
        CCOUT << "    pParams, pCallback );";
        BLOCK_CLOSE;
        NEW_LINES( 2 );

        Wa( "// methods for active canceling" );
        Wa( "gint32 UserCancelRequest(" );
        Wa( "    IEventSink* pCallback," );
        Wa( "    guint64 qwTaskId ) override;" );
        NEW_LINE;

        Wa( "private:" );
        Wa( "gint32 UCRCancelHandler(" );
        Wa( "    IEventSink* pCallback," );
        Wa( "    gint32 iRet," );
        Wa( "    guint64 qwTaskId );" );
        NEW_LINE;

        Wa( "gint32 UserCancelRequestCbWrapper(" );
        Wa( "    const Json::Value& oJsResp );" );
        NEW_LINE;

        Wa( "gint32 UserCancelRequestComplete(" );
        Wa( "    IEventSink* pCallback," );
        CCOUT << "    guint64 qwTaskToCancel );";

        BLOCK_CLOSE;
        Wa( ";" );

    }while( 0 );

    return ret;
}

gint32 CMethodWriterFuse::GenSerialArgs(
    ObjPtr& pArgList,
    const std::string& strBuf,
    bool bDeclare, bool bAssign,
    bool bNoRet,
    bool bLocked )
{
    gint32 ret = 0;
    if( GetArgCount( pArgList ) == 0 )
        return 0;

    do{
        NEW_LINE;
        Wa( "CJsonSerialBase oSerial_(" );
        ObjPtr pObj;
        ret = FindParentByClsid( pArgList,
            clsid( CInterfaceDecl ), pObj );
        if( ERROR( ret ) )
            break;
        CInterfaceDecl* pifd = pObj;
        if( pifd->IsStream() )
        {
            CCOUT << "    ObjPtr( GetStreamIf() ), "<<
                ( bLocked ? "true );" : "false );" );
        }
        else
        {
            CCOUT << "    ObjPtr( this ), "<<
                ( bLocked ? "true );" : "false );" );
        }
        NEW_LINE;

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
    bool bNoRet,
    bool bLocked )
{
    gint32 ret = 0;
    if( GetArgCount( pArgList ) == 0 )
        return 0;

    do{
        NEW_LINE;
        Wa( "CJsonSerialBase oDeserial_(" );

        ObjPtr pObj;
        ret = FindParentByClsid( pArgList,
            clsid( CInterfaceDecl ), pObj );
        if( ERROR( ret ) )
            break;
        CInterfaceDecl* pifd = pObj;
        if( pifd->IsStream() )
        {
            CCOUT << "    ObjPtr( GetStreamIf() ), "<<
                ( bLocked ? "true );" : "false );" );
        }
        else
        {
            CCOUT << "    ObjPtr( this ), "<<
                ( bLocked ? "true );" : "false );" );
        }
        NEW_LINE;

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
        Wa( "Json::Value val_( Json::objectValue );" );
        if( dwOutCount > 0 )
        {
            /* need deserialization */
            Wa( "while( SUCCEEDED( iRet ) )" );
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
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "if( ERROR( ret ) && SUCCEEDED( iRet ) ) iRet = ret;" );
        }
        CMethodDecls* pmds =
            ObjPtr( m_pNode->GetParent() );
        CInterfaceDecl* pifd =
            ObjPtr( pmds->GetParent() );

        Wa( "OnReqComplete( pCallback, pReqCtx, val_," ); 
        CCOUT << "    \"" << pifd->GetName() << "\",";
        NEW_LINE;
        CCOUT << "    \"" << m_pNode->GetName() << "\",";
        NEW_LINE;
        stdstr strNoReply = "false";
        if( bNoReply )
            strNoReply = "true";
        CCOUT << "    iRet, " << strNoReply << " );";
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
        CCOUT << "do";
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
        Wa( "CFuseSvcProxy* pProxy = ObjPtr( this );" );
        Wa( "pProxy->ReceiveEvtJson( strEvent );" );
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
        Wa( "Json::Value val_;" );
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
        NEW_LINE;
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

        Wa( "CCfgOpener oResp(" );
        Wa( "    ( IConfigDb* )pRespCb_->GetConfig() );" );
        Wa( "oResp.SetQwordProp(" );
        Wa( "     propTaskId, qwReqId );" );
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
            Wa( "if( !oJsReq.isMember( JSON_ATTR_PARAMS) )" );
            Wa( "{ ret = -ENOENT; break; }" );
            Wa( "val_ = oJsReq[ JSON_ATTR_PARAMS ];" );
            Wa( "if( val_.empty() || !val_.isObject() )" );
            Wa( "{ ret = -EINVAL; break; }" );

            ret = GenSerialArgs( pInArgs,
                "pBuf_", true, true, true, true );
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
            Wa( "gint32 ret2 = pRespCb_->GetError();" );
            Wa( "if( SUCCEEDED( ret ) )" );
            BLOCK_OPEN;
            Wa( "if( ret2 != STATUS_PENDING )" );
            BLOCK_OPEN;
            Wa( "// pRespCb_ has been called" );
            CCOUT << "ret = STATUS_PENDING;";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "else" );
            BLOCK_OPEN;
            Wa( "// immediate return" );
            Wa( "( *pRespCb_ )( eventCancelTask );" );
            CCOUT << "pRespCb_.Clear();";
            BLOCK_CLOSE;
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING )" );
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = oResp_[ propTaskId ];" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwReqId;" );
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
            Wa( "// immediate return" );
            Wa( "oResp_.GetIntProp(" );
            Wa( "    propReturnValue, ( guint32& )ret );" );
            if( dwOutCount > 0 )
            {
                Wa( "val_ = Json::Value( Json::objectValue );" );

                CCOUT << "do";
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
                    "pBuf2", true, true, true, true );
                if( ERROR( ret ) )
                    break;
                BLOCK_CLOSE;
                Wa( "while( 0 );" );

                Wa( "if( !ERROR( ret ) && !val_.empty() )" );
                CCOUT << "    oJsResp[ JSON_ATTR_PARAMS ] = val_;";
            }
        }
        else
        {
            Wa( "if( ret == STATUS_PENDING )" );
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = oResp_[ propTaskId ];" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwReqId;" );
            CCOUT << "CCfgOpener oContext( context );";
            NEW_LINE;
            CCOUT <<"oContext.SetQwordProp(";
            NEW_LINE;
            CCOUT << "    propTaskId, qwTaskId );";
            BLOCK_CLOSE;
        }
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        NEW_LINE;
        Wa( "if( ERROR( ret ) && !pRespCb_.IsEmpty() )" );
        Wa( "    ( *pRespCb_ )( eventCancelTask );" );
        NEW_LINE;
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
        Wa( "    !oJsEvt.isObject() )" );
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
            CCOUT << "do";
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
            ret = GenSerialArgs( pInArgs,
                "pBuf_", true, true, true, true );
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
        NEW_LINE;
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
        if( !bNoReply )
            Wa( "TaskletPtr pNewCb;" );
        CCOUT << "do";
        BLOCK_OPEN;

        Wa( "CParamList oReqCtx_;" );
        Wa( "oReqCtx_.SetPointer(" );
        Wa( "    propEventSink, pCallback );" );
        Wa( "IConfigDb* pReqCtx_ = oReqCtx_.GetCfg();" );

        if( !bNoReply )
        {
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
            }
            else
            {
                Wa( "ret = SetInvTimeout( pCallback, 0 );" );
            }
            Wa( "if( ERROR( ret ) ) break;" );
            Wa( "DisableKeepAlive( pCallback );" );
            NEW_LINE;

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
        }

        Wa( "Json::Value val_( Json::objectValue );" );
        if( dwInCount > 0 )
        {
            ret = GenDeserialArgs(
                pInArgs, "pBuf_",
                false, false, true );
            if( ERROR( ret ) )
                break;
        }

        Wa( "stdstr strReq;" );
        Wa( "ret = BuildJsonReq( pReqCtx_, val_," );
        CCOUT << "    \"" << strMethod << "\",";
        NEW_LINE;
        CCOUT << "    \"" << m_pIf->GetName() << "\",";
        NEW_LINE;
        Wa( "    0, strReq, false, false );" );
        Wa( "if( ERROR( ret ) ) break;" );
        CCOUT << "//NOTE: pass strReq to FUSE";
        NEW_LINE;
        Wa( "CFuseSvcServer* pSvr = ObjPtr( this );" );
        Wa( "ret = pSvr->ReceiveMsgJson( strReq," );
        CCOUT << "    pCallback->GetObjId() );";
        NEW_LINE;
        if( !bNoReply )
        {
            Wa( "if( SUCCEEDED( ret ) )" );
            CCOUT << "    ret = STATUS_PENDING;";
        }
        else
        {
            Wa( "if( ret == STATUS_PENDING )" );
            CCOUT << "    ret = STATUS_SUCCESS;";
        }
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        if( !bNoReply )
        {
            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING )" );
            Wa( "    return ret;" );
            NEW_LINE;
            Wa( "CParamList oResp_;" );
            Wa( "oResp_[ propSeriProto ] = seriRidl;" );
            Wa( "oResp_[ propReturnValue ] = ret;" );
            Wa( "this->SetResponse( pCallback," );
            Wa(  "    oResp_.GetCfg() );" );
            NEW_LINE;
            Wa( "if( !pNewCb.IsEmpty() )" );
            BLOCK_OPEN;
            Wa( "CIfRetryTask* pTask = pNewCb;" );
            Wa( "pTask->ClearClientNotify();" );
            Wa( "if( pCallback != nullptr )" );
            Wa( "    rpcf::RemoveInterceptCallback(" );
            Wa( "        pTask, pCallback );" );
            CCOUT << "( *pNewCb )( eventCancelTask );";
            BLOCK_CLOSE;
            NEW_LINE;
        }
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
        Wa( "oResp_[ propSeriProto ] = seriRidl;" );

        if( dwOutCount > 0 )
        {
            Wa( "while( SUCCEEDED( ret ) )" );
            BLOCK_OPEN;
            Wa( "BufPtr pBuf_( true );" );
            Wa( "Json::Value val_ =" );
            Wa( "    oJsResp[ JSON_ATTR_PARAMS ];" );
            ret = GenSerialArgs( pOutArgs,
                "pBuf_", true, true, true, true );
            if( ERROR( ret ) )
                break;
            Wa( "oResp_.Push( pBuf_ );" );
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
        }
        Wa( "oResp_[ propReturnValue ] = ret;" );
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
            Wa( "/* // uncomment the block if you" );
            Wa( "   // want to examine the request " );
            Wa( "   // parameters " );
            Wa( "Json::Value val_;" );
            ret = GenDeserialArgs( pInArgs,
                "pBuf_", false, false, true, false );
            if( ERROR( ret ) )
                break;
            Wa( "*/" );
        }

        Wa( "// clean up the FUSE resources" );
        Wa( "guint64 qwTaskId = pCallback->GetObjId();" );
        Wa( "CFuseSvcServer* pSvr = ObjPtr( this );" );
        CCOUT << "pSvr->OnUserCancelRequest( qwTaskId );";
        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplServiceImplFuse::OutputUCRSvr()
{
    gint32 ret = 0;
    do{
        stdstr strSvcName = m_pNode->GetName();
        stdstr strClassName = "C";
        strClassName += strSvcName + "_SvrImpl";

        CCOUT << "gint32 " << strClassName
            << "::UCRCancelHandler(";
        NEW_LINE;
        Wa( "    IEventSink* pCallback," );
        Wa( "    gint32 iRet," );
        Wa( "    guint64 qwTaskId )" );
        BLOCK_OPEN;
        Wa( "return super::UserCancelRequest(" );
        CCOUT << "    pCallback, qwTaskId );";
        BLOCK_CLOSE;
        NEW_LINES(2);

        /*Wa( "extern gint32 BuildJsonReq(" );
        Wa( "    IConfigDb* pReqCtx_," );
        Wa( "    const Json::Value& oJsParams," );
        Wa( "    const std::string& strMethod," );
        Wa( "    const std::string& strIfName, " );
        Wa( "    gint32 iRet," );
        Wa( "    std::string& strReq," );
        Wa( "    bool bProxy," );
        Wa( "    bool bResp );" );
        NEW_LINE;*/

        CCOUT << "gint32 " << strClassName
            << "::UserCancelRequest(";
        NEW_LINE;
        Wa( "    IEventSink* pCallback," );
        Wa( "    guint64 qwInvTaskId )" );
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "TaskletPtr pNewCb;" );
        CCOUT << "do";
        BLOCK_OPEN;

        Wa( "DisableKeepAlive( pCallback );" );
        Wa( "ret = SetInvTimeout( pCallback, 30 );" );
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        Wa( "TaskGrpPtr pTaskGrp = GetTaskGroup();" );
        Wa( "if( pTaskGrp.IsEmpty() )" );
        BLOCK_OPEN;
        Wa( "ret = -EFAULT;" );
        CCOUT << "break;";
        BLOCK_CLOSE;
        NEW_LINE;

        Wa( "TaskletPtr pTask;" );
        Wa( "ret = pTaskGrp->FindTaskByRmtId(" );
        Wa( "    qwInvTaskId, pTask );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        NEW_LINE;
        
        Wa( "CParamList oReqCtx_;" );
        Wa( "oReqCtx_.SetPointer(" );
        Wa( "    propEventSink, pCallback );" );
        NEW_LINE;
        
        Wa( "ret = DEFER_CANCEL_HANDLER2(" );
        Wa( "    -1, pNewCb, this," );
        CCOUT << "    &"<< strClassName
            <<"::UCRCancelHandler,";
        NEW_LINE;
        Wa( "    pCallback, 0, qwInvTaskId );" );
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        Wa( "Json::Value val_( Json::objectValue );" );
        Wa( "val_[ JSON_ATTR_REQCTXID ] =" );
        Wa( "    ( Json::UInt64 )pTask->GetObjId();" );
        NEW_LINE;

        Wa( "stdstr strReq;" );
        Wa( "IConfigDb* pReqCtx_ = oReqCtx_.GetCfg();" );
        Wa( "ret = BuildJsonReq( pReqCtx_, val_," );
        Wa( "    \"UserCancelRequest\"," );
        Wa( "    \"IInterfaceServer\"," );
        Wa( "    0, strReq, false, false );" );
        Wa( "if( ERROR( ret ) ) break;" );
        NEW_LINE;

        Wa( "// NOTE: pass strReq to FUSE" );
        Wa( "ret = this->ReceiveMsgJson( strReq," );
        Wa( "    pCallback->GetObjId() );" );
        Wa( "if( SUCCEEDED( ret ) )" );
        CCOUT << "    ret = STATUS_PENDING;";
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        
        Wa( "if( ret == STATUS_PENDING )" );
        Wa( "    return ret;" );
        NEW_LINE;

        Wa( "if( !pNewCb.IsEmpty() )" );
        BLOCK_OPEN;
        Wa( "CIfRetryTask* pTask = pNewCb;" );
        Wa( "pTask->ClearClientNotify();" );
        Wa( "if( pCallback != nullptr )" );
        Wa( "    rpcf::RemoveInterceptCallback(" );
        Wa( "        pTask, pCallback );" );
        Wa( "( *pNewCb )( eventCancelTask );" );
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "// fallback to the default request cancel handler" );
        Wa( "return super::UserCancelRequest(" );
        CCOUT << "    pCallback, qwInvTaskId );";
        BLOCK_CLOSE;
        NEW_LINES(2);

        CCOUT << "gint32 " << strClassName
            << "::UserCancelRequestComplete(";
        NEW_LINE;
        Wa( "    IEventSink* pCallback," );
        Wa( "    guint64 qwTaskToCancel )" );
        BLOCK_OPEN;
        Wa( "if( pCallback == nullptr )" );
        Wa( "    return -EINVAL;" );
        Wa( "gint32 ret = super::UserCancelRequest(" );
        Wa( "    pCallback, qwTaskToCancel );" );
        Wa( "pCallback->OnEvent(" );
        Wa( "    eventTaskComp, ret, 0, 0 );" );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINES(2);

        CCOUT << "gint32 " << strClassName
            <<"::UserCancelRequestCbWrapper(";
        NEW_LINE;
        Wa( "    const Json::Value& oJsResp )" );
        BLOCK_OPEN; 
        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;

        Wa( "guint64 qwTaskId = ( HANDLE )" );
        Wa( "    oJsResp[ JSON_ATTR_REQCTXID ].asUInt64();" );
        NEW_LINE;

        Wa( "TaskGrpPtr pTaskGrp = GetTaskGroup();" );
        Wa( "if( pTaskGrp.IsEmpty() )" );
        Wa( "    break;" );
        NEW_LINE;

        Wa( "TaskletPtr pTask;" );
        Wa( "ret = pTaskGrp->FindTask(" );
        Wa( "    qwTaskId, pTask );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        NEW_LINE;

        Wa( "IConfigDb* pReq = nullptr;" );
        Wa( "CCfgOpenerObj oInvTask(" );
        Wa( "    ( IEventSink* )pTask );" );
        Wa( "ret = oInvTask.GetPointer(" );
        Wa( "    propReqPtr, pReq );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        NEW_LINE;

        Wa( "guint64 qwTaskToCancel = 0;" );
        Wa( "CCfgOpener oReq( pReq );" );
        Wa( "ret = oReq.GetQwordProp(" );
        Wa( "    0, qwTaskToCancel );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );

        NEW_LINE;
        Wa( "ret = UserCancelRequestComplete(" );
        CCOUT << "    pTask, qwTaskToCancel );";
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
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
        CCOUT << "// " << g_strCmdLine;
        NEW_LINE;
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
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "if( this->GetState() != stateConnected )" );
            Wa( "{ ret = -ENOTCONN; break; }" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||");
            Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
            Wa( "if( strType != \"resp\" &&" );
            Wa( "    strType != \"evt\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_IFNAME1 ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_IFNAME1 ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strIfName =" );
            Wa( "    oMsg[ JSON_ATTR_IFNAME1 ].asString();" );
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
                NEW_LINE;
                CCOUT << "ret = " << "I" << strIfName
                    << "_SImpl::DispatchIfMsg( oMsg );";
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            Wa( "if( strIfName == \"IInterfaceServer\" )" );
            BLOCK_OPEN;
            Wa( "if( !oMsg.isMember( JSON_ATTR_METHOD ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_METHOD ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strMethod =" );
            Wa( "    oMsg[ JSON_ATTR_METHOD ].asString();" );
            Wa( "if( strMethod == \"OnKeepAlive\" )" );
            BLOCK_OPEN;
            Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
            Wa( "if( strType != \"evt\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_REQCTXID ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 dwTaskId =" );
            Wa( "    oMsg[ JSON_ATTR_REQCTXID ].asUInt64();" );
            CCOUT << "ret = this->OnKeepAlive( dwTaskId );";
            NEW_LINE;
            CCOUT << "break;";
            BLOCK_CLOSE; // OnKeepAlive

            Wa( "if( strMethod == \"UserCancelRequest\" )" );
            BLOCK_OPEN;
            Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
            Wa( "if( strType != \"resp\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            CCOUT << "ret = this->UserCancelRequestCbWrapper( oMsg );";
            NEW_LINE;
            CCOUT << "break;";
            BLOCK_CLOSE; // UserCancelRequest

            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "ret = -ENOTSUP;";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;

            OutputUCRSvr();
        }
        else
        {
            strClass = "C";
            strClass += strSvcName + "_CliImpl";

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
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "if( !oReq.isMember( JSON_ATTR_METHOD ) ||");
            Wa( "    !oReq[ JSON_ATTR_METHOD ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strMethod =" );
            Wa( "    oReq[ JSON_ATTR_METHOD ].asString();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_IFNAME1 ) ||");
            Wa( "    !oReq[ JSON_ATTR_IFNAME1 ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strIfName =" );
            Wa( "    oReq[ JSON_ATTR_IFNAME1 ].asString();" );
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
            Wa( "oResp[ JSON_ATTR_IFNAME1 ] = strIfName;" );
            Wa( "oResp[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwReqId;" );
            Wa( "if( this->GetState() != stateConnected )" );
            Wa( "{ ret = -ENOTCONN; break; }" );
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
                NEW_LINE;
                BLOCK_OPEN;
                CCOUT << "ret = " << "I" << strIfName
                    << "_PImpl::DispatchIfReq( ";
                NEW_LINE;
                CCOUT << "    pContext, oReq, oResp );";
                NEW_LINE;
                Wa( "if( ret != STATUS_PENDING )" );
                Wa( "    this->RemoveReq( qwReqId );" );
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            Wa( "if( strIfName == \"IInterfaceServer\" )" );
            BLOCK_OPEN;
            Wa( "if( strMethod != \"UserCancelRequest\" )" );
            Wa( "{ ret = -ENOSYS; break; }" );
            Wa( "if( !oReq.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !oReq[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 qwThisReq =" );
            Wa( "    oReq[ JSON_ATTR_REQCTXID ].asUInt64();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_PARAMS ) ||");
            Wa( "    !oReq[ JSON_ATTR_PARAMS ].isObject() ||" );
            Wa( "    oReq[ JSON_ATTR_PARAMS ].empty() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "Json::Value val_ = oReq[ JSON_ATTR_PARAMS ];" );
            Wa( "if( !val_.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !val_[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 qwReqId =" );
            Wa( "    val_[ JSON_ATTR_REQCTXID ].asUInt64();" );
            Wa( "ret = CancelRequestByReqId( qwReqId, qwThisReq );" );
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "ret = -ENOSYS;";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            Wa( "if( ret == -STATUS_PENDING )" );
            Wa( "    oResp[ JSON_ATTR_RETCODE ] = STATUS_PENDING;" );
            Wa( "else" );
            Wa( "    oResp[ JSON_ATTR_RETCODE ] = ret;" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;

            // implementation of the CancelRequestByReqId
            CCOUT << "gint32 " << strClass
                << "::CancelRequestByReqId( ";
            NEW_LINE;
            CCOUT << "    guint64 qwReqId, guint64 qwThisReq )";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = 0;" );
            Wa( "ret = this->GetTaskId( qwReqId, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "gint32 (*func)( CRpcServices* pIf," );
            Wa( "    IEventSink*, guint64, guint64) =" );
            Wa( "    ([]( CRpcServices* pIf," );
            Wa( "        IEventSink* pThis," );
            Wa( "        guint64 qwReqId, " );
            Wa( "        guint64 qwThisReq )->gint32" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
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
            Wa( "Json::Value oJsResp( Json::objectValue );" );
            Wa( "oJsResp[ JSON_ATTR_IFNAME1 ] =" );
            Wa( "    \"IInterfaceServer\";" );
            Wa( "oJsResp[ JSON_ATTR_METHOD ] =" );
            Wa( "    \"UserCancelRequest\";" );
            Wa( "oJsResp[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
            Wa( "oJsResp[ JSON_ATTR_RETCODE ] = iRet;" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwThisReq;" );
            Wa( "Json::Value oParams( Json::objectValue );" );
            Wa( "oParams[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwReqId;" );
            Wa( "oJsResp[ JSON_ATTR_PARAMS ] = oParams;" );
            Wa( "Json::StreamWriterBuilder oBuilder;" );
            Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
            Wa( "oBuilder[\"indentation\"] = \"\";" );
            CCOUT <<  "stdstr strResp = Json::writeString( "
                << "oBuilder, oJsResp );";
            NEW_LINE;
            CCOUT << strClass << "* pClient = ObjPtr( pIf );";
            NEW_LINE;
            Wa( "pClient->ReceiveMsgJson( strResp, qwThisReq );" );
            CCOUT << "pClient->RemoveReq( qwThisReq );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            Wa( ");" );
            Wa( "TaskletPtr pCb;" );
            Wa( "ret = NEW_FUNCCALL_TASK( pCb," );
            Wa( "    this->GetIoMgr(), func, this," );
            Wa( "    nullptr, qwReqId, qwThisReq );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINE;
            Wa( "CDeferredFuncCallBase< CIfRetryTask >*" );
            Wa( "    pCall = pCb;" );
            Wa( "TaskletPtr pWrapper;" );
            Wa( "CCfgOpener oCfg;" );
            Wa( "oCfg.SetPointer( propIfPtr, this );" );
            Wa( "ret = pWrapper.NewObj(" );
            Wa( "   clsid( CTaskWrapper )," );
            Wa( "   ( IConfigDb* )oCfg.GetCfg() );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINE;
            Wa( "ObjPtr pObj( pWrapper );" );
            Wa( "Variant oArg0( pObj );" );
            Wa( "pCall->UpdateParamAt( 1, oArg0 );" );
            NEW_LINE;
            Wa( "CTaskWrapper* ptw = pWrapper;" );
            Wa( "ptw->SetCompleteTask( pCall );" );
            Wa( "CCfgOpenerObj otwCfg( ptw );" );
            Wa( "otwCfg.SetQwordProp( propTaskId, qwThisReq );" );
            NEW_LINE;
            Wa( "ret = this->CancelReqAsync(" );
            Wa( "    ptw, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            BLOCK_OPEN;
            Wa( "( *ptw )( eventCancelTask );" );
            CCOUT << "this->RemoveReq( qwReqId );";
            BLOCK_CLOSE;
            Wa( "break;" );
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;

            /*NEW_LINE;
            Wa( "extern gint32 BuildJsonReq(" );
            Wa( "    IConfigDb* pReqCtx_," );
            Wa( "    const Json::Value& oJsParams," );
            Wa( "    const std::string& strMethod," );
            Wa( "    const std::string& strIfName, " );
            Wa( "    gint32 iRet," );
            Wa( "    std::string& strReq," );
            Wa( "    bool bProxy," );
            Wa( "    bool bResp );" );
            NEW_LINE; */

            // implement OnReqComplete
            CCOUT << "gint32 " << strClass;
            Wa( "::OnReqComplete(" );
            Wa( "    IEventSink* pCallback," );
            Wa( "    IConfigDb* pReqCtx," );
            Wa( "    Json::Value& valResp," );
            Wa( "    const std::string& strIfName," );
            Wa( "    const std::string& strMethod," );
            Wa( "    gint32 iRet, bool bNoReply )" );
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
            Wa( "CCfgOpener oReqCtx_( pReqCtx );" );
            Wa( "guint64 qwReqId = 0;" );
            Wa( "ret = oReqCtx_.GetQwordProp( 1, qwReqId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "if( !bNoReply )" );
            Wa( "    ret = this->ReceiveMsgJson( strReq, qwReqId );" );
            CCOUT << "this->RemoveReq( qwReqId );";
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

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 dwCount = pmds->GetCount();
        if( dwCount == 0 )
        {
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            NEW_LINE;
            break;
        }

        bool bHasReq = false;
        guint32 i;
        for( i = 0; i < dwCount; i++ )
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

            bHasReq = true;
            break;
        }
        if( ERROR( ret ) )
            break;

        if( !bHasReq )
        {
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            NEW_LINE;
            break;
        }

        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;

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

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 dwCount = pmds->GetCount();
        if( dwCount == 0 )
        {
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            NEW_LINE;
            break;
        }

        bool bHasReq = false;
        bool bHasEvent = false;

        guint32 i;
        for( i = 0; i < dwCount; i++ )
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
                bHasEvent = true;
                continue;
            }
            bHasReq = true;
        }
        if( ERROR( ret ) )
            break;

        Wa( "gint32 ret = 0;" );
        CCOUT <<"do";
        BLOCK_OPEN;
        Wa( "if( !oMsg.isMember( JSON_ATTR_METHOD ) ||");
        Wa( "    !oMsg[ JSON_ATTR_METHOD ].isString() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "stdstr strMethod =" );
        Wa( "    oMsg[ JSON_ATTR_METHOD ].asString();" );
        Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||");
        Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "stdstr strType =" );
        Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
        if( bHasReq )
        {
            Wa( "if( strType == \"resp\" )" );
            BLOCK_OPEN;
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

            for( i = 0; i < dwCount; i++ )
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
                if( !pmd->IsNoReply() )
                {
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
                else
                {
                    Wa( "// no reply" );
                    Wa( "ret = -EINVAL;" );
                }
                CCOUT << "break;";
                BLOCK_CLOSE;
                if( i < dwCount - 1 )
                    NEW_LINE;
            }
            BLOCK_CLOSE; // strType == resp
            NEW_LINE;
        }
        if( bHasEvent )
        {
            Wa( "if( strType == \"evt\" )" );
            BLOCK_OPEN;
            for( i = 0; i < dwCount; i++ )
            {
                ObjPtr pObj = pmds->GetChild( i );
                CMethodDecl* pmd = pObj;
                if( pmd == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                if( !pmd->IsEvent() )
                    continue;

                ObjPtr pInArgs = pmd->GetInArgs();
                guint32 dwInCount = GetArgCount( pInArgs );
                ObjPtr pOutArgs = pmd->GetOutArgs();
                guint32 dwOutCount = GetArgCount( pOutArgs );
                CCOUT << "if( strMethod == \""
                    << pmd->GetName() << "\" )";
                NEW_LINE;
                BLOCK_OPEN;
                CCOUT << "ret = " << strClass << "::"
                    << pmd->GetName() <<"(";
                if( dwInCount > 0 ) 
                    CCOUT << " oMsg );";
                else
                    CCOUT << ");";
                NEW_LINE;
                Wa( "if( ret == STATUS_PENDING )" );
                Wa( "    ret = STATUS_SUCCESS;" );
                CCOUT << "break;";
                BLOCK_CLOSE;
                if( i < dwCount - 1 )
                    NEW_LINE;
            }
            BLOCK_CLOSE; // strType == evt
            NEW_LINE;
        }
        CCOUT << "ret = -ENOENT;";
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
            bool bProxy = m_bProxy;
            if( bProxy )
            {
                m_pWriter->SelectMainCli();
            }
            else
            {
                m_pWriter->SelectMainSvr();
            }

            EMIT_DISCLAIMER;
            CCOUT << "// " << g_strCmdLine;
            NEW_LINE;
            Wa( "#include \"rpc.h\"" );
            Wa( "#include \"proxy.h\"" );
            Wa( "#include \"fuseif.h\"" );
            Wa( "using namespace rpcf;" );
            for( auto elem : vecSvcs )
            {
                CServiceDecl* pSvc = elem;
                bool bNewLine = false;
                if( bProxy || g_bMklib )
                {
                    CCOUT << "#include \""
                        << pSvc->GetName() << "cli.h\"";
                    bNewLine = true;
                }
                if( !bProxy )
                {
                    if( bNewLine )
                        NEW_LINE;
                    CCOUT << "#include \""
                        << pSvc->GetName() << "svr.h\"";
                }
                NEW_LINE;
            }
            if( g_bMklib && bProxy )
                break;

            NEW_LINES( 1 );
            if( g_bMklib )
                Wa( "extern ObjPtr g_pIoMgr;" );
            else
                Wa( "ObjPtr g_pIoMgr;" );
            NEW_LINE;

            ObjPtr pRoot( m_pNode );
            CImplClassFactory oicf(
                m_pWriter, pRoot, bProxy );

            ret = oicf.Output();
            if( ERROR( ret ) )
                break;

            if( g_bMklib )
                break;

            NEW_LINE;

            EmitInitContext( bProxy, m_pWriter );
            // main function
            EmitFuseMain( vecSvcs, bProxy, m_pWriter );
        }

    }while( 0 );

    return ret;
}
