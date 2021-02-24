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
#pragma once

#include <string>
#include <map>
#include "ridlc.h"
#include "rpc.h"
#include "idlclsid.h" 

#define SET_PARENT_OF( pObj ) \
if( !pObj.IsEmpty() ) \
{ \
    CAstNodeBase* pnode = pObj; \
    pnode->SetParent( this ); \
}


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

    inline bool IsDeclared(
        const std::string& strName )
    {
        ObjPtr pObj;
        gint32 ret = GetDeclNode(
            strName, pObj );
        if( SUCCEEDED( ret ) )
            return true;
        return false;
    }

    inline void Clear()
    { m_mapDecls.clear(); }
};

extern CDeclMap g_mapDecls;

struct CAliasMap
{
    std::map< std::string, ObjPtr > m_mapAlias;
    inline gint32 GetAliasType(
        const std::string& strName,
        ObjPtr& pType )
    {
        std::map< std::string, ObjPtr >::iterator
            itr = m_mapAlias.find( strName );
        if( itr == m_mapAlias.end() )
            return -ENOENT;
        pType = itr->second;
        return STATUS_SUCCESS;
    }

    inline gint32 AddAliasType(
        const std::string& strName,
        ObjPtr& pType )
    {
        if( pType.IsEmpty() )
            return -EINVAL;

        std::map< std::string, ObjPtr >::iterator
            itr = m_mapAlias.find( strName );
        if( itr != m_mapAlias.end() )
            return -EEXIST;
        m_mapAlias[ strName ] = pType;
        return STATUS_SUCCESS;
    }
};

extern CAliasMap g_mapAliases;

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

    virtual std::string ToString() const
    { return std::string( "" ); }
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

    std::string ToString() const
    {
        std::string strName;
        switch( m_dwAttrName )
        {
        case TOK_STRING:
            strName = "string";
            break;
        case TOK_UINT64:
            strName = "guint64";
            break;
        case TOK_INT64:
            strName = "gint64";
            break;
        case TOK_UINT32:
            strName = "guint32";
            break;
        case TOK_INT32:
            strName = "gint32";
            break;
        case TOK_UINT16:
            strName = "guint16";
            break;
        case TOK_INT16:
            strName = "gint16";
            break;
        case TOK_FLOAT:
            strName = "float";
            break;
        case TOK_DOUBLE:
            strName = "double";
            break;
        case TOK_BYTE:
            strName = "guint8";
            break;
        case TOK_BOOL:
            strName = "bool";
            break;
        case TOK_BYTEARR:
            strName = "char*";
            break;
        case TOK_OBJPTR:
            strName = "ObjPtr";
            break;
        case TOK_HSTREAM:
            strName = "guint64";
            break;
        default:
            break;
        }
        return strName;
    }
};

struct CArrayType : public CPrimeType
{
    typedef CPrimeType super;

    CArrayType() : super()
    { SetClassId( clsid( CArrayType ) ); }

    ObjPtr m_pElemType;

    void SetElemType( ObjPtr& pElem )
    {
        m_pElemType = pElem;
    }

    ObjPtr& GetElemType()
    { return m_pElemType; }

    std::string ToString() const
    {
        std::string strName;
        strName = "std::vector<";

        CAstNodeBase* pNode = m_pElemType;
        std::string strElem = pNode->ToString();
        strName += strElem;
        strName += ">";
        return strName;
    }
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

    std::string ToString() const
    {
        std::string strName;
        strName = "std::map<";

        CAstNodeBase* pKey = m_pKeyType;
        strName +=
            pKey->ToString() + ",";

        CAstNodeBase* pElem = m_pElemType;
        strName +=
            pElem->ToString();
        strName += ">";
        return strName;
    }
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

    std::string ToString() const
    {
        ObjPtr pTemp;
        if( g_mapDecls.IsDeclared( m_strName ) )
            return m_strName;

        ObjPtr pType;
        gint32 ret = g_mapAliases.GetAliasType(
            m_strName, pType );
        if( SUCCEEDED( ret ) )
        {
            CAstNodeBase* pNode = pType;
            return pNode->ToString();
        }
        return std::string( "");
    }
};

struct CFieldDecl : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_pType;
    BufPtr m_pVal;

    CFieldDecl() : super()
    { SetClassId( clsid( CFieldDecl ) ); }

    void SetType( ObjPtr& pType )
    {
        m_pType = pType;
        SET_PARENT_OF( pType );
    }

    ObjPtr& GetType()
    { return m_pType; }

    inline void SetVal( BufPtr& pVal )
    { m_pVal = pVal; }

    inline BufPtr& GetVal()
    { return m_pVal; }
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

struct CAliasList : public CAstNodeBase
{
    std::deque< std::string > m_queChilds;
    typedef CAstNodeBase super;
    CAliasList() : super()
    { SetClassId( clsid( CAliasList ) ); }

    inline void AddChild( const std::string& strAlias )
    {
        if( !strAlias.empty() )
            m_queChilds.push_back( strAlias );
    }

    inline const std::string GetChild( guint32 i )
    {
        if( i >= m_queChilds.size() )
            return "";
        return m_queChilds[ i ];
    }

    gint32 GetCount()
    { return m_queChilds.size(); }

    void InsertChild( const std::string& strAlias )
    {
        if( !strAlias.empty() )
            m_queChilds.push_front( strAlias );
    }
};

struct CTypedefDecl : public CAstNodeBase
{
    typedef CAstNodeBase super;
    ObjPtr m_pType;
    ObjPtr m_pAliasList;
    CTypedefDecl() : super()
    { SetClassId( clsid( CTypedefDecl ) ); }

    void SetType( ObjPtr& pType )
    {
        m_pType = pType;
        SET_PARENT_OF( pType );
    }
    ObjPtr& GetType()
    { return m_pType; }

    void SetAliasList( ObjPtr& pAliasList )
    {
        m_pAliasList = pAliasList;
        SET_PARENT_OF( pAliasList );
    }
    ObjPtr& GetAliasList()
    { return m_pAliasList; }
};
