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

using namespace rpcfrmwrk;

#include "lexer.h"
#include "astnode.h"
#include <memory>

CDeclMap g_mapDecls;
ObjPtr g_oRootNode;
CAliasMap g_mapAliases;

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
        { TOK_HSTREAM, int_set }
    };

std::map< gint32, char > g_mapTypeSig = {
    { TOK_UINT64, 'Q' },
    { TOK_INT64, 'q' },
    { TOK_UINT32, 'D' },
    { TOK_INT32, 'd' },
    { TOK_UINT16, 'W' },
    { TOK_INT16, 'w' },
    { TOK_FLOAT, 'f' },
    { TOK_DOUBLE, 'D' },
    { TOK_BOOL, 'b' },
    { TOK_BYTE, 'B' },
    { TOK_HSTREAM, 'h' },
    { TOK_STRING, 's' },
    { TOK_BYTEARR, 'a' },
    { TOK_OBJPTR, 'o' },
    { TOK_STRUCT, 'O' }
};

extern std::vector<
    std::unique_ptr< FILECTX > > g_vecBufs;

void yyerror( YYLTYPE *locp,
    char const* szFile, char const *msg );

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

%}

%define api.value.type {BufPtr}
%define api.pure full

// values
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

//
%token TOK_INTERFACE
%token TOK_STRUCT
%token TOK_SERVICE
%token TOK_TYPEDEF

// operators
%token TOK_RETURNS

%token TOK_ASYNC
%token TOK_ASYNCP
%token TOK_ASYNCS
%token TOK_STREAM
%token TOK_SERIAL
%token TOK_TIMEOUT
%token TOK_RTPATH
%token TOK_SSL
%token TOK_WEBSOCK
%token TOK_COMPRES
%token TOK_AUTH

%token TOK_INVALID

%parse-param { char const *file_name };
%initial-action
{
    YYLTYPE2* pLtype = ( YYLTYPE2* )&@$;
    pLtype->initialize( file_name );
    FILECTX* pfc = new FILECTX( file_name );
    g_vecBufs.push_back(
        std::unique_ptr< FILECTX >( pfc ) );
    yyin = pfc->m_fp;
    char szBuf[ 256 ];
    gint32 ret = fread( szBuf, 1, 256, yyin );
    if( ret == 0 )
        ret = ferror( yyin );
    rewind( yyin );
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
        g_oRootNode = pNode;
    }
    ;
statements : statement ';' statements
    {
        ObjPtr pNode = *$3;
        CStatements* pstmts = pNode;
        ObjPtr& pstmt = *$1;
        pstmts->InsertChild( pstmt );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
        g_oRootNode = pNode;
    }
    ;

statement :
    interf_decl { DEFAULT_ACTION; }
    | service_decl { DEFAULT_ACTION; }
    | struct_decl { DEFAULT_ACTION; }
    | typedef_decl { CLEAR_RSYMBS; }
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
      | TOK_OBJPTR
      | TOK_HSTREAM
    ;

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
        EnumClsid iClsid = pKey->GetClsid();
        if( iClsid == clsid( CArrayType ) ||
            iClsid == clsid( CMapType ) )
        {
            std::string strMsg =
                "map key cannot be array or map"; 
            PrintMsg(
                -ENOENT, strMsg.c_str() );
            g_bSemanErr = true;
        }
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
            psr->SetName( strName );
        else
        {
            ObjPtr pType;
            ret = g_mapAliases.GetAliasType(
                strName, pType );
            if( SUCCEEDED( ret ) )
                psr->SetName( strName );
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
        CStructDecl* psr = pNode;
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
        psr->SetName( strName );
        ObjPtr& pfl = *$4;
        psr->SetFieldList( pfl );
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

        pfdl->SetVal( $4 );
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
    ;

attr_name : 
      TOK_ASYNC { DEFAULT_ACTION; }
    | TOK_ASYNCP { DEFAULT_ACTION; }
    | TOK_ASYNCS { DEFAULT_ACTION; }
    | TOK_STREAM { DEFAULT_ACTION; }
    | TOK_SERIAL { DEFAULT_ACTION; }
    | TOK_TIMEOUT { DEFAULT_ACTION; }
    | TOK_RTPATH { DEFAULT_ACTION; }
    | TOK_SSL { DEFAULT_ACTION; }
    | TOK_WEBSOCK { DEFAULT_ACTION; }
    | TOK_COMPRES { DEFAULT_ACTION; }
    | TOK_AUTH { DEFAULT_ACTION; }
    ;

attr_exp : attr_name
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CAttrExp ) );
        CAttrExp* pae = pNode;
        guint32& dwName = *$1;
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
        pal->InsertChild( pArg );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
    }
    ;

method_decl : TOK_IDENT '(' arg_list ')' TOK_RETURNS '(' arg_list ')'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CMethodDecl ) );
        CMethodDecl* pmd = pNode;
        std::string strName = *$1;
        pmd->SetName( strName );
        ObjPtr& pInArgs = *$3;
        pmd->SetInArgs( pInArgs );
        ObjPtr& pOutArgs =  *$7;
        pmd->SetOutArgs( pOutArgs );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        CLEAR_RSYMBS;
        
    }
    ;
method_decl : TOK_IDENT '(' ')' TOK_RETURNS '(' arg_list ')'
    {
        ObjPtr pNode;
        pNode.NewObj( clsid( CMethodDecl ) );
        CMethodDecl* pmd = pNode;
        std::string strName = *$1;
        pmd->SetName( strName );
        ObjPtr& pOutArgs =  *$6;
        pmd->SetOutArgs( pOutArgs );
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
        pmds->InsertChild( pmd );
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
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        g_mapDecls.AddDeclNode( strName, pNode );
        CLEAR_RSYMBS;
    }
    ;

