/*
 * =====================================================================================
 *
 *       Filename:  gencpp.h
 *
 *    Description:  Declarations of proxy/server generator for C++
 *
 *        Version:  1.0
 *        Created:  02/25/2021 01:34:32 PM
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

#pragma once
#include <memory>
#include <iostream>
#include <fstream>
#include <json/json.h>
#include "rpc.h"

#define GEN_FILEPATH( strRet, strPath, strName, bNew ) \
do{ \
    strRet = ( strPath ) + "/" + ( strName ); \
    gint32 ret = access( \
        strRet.c_str(), F_OK ); \
    if( ret == 0 && bNew ) \
        strRet += ".new"; \
}while( 0 )

#define FUSE_PROXY  1
#define FUSE_SERVER 2
#define FUSE_BOTH   3

#define bFuseP ( ( g_dwFlags & FUSE_PROXY ) > 0 )
#define bFuseS ( ( g_dwFlags & FUSE_SERVER ) > 0 )
#define bFuse ( ( g_dwFlags & FUSE_BOTH ) > 0 )

guint32 GenClsid(
    const std::string& strName );

gint32 GenCppProj(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot );

inline stdstr FormatClsid( guint32 dwClsid )
{
    char szHexId[ 16 ] = {0};
    snprintf( szHexId,
        sizeof( szHexId ) - 1, "0x%08X", dwClsid );
    stdstr strRet = szHexId;
    return strRet;
}

typedef std::unique_ptr< std::ofstream > STMPTR;
typedef std::unique_ptr< std::ifstream > STMIPTR;

struct IFileSet
{
    std::string  m_strPath;
    std::map< std::string, gint32 > m_mapSvcImp;
    std::vector< STMPTR > m_vecFiles;

    IFileSet( const stdstr& strOutPath )
    { m_strPath = strOutPath; }

    virtual gint32 OpenFiles() = 0;
    virtual gint32 AddSvcImpl(
        const std::string& strSvcName ) = 0;
    ~IFileSet();

    const stdstr& GetOutPath() const
    { return m_strPath; }
};

struct CFileSet : public IFileSet
{
    std::string  m_strAppHeader;
    std::string  m_strAppCpp;
    std::string  m_strObjDesc;
    std::string  m_strDriver;
    std::string  m_strMakefile;
    std::string  m_strMainCli;
    std::string  m_strMainSvr;
    std::string  m_strReadme;

    typedef IFileSet super;

    CFileSet( const std::string& strOutPath,
        const std::string& strAppName );

    virtual gint32 OpenFiles();
    virtual gint32 AddSvcImpl(
        const std::string& strSvcName );
    ~CFileSet()
    {}
};

#define COUT ( *m_curFp )
struct CWriterBase
{
    guint32 m_dwIndWid = 4;
    guint32 m_dwIndent = 0;

    std::unique_ptr< IFileSet > m_pFiles;

    std::ofstream* m_curFp = nullptr;
    std::string m_strCurFile;

    CWriterBase(
        const std::string& strPath,
        const std::string& strAppName,
        guint32 dwIndent = 0 )
    {
        m_dwIndent = 0;
    }

    void WriteLine1( const std::string& strText )
    {
        NewLine();
        AppendText( strText );
    }

    void WriteLine0( const std::string& strText )
    {
        AppendText( strText );
        NewLine();
    }

    void IndentUp()
    { m_dwIndent += m_dwIndWid; }

    inline void IndentDown()
    {
        if( m_dwIndent > m_dwIndWid )
            m_dwIndent -= m_dwIndWid;
        else
            m_dwIndent = 0;
    }

    inline void AppendText(
        const std::string& strText )
    { COUT << strText; };

    inline std::string NewLineStr(
        gint32 iCount = 1 ) const
    { 
        std::string strChars( iCount, '\n' );
        if( m_dwIndent > 0 )
            strChars.append( m_dwIndent, ' ' );
        return strChars;
    }

    inline void NewLine( gint32 iCount = 1 )
    { COUT << NewLineStr( iCount ); }

    inline void BlockOpen()
    {
        AppendText( "{" );
        IndentUp();
        NewLine();
    }
    inline void BlockClose()
    {
        IndentDown();
        NewLine();
        AppendText( "}" );
    }

    guint32 SelectFile( guint32 idx )
    {
        if( idx > m_pFiles->m_vecFiles.size() )
            return -ERANGE;
        if( idx < 0 )
            return -ERANGE;
        m_curFp = m_pFiles->
            m_vecFiles[ idx ].get();
        return STATUS_SUCCESS;
    }

    void reset()
    {
        m_dwIndent = 0;
    }

    gint32 AddSvcImpl(
        const std::string& strSvcName )
    {
        return m_pFiles->AddSvcImpl( strSvcName );
    }

    const stdstr& GetCurFile() const
    { return m_strCurFile; }

    const stdstr& GetOutPath() const
    { return m_pFiles->GetOutPath(); }
};

class CCppWriter : public CWriterBase
{
    public:
    ObjPtr m_pNode;
    typedef CWriterBase super;
    CCppWriter(
        const std::string& strPath,
        const std::string& strAppName,
        ObjPtr pStmts ) :
        super( strPath, strAppName )
    {
        std::unique_ptr< IFileSet > ptr(
            new CFileSet( strPath, strAppName ) );
        m_pFiles = std::move( ptr );
        m_pNode = pStmts;
    }

    inline gint32 SelectHeaderFile()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strAppHeader;
        return SelectFile( 0 );
    }

    inline gint32 SelectCppFile()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strAppCpp;
        return SelectFile( 1 );
    }

    inline gint32 SelectDescFile()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strObjDesc;
        return SelectFile( 2 );
    }

    inline gint32 SelectDrvFile()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strDriver;
        return SelectFile( 3 );
    }

    inline gint32 SelectMakefile()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMakefile;
        return SelectFile( 4 );
    }

    inline gint32 SelectMainCli()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainCli;
        return SelectFile( 5 );
    }

    inline gint32 SelectMainSvr()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainSvr;
        return SelectFile( 6 );
    }

    inline gint32 SelectReadme()
    {
        CFileSet* pFiles = static_cast< CFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strReadme;
        return SelectFile( 7 );
    }

    inline gint32 SelectImplFile(
        const std::string& strFile )
    {
        decltype( m_pFiles->m_mapSvcImp )::iterator itr =
        m_pFiles->m_mapSvcImp.find( strFile );
        if( itr == m_pFiles->m_mapSvcImp.end() )
            return -ENOENT;
        gint32 idx = itr->second;
        m_strCurFile = strFile;
        return SelectFile( idx );
    }
};

#define INDENT_UP   m_pWriter->IndentUp()
#define INDENT_DOWN m_pWriter->IndentDown()


#define Wa( text )  m_pWriter->WriteLine0( text )
#define Wb( text )  m_pWriter->WriteLine1( text )

#define AP( text ) \
    m_pWriter->AppendText( text)

#define NEW_LINE m_pWriter->NewLine()
#define NEW_LINES( i ) m_pWriter->NewLine( i )

#define INDENT_UPL   { m_pWriter->IndentUp(); NEW_LINE; }
#define INDENT_DOWNL { m_pWriter->IndentDown(); NEW_LINE; }

#define BLOCK_OPEN  m_pWriter->BlockOpen()
#define BLOCK_CLOSE m_pWriter->BlockClose()

#define CCOUT ( ( std::ofstream& ) ( *m_pWriter->m_curFp ) )

class CHeaderPrologue
{
    CCppWriter* m_pWriter = nullptr;
    CStatements* m_pStmts = nullptr;

    public:
    CHeaderPrologue(
        CCppWriter* pWriter,
        ObjPtr& pNode )
    {
        m_pWriter = pWriter;
        m_pStmts = pNode;
    }
    gint32 Output();
};

class CDeclareClassIds
{
    CCppWriter* m_pWriter = nullptr;
    CStatements* m_pStmts = nullptr;

    public:
    CDeclareClassIds(
        CCppWriter* pWriter,
        ObjPtr& pNode )
    {
        m_pWriter = pWriter;
        m_pStmts = pNode;
        if( m_pStmts == nullptr )
        {
            std::string strMsg =
                DebugMsg( -EFAULT,
                "error empty 'statements' node" );
            throw std::runtime_error( strMsg );
        }
    }
    gint32 Output();
};

class CDeclareTypedef
{
    CCppWriter* m_pWriter = nullptr;
    CTypedefDecl* m_pNode = nullptr;

    public:
    CDeclareTypedef(
        CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CDeclareStruct
{
    CCppWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CDeclareStruct( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
    gint32 OutputFuse();
};

struct CArgListUtils
{
    std::string DeclPtrLocal(
        const std::string& strType,
        const std::string strVar ) const;

    gint32 GetArgCount( ObjPtr& pArgs ) const;

    gint32 ToStringInArgs(
        ObjPtr& pArgs,
        std::vector< std::string >& vecArgs ) const;

    gint32 ToStringOutArgs(
        ObjPtr& pArgs,
        std::vector< std::string >& vecArgs ) const;

    gint32 GenLocals( ObjPtr& pArgList,
        std::vector< std::string >& vecLocals,
        bool bObjPtr = false ) const;

    gint32 GetHstream( ObjPtr& pArgList,
        std::vector< ObjPtr >& vecHstms ) const;

    gint32 GetArgsForCall( ObjPtr& pArgList,
        std::vector< std::string >& vecArgs ) const;

    gint32 GetArgTypes( ObjPtr& pArgList,
        std::vector< std::string >& vecTypes ) const;

    gint32 GetArgTypes( ObjPtr& pArgList,
        std::set< ObjPtr >& vecTypes ) const;

    gint32 FindParentByClsid(
        ObjPtr& pArgList,
        EnumClsid iClsid,
        ObjPtr& pNode ) const;
};

struct CMethodWriter 
    : public CArgListUtils
{
    CWriterBase* m_pWriter = nullptr;

    typedef CArgListUtils super;
    CMethodWriter( CWriterBase* pWriter );

    gint32 GenActParams(
        ObjPtr& pArgList );

    gint32 GenActParams(
        ObjPtr& pArgList,
        ObjPtr& pArgList2 );

    virtual gint32 GenSerialArgs(
        ObjPtr& pArgList,
        const std::string& strBuf,
        bool bDeclare, bool bAssign,
        bool bNoRet = false );

    virtual gint32 GenDeserialArgs(
        ObjPtr& pArgList,
        const std::string& strBuf,
        bool bDeclare, bool bAssign,
        bool bNoRet = false );

    gint32 DeclLocals(
        ObjPtr& pArgList,
        bool bObjPtr = false );

    gint32 GenFormArgs(
        ObjPtr& pArg, bool bIn, bool bShowDir );

    gint32 GenFormInArgs(
        ObjPtr& pArg, bool bShowDir = false )
    { return GenFormArgs( pArg, true, bShowDir ); }

    gint32 GenFormOutArgs(
        ObjPtr& pArg, bool bShowDir = false )
    { return GenFormArgs( pArg, false, bShowDir ); }
};

class CDeclInterfProxy 
    : public CMethodWriter
{
    CInterfaceDecl* m_pNode = nullptr;

    public:
    typedef CMethodWriter super;
    CDeclInterfProxy( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
    gint32 OutputEvent( CMethodDecl* pmd );
    gint32 OutputSync( CMethodDecl* pmd );
    gint32 OutputAsync( CMethodDecl* pmd );
};


class CDeclInterfSvr
    : public CMethodWriter
{
    CInterfaceDecl* m_pNode = nullptr;

    public:

    typedef CMethodWriter super;
    CDeclInterfSvr( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
    gint32 OutputEvent( CMethodDecl* pmd );
    gint32 OutputSync( CMethodDecl* pmd );
    gint32 OutputAsync( CMethodDecl* pmd );
};

class CSetStructRefs :
    public CArgListUtils
{
    CServiceDecl* m_pNode = nullptr;

    public:
    typedef CArgListUtils super;
    CSetStructRefs( ObjPtr& pNode )
    { m_pNode = pNode; }

    gint32 SetStructRefs();

    gint32 ExtractStructArr( ObjPtr& pArray,
        std::set< ObjPtr >& setStructs,
        bool bReffed = true );

    gint32 ExtractStructMap( ObjPtr& pMap,
        std::set< ObjPtr >& setStructs,
        bool bReffed = true );

    gint32 ExtractStructStruct(
        ObjPtr& pStruct,
        std::set< ObjPtr >& setStructs,
        bool bReffed = true );
};

class CDeclService
{
    CCppWriter* m_pWriter = nullptr;
    CServiceDecl* m_pNode = nullptr;

    public:
    CDeclService( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
};

using ABSTE = std::pair< std::string, ObjPtr >;

class CDeclServiceImpl :
    public CMethodWriter
{
    public:
    CServiceDecl* m_pNode = nullptr;
    bool m_bServer = true;
    typedef CMethodWriter super;
    CDeclServiceImpl( CCppWriter* pWriter,
        ObjPtr& pNode, bool bServer );

    gint32 FindAbstMethod(
        std::vector< ABSTE >& vecMethods,
        bool bProxy );

    virtual gint32 DeclAbstMethod(
        ABSTE oMethod,
        bool bProxy,
        bool bComma = true );

    virtual gint32 Output();

    inline bool IsServer() const
    { return m_bServer; }
};

class CImplServiceImpl :
    public CDeclServiceImpl
{
    public:
    typedef CDeclServiceImpl super;
    CImplServiceImpl( CCppWriter* pWriter,
        ObjPtr& pNode, bool bServer )
        : super( pWriter, pNode, bServer )
    {}

    gint32 Output() override;
};


class CImplSerialStruct
{
    CCppWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CImplSerialStruct( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 OutputAssign();
    gint32 OutputSerial();
    gint32 OutputDeserial();
    gint32 Output();
    gint32 OutputFuse();
    gint32 OutputSerialFuse();
    gint32 OutputDeserialFuse();
};

class CImplIufProxy
    : public CArgListUtils
{
    protected:
    CCppWriter* m_pWriter = nullptr;
    CInterfaceDecl* m_pNode = nullptr;

    public:
    typedef CArgListUtils super;
    CImplIufProxy( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CImplIufSvr :
    public CArgListUtils
{
    protected:
    CCppWriter* m_pWriter = nullptr;
    CInterfaceDecl* m_pNode = nullptr;

    public:
    typedef CArgListUtils super;
    CImplIufSvr( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CEmitSerialCode
    : public CArgListUtils
{
    public:
    typedef CArgListUtils super;
    CWriterBase* m_pWriter = nullptr;
    CAstListNode* m_pArgs;

    CEmitSerialCode( CWriterBase* pWriter,
        ObjPtr& pNode );

    gint32 OutputSerial(
        const std::string& strObj,
        const std::string strBuf = "pBuf" );

    gint32 OutputDeserial(
        const std::string& strObj,
        const std::string strBuf = "pBuf" );
};

class CImplIfMethodProxy
    : public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;

    CImplIfMethodProxy(
        CCppWriter* pWriter, ObjPtr& pNode );

    gint32 OutputAsyncCbWrapper();
    gint32 OutputEvent();
    gint32 OutputSync();
    gint32 OutputAsync();
    gint32 Output();
};

class CImplIfMethodSvr
    : public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplIfMethodSvr(
        CCppWriter* pWriter, ObjPtr& pNode );

    gint32 OutputEvent();
    gint32 OutputSync();
    gint32 OutputAsyncCallback();
    gint32 OutputAsyncCancelWrapper();
    gint32 OutputAsyncSerial();
    gint32 OutputAsyncNonSerial();
    gint32 OutputAsync();
    gint32 Output();
};

class CImplClassFactory
{
    bool m_bServer = true;
    CCppWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;
    public:
    CImplClassFactory(
        CCppWriter* pWriter, ObjPtr& pNode, bool bServer );
    gint32 Output();

    inline bool IsServer() const
    { return m_bServer; }
};

class CImplMainFunc :
    public CArgListUtils
{
    CCppWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;
    bool m_bProxy = true;
    public:
    typedef CArgListUtils super;

    CImplMainFunc(
        CCppWriter* pWriter,
        ObjPtr& pNode,
        bool bProxy );
    gint32 Output();
};

struct CExportBase
{
    CStatements* m_pNode = nullptr;
    CWriterBase* m_pWriter = nullptr;
    std::string m_strFile;

    CExportBase( CWriterBase* pWriter,
        ObjPtr& pNode );
    virtual gint32 Output();
};

class CExportMakefile :
    public CExportBase
{
    public:
    typedef CExportBase super;
    CExportMakefile( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CExportObjDesc :
    public CExportBase
{
    public:
    typedef CExportBase super;
    CExportObjDesc( CWriterBase* pWriter,
        ObjPtr& pNode );

    gint32 Output();

    gint32 BuildObjDesc(
        CServiceDecl* psd,
        Json::Value& oElem );
};

class CExportDrivers :
    public CExportBase
{
    public:
    typedef CExportBase super;
    CExportDrivers( CWriterBase* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CExportReadme
{
    protected:
    CWriterBase* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;

    public:
    CExportReadme( CWriterBase* pWriter,
        ObjPtr& pNode );
    gint32 Output();
    gint32 Output_en();
    gint32 Output_cn();
};
