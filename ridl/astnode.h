#include <string>
#include <map>
#include "ridl.tab.h++"
#include "rpc.h"
 
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
};

extern CDeclMap g_oDeclMap;

struct CAstNodeBase :
    public CObjBase
{
    CAstNodeBase() {} 
    gint32 m_iType;
    ObjPtr m_pParent;

    inline void SetParent(
        CAstNodeBase* pParent )
    {
        if( pParent != nullptr )
            m_pParent = pParent;
    }
};

typedef CAutoPtr< clsid( Invalid ), CAstNodeBase > NodePtr;

struct CAstTreeNode : CAstNodeBase
{
    ObjPtr m_pSibling;
    std::vector< ObjPtr > m_queChilds;
    typedef CAstNodeBase super;
    CAstTreeNode() : CAstNodeBase()
    {}

    inline void SetSibling( ObjPtr& pSib )
    { m_pSibling = pSib; }

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

struct CAttrExps : public CAstTreeNode
{
    typedef CAstTreeNode super;
    CAttrExps() : CAstTreeNode()
    { SetClassId( clsid( CAttrExps ) ); }

};

struct CPrimeType : public CAstNodeBase
{
    guint32 m_dwAttrName;
    typedef CAstNodeBase super;
    CPrimeType() : CAstNodeBase()
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
    { SetClassId( clsid( CArrayType ); }

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
    { SetClassId( clsid( CMapType ); }

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
    std::string m_strName;

    CStructRef() : super()
    { SetClassId( clsid( CStructRef ); }

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }
};

struct CFieldDecl : public CAstNodeBase
{
    typedef CAstNodeBase super;
    std::string m_strName;
    ObjPtr m_pType;

    CFieldDecl() : super()
    { SetClassId( clsid( CFieldDecl ); }

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }
};

struct CFieldList : public CAstTreeNode
{
    typedef CAstTreeNode super;
    CFieldList() : super()
    { SetClassId( clsid( CFieldList ) ); }
};

struct CStructDecl : public CAstNodeBase
{
    typedef CAstNodeBase super;
    std::string m_strName;
    ObjPtr m_oFieldList;

    CStructDecl() : super()
    { SetClassId( clsid( CStructDecl ); }

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }

    void SetFieldList( ObjPtr& oFields )
    {
        m_oFieldList = oFields;
        SET_PARENT_OF( oFields );
    }

    ObjPtr& GetFieldList()
    { return m_oFieldList; }
};


