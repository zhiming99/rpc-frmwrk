%{
#include "rpc.h"

using namespace rpcfrmwrk;

#include "lexer.h"
#include "astnode.h"
#include <memory>

CDeclMap g_oDeclMap;
ObjPtr g_oRootNode;

extern std::vector< std::unique_ptr< FILECTX > > g_vecBufs;


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
        psr->SetName( strName );

        ObjPtr pTemp;
        gint32 ret = g_oDeclMap.GetDeclNode(
            strName, pTemp );
        if( ERROR( ret ) )
        {
            std::string strMsg = "error '"; 
            strMsg += strName + "'";
            strMsg += " not declared yet";
            PrintMsg( -ENOENT, strMsg.c_str() );
        }
        psr->SetName( strName );
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
        gint32 ret = g_oDeclMap.GetDeclNode(
            strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            std::string strMsg = "error '"; 
            strMsg += strName + "'";
            strMsg += " already declared";
            PrintMsg( -EEXIST, strMsg.c_str() );
        }
        psr->SetName( strName );
        ObjPtr& pfl = *$4;
        psr->SetFieldList( pfl );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        g_oDeclMap.AddDeclNode( strName, pNode );
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

value :
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

attr_exp : attr_name '=' value
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
        gint32 ret = g_oDeclMap.GetDeclNode(
            strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            std::string strMsg = "error interface '"; 
            strMsg += strName + "'";
            strMsg += " already declared";
            PrintMsg( -EEXIST, strMsg.c_str() );
        }
        pifd->SetName( strName );
        ObjPtr& pmdl = *$4;
        pifd->SetMethodList( pmdl );
        BufPtr pBuf( true );
        *pBuf = pNode;
        $$ = pBuf;
        g_oDeclMap.AddDeclNode( strName, pNode );
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
        gint32 ret = g_oDeclMap.GetDeclNode(
            strName, pTemp );
        if( ERROR( ret ) )
        {
            std::string strMsg = "error interface '"; 
            strMsg += strName + "'";
            strMsg += " not declared yet";
            PrintMsg( -ENOENT, strMsg.c_str() );
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
        gint32 ret = g_oDeclMap.GetDeclNode(
            strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            std::string strMsg = "error service '"; 
            strMsg += strName + "'";
            strMsg += " already declared";
            PrintMsg( -EEXIST, strMsg.c_str() );
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
        g_oDeclMap.AddDeclNode( strName, pNode );
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

    END_FACTORY_MAPS;
};

void Usage()
{
    printf( "Usage:" );
    printf( "ridlc [options] <filePath> \n" );
    printf( "\t compile the file"
        "it to current directory" );
    printf( "ridlc -h: print this help");
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
                        printf( "%s",
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
            printf( "Missing file to compile" );
            Usage();
            break;
        }

        std::string strFile = argv[ optind ];

        if( argv[ optind + 1 ] != nullptr )
        {
            printf( "error too many arguments" );
            Usage();
            break;
        }

        ret = yyparse( strFile.c_str() );
        if( g_oRootNode.IsEmpty() )
        {
            ret = -EFAULT;
            printf( "error too many arguments" );
            break;
        }

        g_oDeclMap.Clear();
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

void yyerror( YYLTYPE *locp, char const* szPath, char const *msg )
{
    printf( "%s(%d): %s",
        szPath, locp->first_line, msg );
}

