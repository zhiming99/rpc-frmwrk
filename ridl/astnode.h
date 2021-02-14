/*
 * =====================================================================================
 *
 *       Filename:  astnode.h
 *
 *    Description:  declarations of the tree nodes for abstract syntax tree
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
#include <string>
#include <map>
#include "ridlc.h"
#include "rpc.h"
 
#define SET_PARENT_OF( pObj ) \
if( !pObj.IsEmpty() ) \
{ \
    CAstNodeBase* pnode = pObj; \
    pnode->SetParent( this ); \
}

enum EnumMyClsid
{
    DECL_CLSID( CAttrExp ) = clsid( ClassFactoryStart ) + 40,
    DECL_CLSID( CAttrExps ),
    DECL_CLSID( CPrimeType ),
    DECL_CLSID( CArrayType ),
    DECL_CLSID( CMapType ),
    DECL_CLSID( CStructRef ),
    DECL_CLSID( CFieldDecl ),
    DECL_CLSID( CFieldList ),
    DECL_CLSID( CStructDecl ),
    DECL_CLSID( CFormalArg ),
    DECL_CLSID( CArgList ),
    DECL_CLSID( CMethodDecl ),
    DECL_CLSID( CMethodDecls ),
    DECL_CLSID( CInterfaceDecl ),
    DECL_CLSID( CInterfRef ),
    DECL_CLSID( CInterfRefs ),
    DECL_CLSID( CServiceDecl ),
    DECL_CLSID( CStatements ),
};

// declarations
struct CDeclMap
{
    std::map< std::string, ObjPtr > m_mapDecls;
    inline gint32 GetDeclNode(
        const std::string& strName, ObjPtr& pObj )
    {
        std::map< std::string, ObjPtr >::iterator
            itr = m_mapDecls.find( strName );
        if( itr == m_mapDecls.end() )
            return -ENOENT;
        pObj = itr->second;
        return STATUS_SUCCESS;
    }

    inline gint32 AddDeclNode(
        const std::string& strName,
        ObjPtr& pNode )
    {
        std::map< std::string, ObjPtr >::iterator
            itr = m_mapDecls.find( strName );
        if( itr != m_mapDecls.end() )
            return -EEXIST;
        m_mapDecls[ strName ] = pNode;
        return STATUS_SUCCESS;
    }

    inline void Clear()
    { m_mapDecls.clear(); }
};

extern CDeclMap g_oDeclMap;

struct CAstNodeBase :
    public CObjBase
{
    typedef CObjBase super;
    CAstNodeBase():super() {} 
    gint32 m_iType;
    CAstNodeBase* m_pParent;

    inline void SetParent(
        CAstNodeBase* pParent )
    {
        if( pParent != nullptr )
            m_pParent = pParent;
    }
};

struct CNamedNode :
    public CAstNodeBase
{
    typedef CAstNodeBase super;
    std::string m_strName;

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }
};

typedef CAutoPtr< clsid( Invalid ), CAstNodeBase > NodePtr;

struct CAstListNode : CAstNodeBase
{
    std::deque< ObjPtr > m_queChilds;
    typedef CAstNodeBase super;
    CAstListNode() : CAstNodeBase()
    {}

    inline void AddChild( ObjPtr& pChild )
    {
        if( !pChild.IsEmpty() )
        {
            m_queChilds.push_back( pChild );
            SET_PARENT_OF( pChild );
        }
    }

    inline ObjPtr GetChild( guint32 i )
    {
        if( i >= m_queChilds.size() )
            return ObjPtr();
        return m_queChilds[ i ];
    }

    gint32 GetCount()
    { return m_queChilds.size(); }

    void InsertChild( ObjPtr& pChild )
    {
        if( !pChild.IsEmpty() )
        {
            m_queChilds.push_front( pChild );
            SET_PARENT_OF( pChild );
        }
    }
};

struct CAttrExp : public CAstNodeBase
{
    typedef CAstNodeBase super;

    CAttrExp() : CAstNodeBase()
    { SetClassId( clsid( CAttrExp ) ); }
    guint32 m_dwAttrName;
    BufPtr m_pVal;

    inline void SetName( guint32 dwAttrName )
    { m_dwAttrName = dwAttrName; }

    inline guint32 GetName() const
    { return m_dwAttrName; }

    inline void SetVal( BufPtr& pVal )
    { m_pVal = pVal; }

    inline BufPtr& GetVal()
    { return m_pVal; }
};

struct CAttrExps : public CAstListNode
{
    typedef CAstListNode super;
    CAttrExps() : CAstListNode()
    { SetClassId( clsid( CAttrExps ) ); }

};

struct CPrimeType : public CAstNodeBase
{
    guint32 m_dwAttrName;
    typedef CAstNodeBase super;
    CPrimeType() : super()
    { SetClassId( clsid( CPrimeType ) ); }

    inline void SetName( guint32 dwAttrName )
    { m_dwAttrName = dwAttrName; }

    inline guint32 GetName() const
    { return m_dwAttrName; }
};

struct CArrayType : public CPrimeType
{
    typedef CPrimeType super;

    CArrayType() : super()
    { SetClassId( clsid( CArrayType ) ); }

    ObjPtr m_oElemType;

    void SetElemType( ObjPtr& oElem )
    {
        m_oElemType = oElem;
    }

    ObjPtr& GetElemType()
    { return m_oElemType; }
};

struct CMapType : public CArrayType
{
    typedef CArrayType super;
    ObjPtr m_pKeyType;

    CMapType() : super()
    { SetClassId( clsid( CMapType ) ); }

    void SetKeyType( ObjPtr& pKey )
    {
        m_pKeyType = pKey;
        SET_PARENT_OF( pKey );
    }

    ObjPtr& GetKeyType()
    { return m_pKeyType; }
};

struct CStructRef : public CPrimeType
{
    typedef CPrimeType super;

    CStructRef() : super()
    { SetClassId( clsid( CStructRef ) ); }

    std::string m_strName;

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }
};

struct CFieldDecl : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_pType;

    CFieldDecl() : super()
    { SetClassId( clsid( CFieldDecl ) ); }

    void SetType( ObjPtr& pType )
    {
        m_pType = pType;
        SET_PARENT_OF( pType );
    }

    ObjPtr& GetType()
    { return m_pType; }
};

struct CFieldList : public CAstListNode
{
    typedef CAstListNode super;
    CFieldList() : super()
    { SetClassId( clsid( CFieldList ) ); }
};

struct CStructDecl : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_oFieldList;

    CStructDecl() : super()
    { SetClassId( clsid( CStructDecl ) ); }

    void SetFieldList( ObjPtr& oFields )
    {
        m_oFieldList = oFields;
        SET_PARENT_OF( oFields );
    }

    ObjPtr& GetFieldList()
    { return m_oFieldList; }
};

struct CFormalArg : public CFieldDecl
{
    typedef CFieldDecl super;
    CFormalArg() : super()
    { SetClassId( clsid( CFormalArg ) ); }
};

struct CArgList : public CAstListNode
{
    typedef CAstListNode super;
    CArgList() : super()
    { SetClassId( clsid( CArgList ) ); }
};

struct CMethodDecl : public CNamedNode
{
    ObjPtr m_pInArgs;
    ObjPtr m_pOutArgs;

    typedef CNamedNode super;

    CMethodDecl() : super()
    { SetClassId( clsid( CMethodDecl ) ); }

    void SetInArgs( ObjPtr& pInArgs )
    {
        m_pInArgs = pInArgs;
        SET_PARENT_OF( pInArgs );
    }
    ObjPtr& GetInArgs()
    { return m_pInArgs; }

    void SetOutArgs( ObjPtr& pOutArgs )
    {
        m_pOutArgs = pOutArgs;
        SET_PARENT_OF( pOutArgs );
    }
    ObjPtr& GetOutArgs()
    { return m_pOutArgs; }
};

struct CMethodDecls : public CAstListNode
{
    typedef CAstListNode super;
    CMethodDecls() : super()
    { SetClassId( clsid( CMethodDecls ) ); }
};

struct CInterfaceDecl : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_pMdl;

    CInterfaceDecl() : super()
    { SetClassId( clsid( CInterfaceDecl ) ); }

    void SetMethodList( ObjPtr& pMethods )
    {
        m_pMdl = pMethods;
        SET_PARENT_OF( pMethods );
    }

    ObjPtr& GetMethodList()
    { return m_pMdl; }
};

struct CInterfRef : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_pAttrList;

    CInterfRef() : super()
    { SetClassId( clsid( CInterfRef ) ); }

    void SetAttrList( ObjPtr& pAttrList )
    {
        m_pAttrList = pAttrList;
        SET_PARENT_OF( pAttrList );
    }
    ObjPtr& GetAttrList()
    { return m_pAttrList; }
};

struct CInterfRefs : public CAstListNode
{
    typedef CAstListNode super;
    CInterfRefs() : super()
    { SetClassId( clsid( CInterfRefs ) ); }
};

struct CServiceDecl : public CInterfRef
{
    ObjPtr m_pInterfList;
    typedef CInterfRef super;
    CServiceDecl() : super()
    { SetClassId( clsid( CServiceDecl ) ); }

    void SetInterfList( ObjPtr& pInterfList )
    {
        m_pInterfList = pInterfList;
        SET_PARENT_OF( pInterfList );
    }
    ObjPtr& GetInterfList()
    { return m_pInterfList; }
};

struct CStatements : public CAstListNode
{
    typedef CAstListNode super;
    CStatements() : CAstListNode()
    { SetClassId( clsid( CStatements ) ); }
};
