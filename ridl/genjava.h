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

#pragma once
#include <sys/stat.h>
#include "rpc.h"
using namespace rpcf;
#include "genpy.h"

using STRPAIR=std::pair< stdstr, stdstr >;

std::string GetTypeSigJava( ObjPtr& pObj );

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
    std::string  m_strMainCli;
    std::string  m_strMainSvr;
    std::string  m_strReadme;
    std::string  m_strDeserialMap;

    typedef IFileSet super;
    CJavaFileSet( const stdstr& strOutPath,
        const stdstr& strAppName );

    void OpenFile(
        const stdstr strName );

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

    inline gint32 SelectFactoryFile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strFactory;
        return SelectFile( 0 );
    }

    inline gint32 SelectDescFile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strObjDesc;
        return SelectFile( 1 );
    }

    inline gint32 SelectDrvFile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strDriver;
        return SelectFile( 2 );
    }

    inline gint32 SelectMakefile()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMakefile;
        return SelectFile( 3 );
    }

    inline gint32 SelectMainCli()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainCli;
        return SelectFile( 4 );
    }

    inline gint32 SelectMainSvr()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainSvr;
        return SelectFile( 5 );
    }

    inline gint32 SelectReadme()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strReadme;
        return SelectFile( 6 );
    }

    inline gint32 SelectDeserialMap()
    {
        CJavaFileSet* pFiles = static_cast< CJavaFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strFactory;
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

    gint32 AddStructImpl(
        const std::string& strSvcName )
    {
        IFileSet* pFiles = m_pFiles.get();
        CJavaFileSet* pJFiles = static_cast
            < CJavaFileSet* >( pFiles );
        return pJFiles->AddStructImpl( strSvcName );
    }
};

class CJTypeHelper
{
    static std::map< stdstr, stdstr > m_mapTypeCvt;
    static std::map< char, stdstr > m_mapSig2JTp;
    static std::map< char, stdstr > m_mapSig2DefVal;

    public:

    static stdstr GetDefValOfType( ObjPtr pType )
    {
        stdstr strVal;
        stdstr strSig = GetTypeSigJava( pType );
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
        const stdstr& strType, stdstr& strWrapper )
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
        ObjPtr& pObj, stdstr& strText );

    static gint32 GetFormalArgList(
        ObjPtr& pArgs,
        std::vector< STRPAIR >& vecArgs );

    static gint32 GetActArgList(
        ObjPtr& pArgs,
        std::vector< stdstr >& vecArgs );

    static gint32 GetMethodsOfSvc(
        ObjPtr& pSvc,
        std::vector< ObjPtr >& vecm );
};

class CJavaSnippet
{
    CWriterBase* m_pWriter = nullptr;

    public:
    CJavaSnippet(
        CWriterBase* pWriter )
    { m_pWriter = pWriter; }

    gint32 EmitBanner();

    gint32 EmitFormalArgList(
        ObjPtr& pArgs );

    gint32 EmitNewBufPtrSerial(
        guint32 dwSize );

    gint32 EmitByteBufferForDeserial(
        const stdstr& strBuf );

    gint32 EmitSerialArgs(
        ObjPtr& pArgs,
        const stdstr& strName,
        bool bCast );

    gint32 EmitDeserialArgs(
        ObjPtr& pArgs,
        bool bDeclare );

    gint32 EmitDeclArgs(
        ObjPtr& pArgs,
        bool bInit,
        bool bLocal = false );

    gint32 EmitArgClassObj(
        ObjPtr& pArgs );

    gint32 EmitCastArgFromObject(
        ObjPtr& pArgs,
        const stdstr& strVar,
        stdstr& strCast );

    gint32 EmitCastArgsFromObject(
        ObjPtr& pMethod, bool bIn,
        const stdstr strObjArr );

    gint32 EmitCatchExcept(
        stdstr strExcept, bool bSetRet );

    void EmitCatchExcepts(
        bool bSetRet );

    gint32 EmitDeclFields(
        ObjPtr& pFields );

    gint32 EmitSerialFields(
        ObjPtr& pFields );

    gint32 EmitDeserialFields(
        ObjPtr& pFields );

    gint32 EmitActArgList(
        ObjPtr& pArgs );

    gint32 EmitGetArgTypes(
        ObjPtr& pArgs );
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
    gint32 ImplSvcComplete();

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

class CImplJavaSvcsvrbase
{
    CJavaWriter* m_pWriter = nullptr;
    CServiceDecl* m_pNode = nullptr;
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

class CImplJavaSvcclibase
{
    CJavaWriter* m_pWriter = nullptr;
    CServiceDecl* m_pNode = nullptr;
    public:

    CImplJavaSvcclibase(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    int Output();
};

class CImplJavaMethodSvr:
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;

    CImplJavaMethodSvr(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
};

class CImplJavaMethodCli:
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;

    CImplJavaMethodCli(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
};

class CImplJavaSvcSvr
{
    CJavaWriter* m_pWriter = nullptr;
    CServiceDecl* m_pNode = nullptr;
    public:

    CImplJavaSvcSvr(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    int Output();
};

class CImplJavaSvcCli
{
    CJavaWriter* m_pWriter = nullptr;
    CServiceDecl* m_pNode = nullptr;
    public:

    CImplJavaSvcCli(
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

class CImplStructFactory
{
    CJavaWriter* m_pWriter;
    CStatements* m_pNode = nullptr;
    public:
    CImplStructFactory(
        CJavaWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CJavaExportMakefile :
    public CExportBase
{
    public:
    typedef CExportBase super;
    CJavaExportMakefile(
        CWriterBase* pWriter,
        ObjPtr& pNode );
    gint32 Output() override;
};

class CJavaExportReadme :
    public CExportReadme
{
    public:
    typedef CExportReadme super;
    CJavaExportReadme(
        CWriterBase* pWriter,
        ObjPtr& pNode )
        : super( pWriter, pNode )
    {}
    gint32 Output();
};

class CImplDeserialMap
{
    CJavaWriter* m_pWriter;
    public:
    CImplDeserialMap(
        CJavaWriter* pWriter );
    gint32 Output();
};

class CImplJavaMainCli
{
    CJavaWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;
    public:

    CImplJavaMainCli(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    int Output();
};

class CImplJavaMainSvr
{
    CJavaWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;
    public:

    CImplJavaMainSvr(
        CJavaWriter* pWriter,
        ObjPtr& pNode );

    int Output();
};
