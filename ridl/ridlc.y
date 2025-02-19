/*
 * =====================================================================================
 *
 *       Filename:  ridlc.y
 *
 *    Description:  The gramma parser for the RPC IDL
 *
 *        Version:  1.0
 *        Created:  02/10/2021 05:01:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
%{
#include "rpc.h"

using namespace rpcf;

#include "lexer.h"
#include "astnode.h"
#include <memory>
#include <stdlib.h>
#include <set>

#define FUSE_PROXY  1
#define FUSE_SERVER 2
#define FUSE_BOTH   3

CDeclMap g_mapDecls;
ObjPtr g_pRootNode;
CAliasMap g_mapAliases;
std::string g_strAppName;
std::map< std::string, BufPtr > g_mapConsts;

bool g_bSemanErr = false;

std::set< gint32 > int_set =
    { typeUInt64, typeUInt32, typeUInt16, typeByte };

std::map< gint32, std::set< gint32 > >
    g_mapTypeVal = {
        { TOK_STRING, { typeString } },
        { TOK_UINT64, int_set },
        { TOK_INT64, int_set },
        { TOK_UINT32, int_set },
        { TOK_INT32, int_set },
        { TOK_UINT16, int_set },
        { TOK_INT16, int_set },
        { TOK_FLOAT, { typeFloat, typeDouble } },
        { TOK_DOUBLE, { typeFloat, typeDouble } },
        { TOK_BYTE, { typeByte } },
        { TOK_BOOL, { typeByte } },
        { TOK_HSTREAM, { typeNone } }
    };

std::map< gint32, EnumTypeId >
    g_mapTok2Type = {
        { TOK_UINT64, typeUInt64 },
        { TOK_INT64, typeUInt64 },
        { TOK_UINT32, typeUInt32 },
        { TOK_INT32, typeUInt32 },
        { TOK_UINT16, typeUInt16 },
        { TOK_INT16, typeUInt16 },
        { TOK_FLOAT, typeFloat },
        { TOK_DOUBLE, typeDouble },
        { TOK_BYTE, typeByte }
};

std::set< gint32 > g_setValidFlags = {
    TOK_ASYNC,
    TOK_ASYNCP,
    TOK_ASYNCS,
    TOK_EVENT,
    TOK_STREAM,
    TOK_NOREPLY,
};

extern std::vector<
    std::unique_ptr< FILECTX > > g_vecBufs;

extern bool g_bNewSerial;
extern bool g_dwFlags;
extern std::set< stdstr > g_setServices;

void yyerror( YYLTYPE *locp,
    char const* szFile, char const *msg );

template< class RetType >
RetType CastPrimeVal( BufPtr& pBuf )
{
    EnumTypeId  iType = pBuf->GetExDataType();
    switch( iType )
    {
    case typeByte:
            return ( RetType )( guint8& )*pBuf;
    case typeUInt16:
            return ( RetType )( guint16&)*pBuf;
    case typeUInt32:
            return ( RetType )( guint32&)*pBuf;
    case typeUInt64:
            return ( RetType )( guint64&)*pBuf;
    case typeFloat:
            return ( RetType )( float&)*pBuf;
    case typeDouble:
            return ( RetType )( double&)*pBuf;
    default:
        break;
    }
    return 0;
}

BufPtr CastPrimeValBuf(
    EnumTypeId iRetType, BufPtr& pBuf )
{
    BufPtr pRetBuf( true );
    switch( iRetType )
    {
    case typeByte:
        *pRetBuf = CastPrimeVal< guint8 >( pBuf );
        break;
    case typeUInt16:
        *pRetBuf = CastPrimeVal< guint16 >( pBuf );
        break;
    case typeUInt32:
        *pRetBuf = CastPrimeVal< guint32 >( pBuf );
        break;
    case typeUInt64:
        *pRetBuf = CastPrimeVal< guint64 >( pBuf );
        break;
    case typeFloat:
        *pRetBuf = CastPrimeVal< float >( pBuf );
        break;
    case typeDouble:
        *pRetBuf = CastPrimeVal< double >( pBuf );
        break;
    default:
        break;
    }
    return pRetBuf;
}

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#define YYCOPY(Dst, Src, Count) \
do{\
  YYPTRDIFF_T yyi;\
  for (yyi = 0; yyi < (Count); yyi++) \
    (Dst)[yyi] = (Src)[yyi];\
}while (0)

#define CLEAR_RSYMBS \
for( int i = 0; i < ( yylen ); i++ ) \
    { yyvsp[ -i ].Clear(); }

#define DEFAULT_ACTION \
    yyval = yyvsp[ 1 - yylen ]; CLEAR_RSYMBS;

#define ADDTO_ALIAS_MAP( pal, pType_ ) \
do{ \
    gint32 iCount = ( pal )->GetCount(); \
    for( int i = 0; i < iCount; i++ ) \
    { \
        std::string strAlias = \
            ( pal )->GetChild( i ); \
        ObjPtr pTemp;\
        ret = g_mapAliases.GetAliasType( \
            strAlias, pTemp ); \
        if( SUCCEEDED( ret ) ) \
        { \
            std::string strMsg = \
                "typedef '";  \
            strMsg += strAlias + "'"; \
            strMsg += " already declared"; \
            PrintMsg( -EEXIST, \
                strMsg.c_str() ); \
            ret = -EEXIST; \
            break;\
        } \
        ret = 0;\
        g_mapAliases.AddAliasType( \
            strAlias, ( pType_ ) ); \
    } \
}while( 0 );

#define ENABLE_STREAM( pSrc_, pDest_ ) do{ \
    CAstNodeBase* pNode = pSrc_; \
    if( pNode != nullptr && \
        pNode->IsStream() ) \
        pDest_->EnableStream(); \
}while( 0 )

#define ENABLE_SERIAL( pSrc_, pDest_ ) do{ \
    CAstNodeBase* pNode = pSrc_; \
    if( pNode != nullptr && \
        pNode->IsSerialize() ) \
        pDest_->EnableSerialize(); \
}while( 0 )

gint32 CheckNameDup(
    ObjPtr& pInArgs,
    ObjPtr& pOutArgs,
    std::string& strDupName );

%}

%define api.value.type {BufPtr}
%define api.pure full

// values
%token TOK_START
%token TOK_IDENT 
%token TOK_STRVAL
%token TOK_BOOLVAL
%token TOK_BYVAL
%token TOK_INTVAL
%token TOK_FLOATVAL
%token TOK_DBLVAL

// types
%token TOK_STRING
%token TOK_UINT64
%token TOK_INT64
%token TOK_UINT32
%token TOK_INT32
%token TOK_UINT16
%token TOK_INT16
%token TOK_FLOAT
%token TOK_DOUBLE
%token TOK_BYTE
%token TOK_BOOL
%token TOK_BYTEARR
%token TOK_OBJPTR
%token TOK_HSTREAM
%token TOK_ARRAY
%token TOK_MAP
%token TOK_VARIANT

// keywords
%token TOK_INTERFACE
%token TOK_STRUCT
%token TOK_SERVICE
%token TOK_TYPEDEF
%token TOK_APPNAME
%token TOK_CONST
%token TOK_RETURNS

// flags
%token TOK_ASYNC
%token TOK_ASYNCP
%token TOK_ASYNCS
%token TOK_STREAM
%token TOK_EVENT
%token TOK_TIMEOUT
%token TOK_KEEPALIVE
%token TOK_NOREPLY

%parse-param { char const *file_name };
%initial-action
{
    gint32 ret = 0;
    do{
        YYLTYPE2* pLtype = ( YYLTYPE2* )&@$;
        pLtype->initialize( file_name );
        FILECTX* pfc = nullptr;
        if( file_name[ 0 ] != '/' )
        {
            char* path = realpath(
                file_name, nullptr );

            if( path == nullptr )
            {
                stdstr strMsg =
                    "cannot find the file ";
                strMsg = strMsg + "'" + file_name + "'";
                OutputMsg( -ENOENT, strMsg );
                ret = -ENOENT;
                break;
            }
            else
            {
                pfc = new FILECTX( path );
                free( path );
            }
        }
        else
        {
            pfc = new FILECTX( file_name );
        }
        g_vecBufs.push_back(
            std::unique_ptr< FILECTX >( pfc ) );
        yyin = pfc->m_fp;
        char szBuf[ 256 ];
        ret = fread( szBuf, 1, 256, yyin );
        if( ret == 0 )
            ret = ferror( yyin );
        rewind( yyin );
    }while( 0 );
    if( ERROR( ret ) )
        YYERROR;
};


%%

statements : statement ';'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStatements ) );
        CStatements* pstmts = pNode;
        ObjPtr& pstmt = *$1;
        pstmts->AddChild( pstmt );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
        g_pRootNode = pNode;
    }
    ;
statements : statements statement ';'
    {
        ObjPtr pNode = *$1;
        if( !$2.IsEmpty() && !$2->empty() )
        {
            CStatements* pstmts = pNode;
            ObjPtr& pstmt = *$2;
            pstmts->AddChild( pstmt );
        }
        $$ = $1;
        CLEAR_RSYMBS;
        g_pRootNode = pNode;
    }
    ;

statement :
    interf_decl { DEFAULT_ACTION; }
    | service_decl { DEFAULT_ACTION; }
    | struct_decl { DEFAULT_ACTION; }
    | typedef_decl { CLEAR_RSYMBS; }
    | appname { DEFAULT_ACTION; }
    | const_decl { DEFAULT_ACTION; }
    ;

appname : TOK_APPNAME TOK_STRVAL
    {
        std::string strName = *$2;
        if( strName.empty() )
        {
            std::string strMsg =
                "appname is empty";
            PrintMsg(
                -EINVAL, strMsg.c_str() );
            g_bSemanErr = true;
        }
        g_strAppName = strName;

        ObjPtr pNode;
        pNode.NewObj( clsid( CAppName ) );
        CAppName* pan = pNode;
        pan->SetName( strName );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

prime_type :
      TOK_STRING
      | TOK_UINT64
      | TOK_INT64
      | TOK_UINT32
      | TOK_INT32
      | TOK_UINT16
      | TOK_INT16
      | TOK_FLOAT
      | TOK_DOUBLE
      | TOK_BYTE
      | TOK_BOOL
      | TOK_BYTEARR
      | TOK_HSTREAM
      | TOK_VARIANT
    ;
prime_type : TOK_OBJPTR
    {
        $$ = $1;
        if( g_dwFlags &
            ( FUSE_PROXY | FUSE_SERVER ) )
        {
            stdstr strMsg = "ObjPtr is not "
                "supported by rpcfs. Dummy code is generated";
            PrintWarning(
                -ENOTSUP, strMsg.c_str() );
        }
        CLEAR_RSYMBS;
    }

arr_type :
    TOK_ARRAY '<' idl_type '>'
    {
        ObjPtr pObj;
        pObj.NewObj( clsid( CArrayType ) );
        CArrayType* pat = pObj;
        guint32& dwName = *$1;
        pat->SetName( dwName );
        ObjPtr& pType = *$3;
        pat->SetElemType( pType );

        ENABLE_STREAM( pType, pat );
        pat->EnableSerialize();

        BufPtr pBuf( true );
        *pBuf = pObj;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

map_type :
    TOK_MAP '<' idl_type ',' idl_type '>'
    {
        ObjPtr pObj;
        pObj.NewObj( clsid( CMapType ) );
        CMapType* pmt = pObj;
        guint32& dwName = *$1;
        pmt->SetName( dwName );
        ObjPtr& pElem = *$5;
        pmt->SetElemType( pElem );
        ObjPtr& pKey = *$3;

        ENABLE_STREAM( pKey, pmt );
        pmt->EnableSerialize();

        pmt->SetKeyType( pKey );
        BufPtr pBuf( true );
        *pBuf = pObj;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

struct_ref : TOK_IDENT
    {
        ObjPtr pObj;
        pObj.NewObj( clsid( CStructRef ) );
        CStructRef* psr = pObj;
        std::string strName = *$1;

        ObjPtr pTemp;
        gint32 ret = g_mapDecls.GetDeclNode(
            strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            psr->SetName( strName );
            psr->EnableSerialize();
        }
        else
        {
            ObjPtr pType;
            ret = g_mapAliases.GetAliasType(
                strName, pType );
            if( SUCCEEDED( ret ) )
            {
                psr->SetName( strName );
                ENABLE_STREAM( pType, psr );
                if( pType->GetClsid() !=
                    clsid( CPrimeType ) )
                {
                    psr->EnableSerialize();
                }
                else
                {
                    CPrimeType* ppt = pType;
                    guint32 dwName = ppt->GetName();
                    if( dwName == TOK_HSTREAM )
                        psr->EnableSerialize();
                }
            }
            else
            {
                std::string strMsg = "'"; 
                strMsg += strName + "'";
                strMsg += " not declared yet";
                PrintMsg(
                    -ENOENT, strMsg.c_str() );
                CLEAR_RSYMBS;
                g_bSemanErr = true;
            }
        }
        BufPtr pBuf( true );
        *pBuf = pObj;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

struct_decl : TOK_STRUCT TOK_IDENT '{' field_list '}'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CStructDecl ) );
        CStructDecl* psd = pNode;
        std::string strName = *$2;
        ObjPtr pTemp;
        gint32 ret = g_mapDecls.GetDeclNode(
            strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            std::string strMsg = "'"; 
            strMsg += strName + "'";
            strMsg += " already declared";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        psd->SetName( strName );
        ObjPtr& pfl = *$4;
        psd->SetFieldList( pfl );
        psd->EnableSerialize();

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        g_mapDecls.AddDeclNode( strName, pNode );
        CLEAR_RSYMBS;
    }
    ;

field_list : field_decl
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CFieldList ) );
        ObjPtr& pfdd = *$1;
        CFieldList* pfl = pNode;
        pfl->AddChild( pfdd );

        ENABLE_STREAM( pfdd, pfl );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

field_list : field_decl field_list
    {
        ObjPtr& pNode = *$2;
        ObjPtr& pfdd = *$1;
        CFieldList* pfl = pNode;
        pfl->InsertChild( pfdd );

        ENABLE_STREAM( pfdd, pfl );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
    }
    ;

field_decl : idl_type TOK_IDENT ';'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CFieldDecl ) );
        std::string strName = *$2;
        CFieldDecl* pfdl = pNode;
        pfdl->SetName( strName );
        ObjPtr& pType = *$1;
        pfdl->SetType( pType );

        ENABLE_STREAM( pType, pfdl );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

field_decl : idl_type TOK_IDENT '=' const_val ';'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CFieldDecl ) );
        std::string strName = *$2;
        CFieldDecl* pfdl = pNode;
        pfdl->SetName( strName );
        ObjPtr& pType = *$1;
        pfdl->SetType( pType );
        CPrimeType* pPtype = pType;
        if( pPtype == nullptr )
        {
            std::string strMsg =
                "value does not match the type";
            PrintMsg(
                -ENOTSUP, strMsg.c_str() );
            g_bSemanErr = true;
        }
        gint32 iType = pPtype->GetName();
        BufPtr pVal = $4;
        gint32 iValType = ( gint32 )
            pVal->GetExDataType();

        if( g_mapTypeVal.find( iType ) ==
            g_mapTypeVal.end() )
        {
            std::string strMsg =
            "the type does not support initializer";
            PrintMsg(
                -ENOTSUP, strMsg.c_str() );
            g_bSemanErr = true;
        }
        else if( g_mapTypeVal[ iType ].find( iValType )
            == g_mapTypeVal[ iType ].end() )
        {
            std::string strMsg =
                "the type and value mismatch";
            PrintMsg(
                -ENOTSUP, strMsg.c_str() );
            g_bSemanErr = true;
        }

        if( iType == TOK_STRING ||
            iType == TOK_BOOL ||
            iType == TOK_HSTREAM )
        {
            pfdl->SetVal( pVal );
        }
        else
        {
            std::map< gint32, EnumTypeId>::iterator
                itr = g_mapTok2Type.find( iType );
            if( itr != g_mapTok2Type.end() )
            {
                EnumTypeId iExType = itr->second;
                BufPtr pActVal =
                    CastPrimeValBuf( iExType, pVal );
                pfdl->SetVal( pActVal );
            }
            else
            {
                std::string strMsg =
                    "internal error happens";
                PrintMsg(
                    -EFAULT, strMsg.c_str() );
                g_bSemanErr = true;
            }
        }

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

idl_type :
      prime_type {
        ObjPtr pObj;
        pObj.NewObj( clsid( CPrimeType ) );
        CPrimeType* ppt = pObj;
        guint32& dwName = *$1;
        ppt->SetName( dwName );
        if( dwName == TOK_HSTREAM )
        {
            ppt->EnableStream();
            ppt->EnableSerialize();
        }
        BufPtr pBuf( true );
        *pBuf = pObj;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    | arr_type { DEFAULT_ACTION; }
    | map_type { DEFAULT_ACTION; }
    | struct_ref { DEFAULT_ACTION; }
    ;

const_val :
      TOK_STRVAL { DEFAULT_ACTION; }
    | TOK_INTVAL { DEFAULT_ACTION; }
    | TOK_FLOATVAL { DEFAULT_ACTION; }
    | TOK_DBLVAL { DEFAULT_ACTION; }
    | TOK_BOOLVAL { DEFAULT_ACTION; }
    | TOK_BYVAL { DEFAULT_ACTION; }
    | TOK_IDENT
    {
        std::string strName = *$1;
        if( g_mapConsts.find( strName ) ==
            g_mapConsts.end() )
        {
            std::string strMsg = " Constant '"; 
            strMsg += strName + "' is not declared";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        else
        {
            $$ = g_mapConsts[ strName ];
        }
        CLEAR_RSYMBS;
    }
    ;

attr_name : 
      TOK_ASYNC { DEFAULT_ACTION; }
    | TOK_ASYNCP { DEFAULT_ACTION; }
    | TOK_ASYNCS { DEFAULT_ACTION; }
    | TOK_EVENT { DEFAULT_ACTION; }
    | TOK_STREAM { DEFAULT_ACTION; }
    | TOK_NOREPLY { DEFAULT_ACTION; }
    // flag with value
    | TOK_TIMEOUT { DEFAULT_ACTION; }
    | TOK_KEEPALIVE { DEFAULT_ACTION; }
    ;

attr_exp : attr_name
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CAttrExp ) );
        CAttrExp* pae = pNode;
        guint32& dwName = *$1;
        if( g_setValidFlags.find( dwName ) ==
            g_setValidFlags.end() )
        {
            std::string strMsg = "invalid flag found"; 
            PrintMsg( -EINVAL, strMsg.c_str() );
            g_bSemanErr = true;
        }
        pae->SetName( dwName );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

attr_exp : attr_name '=' const_val
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CAttrExp ) );
        CAttrExp* pae = pNode;
        guint32& dwName = *$1;
        if( dwName != TOK_TIMEOUT &&
            dwName != TOK_KEEPALIVE )
        {
            std::string strMsg =
                "invalid flag expression"; 
            PrintMsg(
                -EINVAL, strMsg.c_str() );
            g_bSemanErr = true;
        }

        BufPtr pValBuf = $3;
        if( pValBuf.IsEmpty() || pValBuf->empty() )
        {
            std::string strMsg =
                "flag expression"; 
            PrintMsg(
                -EINVAL, strMsg.c_str() );
            g_bSemanErr = true;
        }
        else
        {
            EnumTypeId iType =
                pValBuf->GetExDataType();
            if( iType != typeUInt32 )
            {
                std::string strMsg =
                    "flag should be a number "
                    "value"; 
                PrintMsg(
                    -EINVAL, strMsg.c_str() );
                g_bSemanErr = true;
            }
        }

        pae->SetName( dwName );
        pae->SetVal( $3 );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

attr_exps :  attr_exp
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CAttrExps ) );
        ObjPtr& pExp = *$1;
        CAttrExps* paes = pNode;
        paes->AddChild( pExp );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;
attr_exps :  attr_exp ',' attr_exps
    {
        ObjPtr& pNode = *$3;
        CAttrExps* paes = pNode;
        ObjPtr& pExp = *$1;
        paes->InsertChild( pExp );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

attr_list : %empty
    ;

attr_list : '[' attr_exps ']'
    {
        ObjPtr pNode = *$2;
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;
    
formal_arg : idl_type TOK_IDENT
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CFormalArg ) );
        std::string strName = *$2;
        CFormalArg* pfa = pNode;
        pfa->SetName( strName );
        ObjPtr& pType = *$1;
        pfa->SetType( pType );

        ENABLE_STREAM( pType, pfa );
        ENABLE_SERIAL( pType, pfa );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

arg_list : formal_arg
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CArgList ) );
        CArgList* pal = pNode;
        ObjPtr& pArg = *$1;
        pal->AddChild( pArg );

        ENABLE_STREAM( pArg, pal );
        ENABLE_SERIAL( pArg, pal );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
arg_list : formal_arg ',' arg_list
    {
        ObjPtr& pNode = *$3;
        ObjPtr& pArg = *$1;
        CArgList* pal = pNode;
        gint32 ret = pal->InsertChild( pArg );
        if( ret == -EEXIST )
        {
            CFormalArg* pfa = pArg;
            std::string strName = pfa->GetName();
            std::string strMsg = "duplicated arg name '"; 
            strMsg += strName + "'";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }

        ENABLE_STREAM( pArg, pal );
        ENABLE_SERIAL( pArg, pal );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

method_decl : attr_list TOK_IDENT '(' arg_list ')' TOK_RETURNS '(' arg_list ')'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CMethodDecl ) );
        CMethodDecl* pmd = pNode;
        std::string strName = *$2;
        pmd->SetName( strName );
        ObjPtr& pInArgs = *$4;
        ObjPtr& pOutArgs =  *$8;

        std::string strDupName;
        gint32 ret = CheckNameDup(
            pInArgs, pOutArgs, strDupName );        
        if( SUCCEEDED( ret ) )
        {
            std::string strMsg = "duplicated arg  '"; 
            strMsg += strDupName + "'";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }

        pmd->SetInArgs( pInArgs );
        pmd->SetOutArgs( pOutArgs );
        ENABLE_STREAM( pInArgs, pmd );
        ENABLE_SERIAL( pInArgs, pmd );

        ENABLE_STREAM( pOutArgs, pmd );
        ENABLE_SERIAL( pOutArgs, pmd );
        if( g_bNewSerial )
            pmd->EnableSerialize();

        BufPtr pAttrBuf = $1;
        if( ! pAttrBuf.IsEmpty() &&
            ! pAttrBuf->empty() )
        {
            ObjPtr& pal = *pAttrBuf;
            pmd->SetAttrList( pal );
            gint32 ret =
                pmd->CheckTimeoutSec();
            if( ERROR( ret )  )
            {
                std::string strMsg = "'timeout' "; 
                strMsg += "does not have valid value";
                PrintMsg( ret, strMsg.c_str() );
                g_bSemanErr = true;
            }
        }
        // semantic checks
        if( pmd->IsEvent() )
        {
            std::string strMsg = "event handler '"; 
            strMsg += strName + "'";
            strMsg += " should not have responses";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        if( pmd->IsNoReply() )
        {
            std::string strMsg = "no-reply request '"; 
            strMsg += strName + "'";
            strMsg += " should not have responses";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        if( pmd->IsEvent() && pmd->IsNoReply() )
        {
            std::string strMsg = "event handler '"; 
            strMsg += strName + "'";
            strMsg += " should not have no-reply attribute";
            PrintMsg( -EINVAL, strMsg.c_str() );
            g_bSemanErr = true;
        }

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
        
    }
    ;

const_decl : TOK_CONST TOK_IDENT '=' const_val
    {
        std::string strName = *$2;
        g_mapConsts[ strName ] = $4;
        ObjPtr pNode;
        pNode.NewObj( clsid( CConstDecl ) );
        CConstDecl* pcstd = pNode;
        pcstd->SetName( strName );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }

method_decl : attr_list TOK_IDENT '(' ')' TOK_RETURNS '(' arg_list ')'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CMethodDecl ) );
        CMethodDecl* pmd = pNode;
        std::string strName = *$2;
        pmd->SetName( strName );
        ObjPtr& pOutArgs =  *$7;
        pmd->SetOutArgs( pOutArgs );

        ENABLE_STREAM( pOutArgs, pmd );
        ENABLE_SERIAL( pOutArgs, pmd );
        if( g_bNewSerial )
            pmd->EnableSerialize();

        BufPtr pAttrBuf = $1;
        if( ! pAttrBuf.IsEmpty() &&
            ! pAttrBuf->empty() )
        {
            ObjPtr& pal = *pAttrBuf;
            pmd->SetAttrList( pal );
            gint32 ret =
                pmd->CheckTimeoutSec();
            if( ERROR( ret )  )
            {
                std::string strMsg = "'timeout' "; 
                strMsg += "does not have valid value";
                PrintMsg( ret, strMsg.c_str() );
                g_bSemanErr = true;
            }
        }

        // semantic checks
        if( pmd->IsEvent() )
        {
            std::string strMsg = "event handler '"; 
            strMsg += strName + "'";
            strMsg += " should not have responses";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        if( pmd->IsNoReply() )
        {
            std::string strMsg = "no-reply request '"; 
            strMsg += strName + "'";
            strMsg += " should not have responses";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        if( pmd->IsEvent() && pmd->IsNoReply() )
        {
            std::string strMsg = "event handler '"; 
            strMsg += strName + "'";
            strMsg += " should not have no-reply attribute";
            PrintMsg( -EINVAL, strMsg.c_str() );
            g_bSemanErr = true;
        }

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

method_decl : attr_list TOK_IDENT '(' ')' TOK_RETURNS '(' ')'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CMethodDecl ) );
        CMethodDecl* pmd = pNode;
        std::string strName = *$2;
        pmd->SetName( strName );
        BufPtr pAttrBuf = $1;
        if( ! pAttrBuf.IsEmpty() &&
            ! pAttrBuf->empty() )
        {
            ObjPtr& pal = *pAttrBuf;
            pmd->SetAttrList( pal );
            gint32 ret =
                pmd->CheckTimeoutSec();
            if( ERROR( ret )  )
            {
                std::string strMsg = "'timeout' "; 
                strMsg += "does not have valid value";
                PrintMsg( ret, strMsg.c_str() );
                g_bSemanErr = true;
            }
        }
        if( g_bNewSerial )
            pmd->EnableSerialize();
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

method_decl : attr_list TOK_IDENT '(' arg_list ')' TOK_RETURNS '(' ')'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CMethodDecl ) );
        CMethodDecl* pmd = pNode;
        std::string strName = *$2;
        pmd->SetName( strName );
        ObjPtr& pInArgs =  *$4;
        pmd->SetInArgs( pInArgs );
        ENABLE_STREAM( pInArgs, pmd );
        ENABLE_SERIAL( pInArgs, pmd );
        if( g_bNewSerial )
            pmd->EnableSerialize();
        BufPtr pAttrBuf = $1;
        if( ! pAttrBuf.IsEmpty() &&
            ! pAttrBuf->empty() )
        {
            ObjPtr& pal = *pAttrBuf;
            pmd->SetAttrList( pal );
            gint32 ret =
                pmd->CheckTimeoutSec();
            if( ERROR( ret )  )
            {
                std::string strMsg = "'timeout' "; 
                strMsg += "does not have valid value";
                PrintMsg( ret, strMsg.c_str() );
                g_bSemanErr = true;
            }
        }
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

method_decls : method_decl ';'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CMethodDecls ) );
        CMethodDecls* pmds = pNode;
        ObjPtr& pmd = *$1;
        pmds->AddChild( pmd );
        ENABLE_STREAM( pmd, pmds );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;
method_decls : method_decl ';' method_decls
    {
        ObjPtr& pNode = *$3;
        CMethodDecls* pmds = pNode;
        ObjPtr& pmd = *$1;
        CMethodDecl* pMethod = pmd;
        stdstr strName = pMethod->GetName();
        if( pmds->ExistMethod( strName ) )
        {
            std::string strMsg =
                "found duplicated method name '"; 
            strMsg += strName + "', not supported";
            PrintMsg(
                -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        pmds->InsertChild( pmd );
        ENABLE_STREAM( pmd, pmds );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

interf_decl : TOK_INTERFACE TOK_IDENT '{' method_decls '}'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CInterfaceDecl ) );
        CInterfaceDecl* pifd = pNode;
        std::string strName = *$2;
        ObjPtr pTemp;
        gint32 ret = g_mapDecls.GetDeclNode(
            strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            std::string strMsg = "interface '"; 
            strMsg += strName + "'";
            strMsg += " already declared";
            PrintMsg( -EEXIST, strMsg.c_str() );
            g_bSemanErr = true;
        }
        pifd->SetName( strName );
        ObjPtr& pmdl = *$4;
        pifd->SetMethodList( pmdl );
        ENABLE_STREAM( pmdl, pifd );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        g_mapDecls.AddDeclNode( strName, pNode );
        CLEAR_RSYMBS;
    }
    ;

interf_ref : TOK_INTERFACE TOK_IDENT
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CInterfRef ) );
        CInterfRef* pifr = pNode;
        std::string strName = *$2;
        ObjPtr pTemp;
        gint32 ret = g_mapDecls.GetDeclNode(
            strName, pTemp );
        if( ERROR( ret ) )
        {
            std::string strMsg = "interface '"; 
            strMsg += strName + "'";
            strMsg += " not declared yet";
            PrintMsg( -ENOENT, strMsg.c_str() );
            g_bSemanErr = true;
        }
        if( !pTemp.IsEmpty() )
        {
            // CInterfaceDecl* pifd = pTemp;
            // pifd->AddRef();
            ENABLE_STREAM( pTemp, pifr );
        }
        pifr->SetName( strName );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

interf_refs : interf_ref ';'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CInterfRefs ) );
        CInterfRefs* pifrs = pNode;
        ObjPtr& pifr = *$1;
        pifrs->AddChild( pifr );
        ENABLE_STREAM( pifr, pifrs );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

interf_refs : interf_ref ';' interf_refs
    {
        ObjPtr pNode = *$3;
        CInterfRefs* pifrs = pNode;
        ObjPtr& pObj = *$1;
        CInterfRef* pifr = pObj;
        if( pifr != nullptr )
        {
            std::string strName =
                pifr->GetName();
            if( pifrs->ExistIf(
                pifr->GetName() ) )
            {
                std::string strMsg =
                    "duplicated interface '"; 
                strMsg += strName + "'";
                PrintMsg(
                    -EEXIST, strMsg.c_str() );
                g_bSemanErr = true;
            }
        }
        pifrs->InsertChild( pObj );
        ENABLE_STREAM( pObj, pifrs );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

service_decl : TOK_SERVICE TOK_IDENT attr_list '{' interf_refs '}'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CServiceDecl ) );
        CServiceDecl* psd = pNode;
        std::string strName = *$2;
        if( !g_setServices.empty() &&
            g_setServices.find( strName ) ==
            g_setServices.end() )
        {
            BufPtr pBuf( true );
            $$ = pBuf;
            CLEAR_RSYMBS;
            break;
        }
        ObjPtr pTemp;
        gint32 ret = g_mapDecls.GetDeclNode(
            strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            std::string strMsg = "service '"; 
            strMsg += strName + "'";
            strMsg += " already declared";
            PrintMsg( -EEXIST, strMsg.c_str() );
            ret = -EEXIST;
            g_bSemanErr = true;
        }
        psd->SetName( strName );
        BufPtr pAttrBuf = $3;
        if( ! pAttrBuf.IsEmpty() &&
            ! pAttrBuf->empty() )
        {
            ObjPtr& pal = *$3;
            psd->SetAttrList( pal );

            BufPtr pBuf;
            CAttrExps* paes = pal;
            ret = paes->GetAttrByToken(
                TOK_STREAM, pBuf );
            if( SUCCEEDED( ret ) )
                psd->EnableStream();
            ret = 0;
        }
        ObjPtr& pifrs = *$5;
        CInterfRefs* pifrsp = pifrs;
        std::vector< ObjPtr > vecIfs;
        ret = pifrsp->GetIfRefs( vecIfs );
        if( ERROR( ret ) )
        {
            std::string strMsg = "service '"; 
            strMsg += strName + "'";
            strMsg += " has no interface";
            PrintMsg( -EINVAL, strMsg.c_str() );
            ret = -EINVAL;
            g_bSemanErr = true;
        }
        else
        {
            for( auto& elem : vecIfs )
            {
                CInterfRef* pifr = elem;
                ObjPtr pObj;
                ret = pifr->GetIfDecl( pObj );
                if( ERROR( ret ) )
                    continue;
                CInterfaceDecl* pifd = pObj;
                pifd->AddRef();
            }
        }
        psd->SetInterfList( pifrs );

        ENABLE_STREAM( pifrs, psd );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        g_mapDecls.AddDeclNode( strName, pNode );
        CLEAR_RSYMBS;
    }
    ;

typedef_decl : TOK_TYPEDEF idl_type alias_list
    {
        gint32 ret = 0;
        ObjPtr& pType = *$2;

        std::string strType;
        CAliasList* pAliases = *$3;
        do{
            CAstNodeBase* pTypeNode = pType;
            if( pTypeNode->GetClsid() ==
                clsid( CPrimeType ) )
            {
                CPrimeType* ppt = pType;
                if( ppt->GetName() ==
                    TOK_HSTREAM )
                {
                    std::string strMsg =
                    "alias of 'HSTREAM' not"
                    " allowed"; 
                    PrintMsg(
                        -EEXIST, strMsg.c_str() );
                    g_bSemanErr = true;
                    break;
                }
            }
            if( pType->GetClsid() !=
                clsid( CStructRef ) )
            {
                ADDTO_ALIAS_MAP(
                    pAliases, pType );
                break;
            }

            CStructRef* psr = pType;
            strType = psr->GetName();

            ObjPtr pTemp;
            ret = g_mapDecls.GetDeclNode(
                strType, pTemp );
            if( SUCCEEDED( ret ) )
            {
                // alias a struct
                ADDTO_ALIAS_MAP(
                    pAliases, pType );
                break;
            }

            // alias an alias
            ret = g_mapAliases.GetAliasType(
                strType, pType );
            if( SUCCEEDED( ret ) )
            {
                ADDTO_ALIAS_MAP(
                    pAliases, pType );
            }
            else
            {
                std::string strMsg =
                    "typedef '"; 
                strMsg += strType + "'";
                strMsg += " not declared yet";
                PrintMsg( -ENOENT,
                    strMsg.c_str() );
                ret = -ENOENT;
            }

        }while( 0 );

        if( ERROR( ret ) )
            g_bSemanErr = true;

        ObjPtr pNode;
        pNode.NewObj( clsid( CTypedefDecl ) );
        CTypedefDecl* ptd = pNode;
        ptd->SetType( pType );
        ObjPtr& pal = *$3;
        ptd->SetAliasList( pal );

        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    };

alias_list : TOK_IDENT 
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CAliasList ) );
        CAliasList* pal = pNode;
        std::string strIdent = *$1;
        pal->AddChild( strIdent );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

alias_list : TOK_IDENT ',' alias_list
    {
        ObjPtr& pNode = *$3;
        std::string strAlias = *$1;
        CAliasList* pal = pNode;
        pal->InsertChild( strAlias );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

%%


void yyerror( YYLTYPE *locp,
    char const* szPath, char const *msg )
{
    printf( "%s(%d): %s\n",
        szPath, locp->first_line, msg );
}

