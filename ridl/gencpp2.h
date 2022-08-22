/*
 * =====================================================================================
 *
 *       Filename:  gencpp2.h
 *
 *    Description:  Declarations of classes for rpc-over-stream 
 *
 *        Version:  1.0
 *        Created:  08/21/2022 03:34:03 PM
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

class CDeclInterfProxy2 :
    public CDeclInterfProxy
{
    public: 
    typedef CDeclInterfProxy super;
    CDeclInterfProxy2( CCppWriter* pWriter,
        ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}

    gint32 OutputROSSkel();
    gint32 OutputEventROSSkel( CMethodDecl* pmd );
    gint32 OutputAsyncROSSkel( CMethodDecl* pmd );
    gint32 OutputROS();
    gint32 OutputEventROS( CMethodDecl* pmd );
    gint32 OutputSyncROS( CMethodDecl* pmd );
    gint32 OutputAsyncROS( CMethodDecl* pmd );
};

class CDeclInterfSvr2 :
    public CDeclInterfSvr
{
    public: 
    typedef CDeclInterfSvr super;
    CDeclInterfSvr2( CCppWriter* pWriter,
        ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}

    gint32 OutputROS();
    gint32 OutputROSSkel();
    gint32 OutputEventROS( CMethodDecl* pmd );
    gint32 OutputSyncROSSkel( CMethodDecl* pmd );
    gint32 OutputSyncROS( CMethodDecl* pmd );
    gint32 OutputAsyncROSSkel( CMethodDecl* pmd );
    gint32 OutputAsyncROS( CMethodDecl* pmd );
};

class CImplIfMethodSvr2 :
    public CImplIfMethodSvr
{
    public: 
    typedef CImplIfMethodSvr super;
    CImplIfMethodSvr2( CCppWriter* pWriter,
        ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}
    gint32 OutputROS();
    gint32 OutputEventROS();
    gint32 OutputSyncROS();
    gint32 OutputAsyncROS();
};

class CImplIfMethodProxy2 :
    public CImplIfMethodProxy
{
    public: 
    typedef CImplIfMethodProxy super;
    CImplIfMethodProxy2( CCppWriter* pWriter,
        ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}

    gint32 OutputROSSkel();
    gint32 OutputROS();
    gint32 OutputEventROS();
    gint32 OutputAsyncROS();
};

class CDeclService2 :
    public CDeclService
{
    public: 
    typedef CDeclService super;
    CDeclService2( CCppWriter* pWriter,
        ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}

    gint32 OutputROSSkel();
    gint32 OutputROS( bool bServer ); 
};

class CDeclServiceImpl2 :
    public CDeclServiceImpl
{
    public: 
    typedef CDeclServiceImpl super;
    CDeclServiceImpl2( CCppWriter* pWriter,
        ObjPtr& pNode, bool bServer ) :
        super( pWriter, pNode, bServer)
    {}
    gint32 OutputROS();
};

class CImplServiceImpl2 :
    public CImplServiceImpl
{
    public: 
    typedef CImplServiceImpl super;
    CImplServiceImpl2( CCppWriter* pWriter,
        ObjPtr& pNod, bool bServer ) :
        super( pWriter, pNod, bServer )
    {}
    gint32 OutputROS(); 
};

class CImplMainFunc2 :
    public CImplMainFunc
{
    public:
    typedef CImplMainFunc super;
    CImplMainFunc2(
        CCppWriter* pWriter,
        ObjPtr& pNode,
        bool bProxy ) :
        super( pWriter, pNode, bProxy )
    {}
    gint32 OutputROS();
};

class CExportObjDesc2 :
    public CExportObjDesc
{
    public:
    typedef CExportObjDesc super;
    CExportObjDesc2( CWriterBase* pWriter,
        ObjPtr& pNode );

    gint32 OutputROS();

    gint32 BuildObjDescROS(
        CServiceDecl* psd,
        Json::Value& oElem );
};

gint32 GenHeaderFileROS(
    CCppWriter* pWriter, ObjPtr& pRoot );

gint32 GenCppFileROS(
    CCppWriter* m_pWriter, ObjPtr& pRoot );
