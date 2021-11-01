/*
 * =====================================================================================
 *
 *       Filename:  genjava.h
 *
 *    Description:  Declarations of classes of proxy/server generator for Java
 *
 *        Version:  1.0
 *        Created:  10/22/2021 11:10:02 PM
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
#include "genpy.h"

using STRPAIR=std::pair< stdstr, stdstr >;

gint32 GenJavaProj(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot );

struct CJavaFileSet : public IFileSet
{
    std::string  m_strFactory;
    std::string  m_strObjDesc;
    std::string  m_strDriver;
    std::string  m_strMakefile;
    std::string  m_strReadme;

    typedef IFileSet super;
    CJavaFileSet( const stdstr& strOutPath,
        const stdstr& strAppName );

    gint32 OpenFiles();

    gint32 AddSvcImpl(
        const std::string& strSvcName );

    gint32 AddStructImpl(
        const std::string& strStructName );

    ~CJavaFileSet();
};

class CJavaWriter : public CWriterBase
{
    public:
    ObjPtr m_pNode;
    typedef CWriterBase super;
    CJavaWriter(
        const std::string& strPath,
        const std::string& strAppName,
        ObjPtr pStmts ) :
        super( strPath, strAppName )
    {
        std::unique_ptr< CJavaFileSet > ptr(
            new CJavaFileSet( strPath, strAppName ) );
        m_pFiles = std::move( ptr );
        m_pNode = pStmts;
    }

    inline gint32 SelectStructsFile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strStructsPy;
        return SelectFile( 0 );
    }

    inline gint32 SelectInitFile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strInitPy;
        return SelectFile( 1 );
    }

    inline gint32 SelectDescFile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strObjDesc;
        return SelectFile( 2 );
    }

    inline gint32 SelectDrvFile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strDriver;
        return SelectFile( 3 );
    }

    inline gint32 SelectMakefile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMakefile;
        return SelectFile( 4 );
    }

    inline gint32 SelectMainCli()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainCli;
        return SelectFile( 5 );
    }

    inline gint32 SelectMainSvr()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainSvr;
        return SelectFile( 6 );
    }

    inline gint32 SelectReadme()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
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

class CJTypeHelper
{
    static std::map< char, stdstr > m_mapTypeCvt =
    {
        { "long", "Long" },
        { "int", "Integer" },
        { "short", "Short" },
        { "boolean", "Boolean" },
        { "byte", "Byte" },
        { "float", "Float" },
        { "double", "Double" }
    };

    static std::map< char, stdstr > m_mapSig2JTp =
    {
        { '(' , "[]" },
        { '[' , "Map" },
        { 'O' ,"Object" },
        { 'Q', "long" },
        { 'q', "long" },
        { 'D', "int" },
        { 'd', "int" },
        { 'W', "short" },
        { 'w', "short" },
        { 'b', "boolean" },
        { 'B', "byte" },
        { 'f', "float" },
        { 'F', "double" },
        { 's', "String" },
        { 'a', "byte[]" },
        { 'o', "ObjPtr" },
        { 'h', "long" }
    };

    static std::map< char, stdstr > m_mapSig2DefVal =
    {
        { '(' , "null" },
        { '[' , "null" },
        { 'O' ,"null" },
        { 'Q', "0" },
        { 'q', "0" },
        { 'D', "0" },
        { 'd', "0" },
        { 'W', "0" },
        { 'w', "0" },
        { 'b', "false" },
        { 'B', "0" },
        { 'f', "0.0" },
        { 'F', "0.0" },
        { 's', "\"\"" },
        { 'a', "null" },
        { 'o', "null" },
        { 'h', "0" }
    };

    public:

    static stdstr GetDefValOfType( ObjPtr pType )
    {
        stdstr strVal;
        stdstr strSig = GetTypeSig( pType );
        if( m_mapSig2DefVal.find( strSig[ 0 ] ) ==
            m_mapSig2DefVal.end() )
        {
            strVal = "null";
        }
        else
        {
            strVal =
                m_mapSig2DefVal[ strSig[ 0 ] ];
        }
        return strVal;
    }

    static gint32 GetObjectPrimitive(
        const stdstr& strType, stdstr& strWrapper );
    {
        if( m_mapTypeCvt.find( strType ) ==
            m_mapTypeCvt.end() )
        {
            strWrapper = strType;
            return 0;
        }
        strWrapper = m_mapTypeCvt[ strType ];
        return 0;
    }

    static gint32 GetArrayTypeText(
        ObjPtr& pObj, stdstr& strText );

    static gint32 GetMapTypeText(
        ObjPtr& pObj, stdstr& strText );

    static gint32 GetStructTypeText(
        ObjPtr& pObj, stdstr& strText );

    static gint32 GetTypeText(
        ObjPtr& pObj, stdstr& strText )

    static gint32 GetFormalArgList(
        ObjPtr& pArgs,
        std::vector< STRPAIR >& vecArgs )

    static gint32 GetActArgList(
        ObjPtr& pArgs,
        std::vector< stdstr >& vecArgs )

    static gint32 GetMethodsOfSvc(
        ObjPtr& pSvc,
        std::vector< ObjPtr >& vecm );
}

class CJavaSnippet
{
    CWriterBase* m_pWriter = nullptr;

    public:
    CJavaSnippet(
        CWriterBase* pWriter )
    { m_pWriter = pWriter; }

    gint32 EmitFormalArgList(
        ObjPtr& pArgs );

    gint32 EmitNewBufPtrSerial(
        guint32 dwSize );

    gint32 EmitByteBufferForDeserial(
        const stdstr& strBuf );

    gint32 EmitSerialArgs(
        ObjPtr& pArgs,
        const stdstr& strName,
        bool bCast )

    gint32 EmitDeserialArgs(
        ObjPtr& pArgs,
        bool bDeclare );

    gint32 EmitDeclArgs(
        ObjPtr& pArgs
        bool bInit );

    gint32 EmitArgClassObj(
        ObjPtr& pArgs );

    gint32 EmitCastArgFromObject(
        ObjPtr& pArgs
        const stdstr& strVar,
        stdstr& strCast );

    gint32 EmitCastArgsFromObject(
        ObjPtr& pMethod, bool bIn,
        const stdstr strObjArr );

    gint32 EmitCatchExcept(
        stdstr strExcept, bool bSetRet );

    gint32 EmitDeclFields(
        ObjPtr& pFields );

    gint32 EmitSerialFields(
        ObjPtr& pFields );

    gint32 EmitDeserialFields(
        ObjPtr& pFields );
};

class CImplJavaMethodSvrBase :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;
    CServiceDecl* m_pSvc = nullptr;

    gint32 ImplNewCancelNotify();
    gint32 ImplInvoke();
    gint32 ImplReqContext();

    public:
    typedef CMethodWriter super;

    CImplJavaMethodSvrBase(
        CJavaWriter* pWriter,
        ObjPtr& pNode,
        ObjPtr& pSvc );

    gint32 DeclAbstractFuncs();
    gint32 OutputReqHandler();
    gint32 OutputEvent();
};

class CImplJavaSvcsvrbase :
{
    CJavaWriter* m_pWriter = nullptr;
    CServiceDecl* m_pSvc = nullptr;
    public:

    CImplJavaSvcsvrbase(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    int Output();
};

class CImplJavaMethodCliBase :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;
    CServiceDecl* m_pSvc = nullptr;

    gint32 ImplAsyncCb();
    gint32 ImplMakeCall();

    public:
    typedef CMethodWriter super;

    CImplJavaMethodCliBase(
        CJavaWriter* pWriter,
        ObjPtr& pNode,
        ObjPtr& pSvc );

    gint32 DeclAbstractFuncs();
    gint32 OutputReqSender();
    gint32 OutputEvent();
};

class CImplJavaSvcclibase :
{
    CJavaWriter* m_pWriter = nullptr;
    CServiceDecl* m_pSvc = nullptr;
    public:

    CImplJavaSvcclibase(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    int Output();
};

class CDeclareStructJava
{
    CJavaWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CDeclareStructJava(
        CJavaWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