interf_ref : TOK_INTERFACE TOK_IDENT attr_list
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
        pifr->SetName( strName );
        BufPtr pAttrBuf = $3;
        if( ! pAttrBuf.IsEmpty() &&
            ! pAttrBuf->empty() )
        {
            ObjPtr& pal = *pAttrBuf;
            pifr->SetAttrList( pal );
        }
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
        ObjPtr& pifr = *$1;
        pifrs->InsertChild( pifr );
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
        }
        ObjPtr& pifrs = *$5;
        psd->SetInterfList( pifrs );

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
        CAstNodeBase* pAstNode = pType;

        std::string strType =
            pAstNode->ToString();
        CAliasList* pAliases = *$3;
        do{
            if( pType->GetClsid() !=
                clsid( CStructRef ) )
            {
                ADDTO_ALIAS_MAP(
                    pAliases, pType );
                break;
            }

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
        {
            CLEAR_RSYMBS;
            g_bSemanErr = true;
        }
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// mandatory part, just copy/paste'd from clsids.cpp
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRY( CBuffer );
    INIT_MAP_ENTRY( CAttrExp );
    INIT_MAP_ENTRY( CAttrExps );
    INIT_MAP_ENTRY( CPrimeType );
    INIT_MAP_ENTRY( CArrayType );
    INIT_MAP_ENTRY( CMapType );
    INIT_MAP_ENTRY( CStructRef );
    INIT_MAP_ENTRY( CFieldDecl );
    INIT_MAP_ENTRY( CFieldList );
    INIT_MAP_ENTRY( CStructDecl );
    INIT_MAP_ENTRY( CFormalArg );
    INIT_MAP_ENTRY( CArgList );
    INIT_MAP_ENTRY( CMethodDecl );
    INIT_MAP_ENTRY( CMethodDecls );
    INIT_MAP_ENTRY( CInterfaceDecl );
    INIT_MAP_ENTRY( CInterfRef );
    INIT_MAP_ENTRY( CInterfRefs );
    INIT_MAP_ENTRY( CServiceDecl );
    INIT_MAP_ENTRY( CStatements );
    INIT_MAP_ENTRY( CAliasList );
    INIT_MAP_ENTRY( CTypedefDecl );

    END_FACTORY_MAPS;
};

void Usage()
{
    printf( "Usage:" );
    printf( "ridlc [options] <filePath> \n" );
    printf( "\t compile the file"
        "it to current directory\n" );
    printf( "Options -h:\tprint this help\n");
    printf( "\t-I:\tspecify the path to"
        " search for the included files.\n"
        "\t\tAnd this option can repeat many"
        "times\n" );
}

std::vector< std::string > g_vecPaths;

int main( int argc, char** argv )
{
    gint32 ret = 0;
    bool bUninit = false;
    do{
        ret = CoInitialize( COINIT_NORPC );
        if( ERROR( ret ) )
            break;
        bUninit = true;

        FactoryPtr pFactory = InitClassFactory();
        ret = CoAddClassFactory( pFactory );
        if( ERROR( ret ) )
            break;

        int opt = 0;
        bool bQuit = false;

        while( ( opt =
            getopt( argc, argv, "hI:" ) ) != -1 )
        {
            switch( opt )
            {
            case 'h' :
                {
                    Usage();
                    bQuit = true;
                    break;
                }
            case 'I' :
                {
                    struct stat sb;
                    if (lstat( optarg, &sb) == -1)
                    {
                        ret = -errno;
                        perror( strerror( ret ) );
                        bQuit = true;
                        break;
                    }
                    mode_t iFlags =
                        ( sb.st_mode & S_IFMT );
                    if( iFlags != S_IFDIR )
                    {
                        std::string strMsg =
                            "warning '";

                        strMsg += optarg;
                        strMsg +=
                           "' is not a directory";
                        printf( "%s\n",
                            strMsg.c_str() );
                        break;
                    }
                    g_vecPaths.push_back(
                        std::string( optarg ) );
                    break;
                }
            default:
                break;
            }

            if( bQuit )
                break;
        }

        if( argv[ optind ] == nullptr )
        {
            printf( "Missing file to compile\n" );
            Usage();
            break;
        }

        std::string strFile = argv[ optind ];

        if( argv[ optind + 1 ] != nullptr )
        {
            printf( "too many arguments\n" );
            Usage();
            break;
        }

        ret = yyparse( strFile.c_str() );
        if( g_oRootNode.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        if( g_bSemanErr )
        {
            ret = ERROR_FAIL;
            break;
        }

        printf( "Successfully parsed %s\n",
            strFile.c_str() );

        g_mapDecls.Clear();
        g_oRootNode.Clear();

    }while( 0 );

    if( bUninit )
        CoUninitialize();

    return ret;
}

FILE* TryOpenFile( const std::string& strFile )
{
    FILE* fp = fopen( strFile.c_str(), "r");
    if ( fp != nullptr )
        return fp;

    for( auto elem : g_vecPaths )
    {
        std::string strPath =
            elem + "/" + strFile;
        fp = fopen( strPath.c_str(), "r" );
        if( fp != nullptr )
            break;
    }

    return fp;
}

void yyerror( YYLTYPE *locp,
    char const* szPath, char const *msg )
{
    printf( "%s(%d): %s\n",
        szPath, locp->first_line, msg );
}

