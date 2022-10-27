/*
 * =====================================================================================
 *
 *       Filename:  genpy2.h
 *
 *    Description:  declarations of classes of python skelton generator
 *
 *        Version:  1.0
 *        Created:  10/09/2022 12:43:09 PM
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

#pragma once
#include <sys/stat.h>
#include "rpc.h"
using namespace rpcf;
#include "../astnode.h"
#include "../gencpp.h"
#include "sha1.h"
#include "../genpy.h"

struct CPyFileSet2 : public IFileSet
{
    stdstr  m_strStructsPy;
    stdstr  m_strInitPy;
    stdstr  m_strMakefile;
    stdstr  m_strMainCli;
    stdstr  m_strMainSvr;
    stdstr  m_strReadme;
    stdstr  m_strIfImpl;

    typedef IFileSet super;
    CPyFileSet2( const std::string& strOutPath,
        const std::string& strAppName );
    gint32 OpenFiles() override;
    gint32 AddSvcImpl(
        const stdstr& strSvcName ) override;
};

class CPyWriter2 : public CPyWriter
{
    public:
    typedef CPyWriter super;
    CPyWriter2(
        const std::string& strPath,
        const std::string& strAppName,
        ObjPtr pStmts ) :
        super( strPath, strAppName )
    {
        std::unique_ptr< CPyFileSet2 > ptr(
            new CPyFileSet2( strPath, strAppName ) );
        m_pFiles = std::move( ptr );
        m_pNode = pStmts;
    }

    gint32 SelectMakefile() override
    {
        auto pFiles = static_cast< CPyFileSet2* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMakefile;
        return SelectFile( 2 );
    }

    gint32 SelectMainCli() override
    {
        auto pFiles = static_cast< CPyFileSet2* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainCli;
        return SelectFile( 3 );
    }

    gint32 SelectMainSvr() override
    {
        auto pFiles = static_cast< CPyFileSet2* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainSvr;
        return SelectFile( 4 );
    }

    gint32 SelectIfImpl()
    {
        auto pFiles = static_cast< CPyFileSet2* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strIfImpl;
        return SelectFile( 5 );
    }

    gint32 SelectReadme() override
    {
        auto pFiles = static_cast< CPyFileSet2* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strReadme;
        return SelectFile( 6 );
    }
};

class CDeclarePyStruct2 :
    public CDeclarePyStruct
{
    public:
    typedef CDeclarePyStruct super;
    CDeclarePyStruct2(
        CPyWriter* pWriter, ObjPtr& pNode )
        : super( pWriter, pNode )
    {}
    gint32 Output() override;
};

struct CPyExportMakefile2
{
    CStatements* m_pNode = nullptr;
    CWriterBase* m_pWriter = nullptr;

    CPyExportMakefile2(
        CWriterBase* pWriter,
        ObjPtr& pNode )
    {
        m_pWriter = pWriter;
        m_pNode = pNode;
    }
    gint32 Output();
};

class CImplPyInterfaces2
{
    CStatements* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyInterfaces2(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CImplPyIfSvrBase2
{
    CInterfaceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyIfSvrBase2(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputDispMsg();
};

class CImplPyMthdSvrBase2 :
    public CImplPyMthdSvrBase
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CImplPyMthdSvrBase super;
    CImplPyMthdSvrBase2(
        CPyWriter* pWriter, ObjPtr& pNode ):
        super( pWriter, pNode )
    {}

    gint32 Output();
    gint32 OutputSync();
    gint32 OutputEvent();
};

class CImplPyMthdProxyBase2 :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplPyMthdProxyBase2(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSync();
    gint32 OutputEvent();
};

class CImplPyIfProxyBase2
{
    CInterfaceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyIfProxyBase2(
        CPyWriter* pWriter, ObjPtr& pNode );

    gint32 Output();
    gint32 OutputDispMsg();
};

class CImplPySvcSvr2
{
    CServiceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPySvcSvr2(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSvcSvrClass();
};

class CImplPyMthdSvr2 :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplPyMthdSvr2(
        CPyWriter* pWriter, ObjPtr& pNode );

    gint32 Output();
};

class CImplPyIfProxy2
{
    CInterfaceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyIfProxy2(
        CPyWriter* pWriter, ObjPtr& pNode );

    gint32 Output();
};

class CImplPyMthdProxy2 :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplPyMthdProxy2(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputAsyncCallback();
    gint32 OutputEvent();
};

class CImplPyMainFunc2 :
    public CArgListUtils
{
    CPyWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;
    public:
    typedef CMethodWriter super;

    CImplPyMainFunc2(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    using SVC_VEC = std::vector< ObjPtr >;
    gint32 OutputCli( CServiceDecl* pSvc );
    gint32 OutputSvr( CServiceDecl* pSvc );
    bool HasEvent( CServiceDecl* pSvc );
    bool HasAsync( CServiceDecl* pSvc );

    gint32 OutputThrdProcCli( CServiceDecl* pSvc );
    gint32 OutputThrdProcSvr( CServiceDecl* pSvc );
};

class CExportPyReadme2 :
    public CExportReadme
{
    public:
    typedef CExportReadme super;
    CExportPyReadme( CWriterBase* pWriter,
        ObjPtr& pNode )
        : super( pWriter, pNode )
    {}
    gint32 Output();
    gint32 Output_en();
    gint32 Output_cn();
};
