/*
 * =====================================================================================
 *
 *       Filename:  parse_context.h
 *
 *    Description:  Declarations of classes for parser context
 *
 *        Version:  1.0
 *        Created:  09/24/2026 10:02:01 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2026 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

#ifndef ST_PARSE_CONTEXT_H
#define ST_PARSE_CONTEXT_H

#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "stlcont.h"
#include "stsymtab.h"
#include <string>
#include <iostream>
#include <libintl.h> // Standard gettext header

// Macro placeholder for future gettext string scanning
#define _(String) dgettext("st_compiler", String)

namespace rpcf
{
enum class EnumTypeCategory {
    Invalid = -1,
    Unknown = 0,
    Simple,       // INT, BOOL, REAL, etc.
    Array,        // ARRAY OF xxx
    Struct,       // STRUCT ...
    FunctionBlock, // FB or Function
    Interface,    // INTERFACE
    Enumerated,   // ENUM ...
    String,        // STRING, WSTRING
    Ambiguous
};

enum class EnumSimpleSpecType {
    Invalid = -1,
    Unknown = 0,
    SimpleSpec,
    SubrangeSpec,
    RefSpec,
    StringSpec
};

enum class DiagnosticSeverity {
    Warning,
    Error,
    Fatal
};

// Simple symbol table entry
struct TypeEntry {
    std::string name;
    EnumTypeCategory category;
    // Additional type info can be added here
};

struct CStCmdlineMacros
{
    // Maps macro name (normalized UPPERCASE) -> value string
    std::unordered_map<std::string, std::string> m_mapMacros;

    CStCmdlineMacros()
    {}

    inline std::string ToUppercase(
        std::string str)
    {
        std::transform(str.begin(),
            str.end(), str.begin(),
            [](unsigned char c) {
            return std::toupper(c);
        });
        return str;
    }

    void ParseAndRegisterMacro(
        const std::string& strArg)
    {
        size_t idxEq = strArg.find('=');
        if (idxEq == std::string::npos)
        {
            std::string strKey = ToUppercase(strArg);
            m_mapMacros[strKey] = "1";
        }
        else
        {
            std::string strKey =
                ToUppercase(strArg.substr(0, idxEq));
            std::string strVal =
                strArg.substr(idxEq + 1);
            m_mapMacros[strKey] = strVal;
        }
    }

};


// Individual tracking block for coordinate translation
struct CStFileInfo
{
    std::string m_strAbsolutePath;
    int32_t     m_iFileIdx = -1;
};

struct CStIncludeManager
{
    std::vector<CStFileInfo> m_vecFileRegistry;
    std::vector<std::string> m_vecSearchPaths;

    // Active include stack (stores file index positions)
    std::vector<int32_t>     m_vecIncludeStack;

    CStIncludeManager()
    {}

    // Check for cyclical references before opening a file
    bool IsCyclicInclude(
        const std::string& strAbsolutePath,
        int32_t& outExistingIdx );

    int32_t RegisterNewFile(
        const std::string& strAbsolutePath );

    // Push file onto the active compilation stack
    inline void PushFile(int32_t iFileIdx)
    { m_vecIncludeStack.push_back(iFileIdx); }

    inline void PopFile()
    {
        if( !m_vecIncludeStack.empty() )
            m_vecIncludeStack.pop_back();
    }
    inline int32_t GetParentFileIdx() const
    {
        if (m_vecIncludeStack.size() >= 2)
        {
            return m_vecIncludeStack[
                m_vecIncludeStack.size() - 2];
        }
        return -1;
    }

    std::string ResolveIncludePath(
        const std::string& strPath ) const;
};

struct CStTokenPipeline
{
    CStIncludeManager*    m_pIncludeMgr = nullptr;
    std::vector<CStToken> m_vecMasterTokens;
    size_t                m_stGlobalLineCounter = 1;
    antlr4::TokenSource*  m_pRootTokenSource = nullptr;
    std::unique_ptr<antlr4::Token> m_pEofToken;

    using TokStream =
        std::unique_ptr< antlr4::CommonTokenStream>;
    std::vector<TokStream> m_vecIncludeStreams;

    using InStream =
        std::unique_ptr< antlr4::ANTLRInputStream>;
    std::vector<InStream> m_vecStreams;

    using stlexerptr= std::unique_ptr<antlr4::Lexer>;
    std::vector<stlexerptr> m_vecLexers;

    using filestream=std::unique_ptr< std::ifstream>;
    std::vector<filestream> m_vecFileStreams;

    CStTokenPipeline(CStIncludeManager* pMgr) : 
        m_pIncludeMgr(pMgr)
    {
        m_vecMasterTokens.reserve( 500000 );
        auto* pFactory = antlr4::
            CommonTokenFactory::DEFAULT.get();

        m_pEofToken = pFactory->create(
            {nullptr, nullptr},
            antlr4::Token::EOF,
            "",
            antlr4::Token::DEFAULT_CHANNEL,
            0, 0, 0, 0 );
        m_vecIncludeStreams.reserve( 5 );
    }

    bool InjectIncludeStream(
        const std::string& strFilePath,
        antlr4::Token* pOriginalIncludeToken,
        size_t nInsertIndex );

    inline const CStIncludeManager* GetIncludeManager() const
    { return m_pIncludeMgr; }

    inline CStIncludeManager* GetIncludeManager()
    { return m_pIncludeMgr; }
};

class CStParseContext;

class CStPragmaFilteringTokenStream :
    public antlr4::TokenStream
{
public:
    typedef antlr4::TokenStream super;

    CStTokenPipeline* m_pPipeline      = nullptr;
    size_t            m_stCurrentIndex = 0;
    CStParseContext*  m_pContext = nullptr;

    CStPragmaFilteringTokenStream(
        CStTokenPipeline* pPipeline,
        CStParseContext* pCtx ) :
        super(), m_pPipeline(pPipeline),
        m_pContext( pCtx )
    {}

    /** High-Precision Bridge: Extracts the full
     * wrapped tracking structure matching the absolute
     * cursor position inside our master database
     * vector.
     */
    inline const CStToken* GetCustomToken(
        size_t stIndex) const
    {
        if (m_pPipeline == nullptr )
            return nullptr;

        auto& vecMaster =
            m_pPipeline->m_vecMasterTokens;

        if( stIndex < vecMaster.size() )
            return &(vecMaster[stIndex]);

        return nullptr;
    }

    inline const CStTokenPipeline* GetPipeLine() const
    { return m_pPipeline; }

    inline CStTokenPipeline* GetPipeLine()
    { return m_pPipeline; }

    // Overriding ANTLR4's Immutable Stream
    // Navigation Interface
    ssize_t GetNextTokenOnChannel(
        ssize_t sstStartIdx,
        ssize_t sstDirection );

    virtual antlr4::Token* LT( ssize_t k) override;

    virtual size_t LA( ssize_t k) override;

    virtual void consume() override;

    virtual antlr4::Token* get( size_t index) const override;

    virtual size_t size() override;

    virtual size_t index() override;

    virtual void seek( size_t index) override;

    virtual antlr4::TokenSource*
        getTokenSource() const override;

    // High-Precision Continuous Range Text
    // Extraction Layout Overrides

    virtual std::string getText() override;

    virtual std::string getText(
        antlr4::RuleContext* ctx) override;

    virtual std::string getText(
        const antlr4::misc::Interval& interval) override;

    ssize_t FindMasterVectorIndex(
        antlr4::Token* pTargetTok ) const;

    virtual std::string getText(
        antlr4::Token* start,
        antlr4::Token* stop) override;

    virtual ssize_t mark() override;

    virtual void release( ssize_t marker ) override;

    virtual std::string getSourceName() const override;

    bool EvaluateInclude(
        const std::string& strIncText );

    gint32 HandlePragma( CStToken* pPragmaToken );
};


// Parse context shared between listener and parser
// predicates
struct CStParseContext
{
    CStParseContext()
    {
        m_pvecIncludePaths.NewObj();
        m_pvecSrcFiles.NewObj();
    }
    
    // Symbol table for type resolution
    CStSymbolTable m_oSymTab;

    CStCmdlineMacros        m_oMacros;
    rpcf::StrVecPtr         m_pvecIncludePaths;
    rpcf::StrVecPtr         m_pvecSrcFiles;
    CStPragmaFilteringTokenStream*
        m_pMainStream = nullptr;

    // Pending type category - set by listener, read by
    // predicates
    EnumTypeCategory m_iPendingCategory =
        EnumTypeCategory::Invalid;

    EnumSimpleSpecType m_iPendingSpec =
        EnumSimpleSpecType::Invalid;

    // Type name being resolved (for debugging)
    std::string m_strPendingTypeName;

    // Resolved type from symbol table
    EnumTypeCategory ResolveType(
        const std::string& strName) const
    {
        // TODO: add implementation
        m_oSymTab.Lookup(strName);
        return EnumTypeCategory::Invalid;
    }

    // Check if a type is known in the symbol table
    bool IsTypeKnown(
        const std::string& strName) const
    {
        // TODO: add implementation
        return false;
    }

    auto GetMacros() const
    { return m_oMacros.m_mapMacros; }

    auto GetMacros()
    { return m_oMacros.m_mapMacros; }

    gint32 DeclareNamespace(
        const std::string& sFirst,
        const std::vector<stparser::Namespace_nameContext*>& rest);

    void LeaveNamespace(
        const std::string& sFirst,
        const std::vector<stparser::Namespace_nameContext*>& rest);

    template<typename... Args>
    void LogDiagnostic(
        DiagnosticSeverity eSeverity,
        const std::string& strFileName,
        size_t uLine,
        size_t uCharPosition,
        const char* szMsgId,
        Args... args)
    {
        const char* szFormat = _(szMsgId);

        int iSize = std::snprintf(
            nullptr, 0, szFormat, args...) + 1;

        if (iSize <= 0)
            return;

        std::vector<char> vecBuf(iSize);
        std::snprintf(vecBuf.data(),
            vecBuf.size(), szFormat, args...);

        // Resolve localized severity label
        const char* szLabel = _("error: ");
        if (eSeverity == DiagnosticSeverity::Warning) {
            szLabel = _("warning: ");
        } else if (eSeverity == DiagnosticSeverity::Fatal) {
            szLabel = _("fatal error: ");
        }

        std::cerr << strFileName << ":"
                  << uLine << ":"
                  << uCharPosition << ": "
                  << szLabel
                  << vecBuf.data() << "\n";

        if (eSeverity == DiagnosticSeverity::Warning) {
            m_dwWarningCount++;
        } else {
            m_dwErrorCount++; // Errors and Fatals halt gen
        }
    }

    template<typename... Args>
    void ForwardDiagnostic(
        DiagnosticSeverity eSeverity,
        antlr4::Token* pToken,
        const char* szMsgId,
        Args... args)
    {
        // Avoid crashing on corrupted lookahead symbols
        if (!pToken)
            return;

        // Resolve source file tracking layer

        auto idx = m_pMainStream->
            FindMasterVectorIndex( pToken );
        if( idx == -1 )
            return;
        auto pTok = m_pMainStream->
            GetCustomToken( idx );
        auto pPipeline =
            m_pMainStream->GetPipeLine();
        auto pIncMgr =
            pPipeline->GetIncludeManager();

        auto oFileInfo = 
            pIncMgr->m_vecFileRegistry[ pTok->m_iFileIdx ];

        std::string strFile;
        if( oFileInfo.m_strAbsolutePath.size() )
            strFile = basename(
                oFileInfo.m_strAbsolutePath.c_str() );

        size_t uLine = pToken->getLine();
        size_t uCol = pToken->getCharPositionInLine();

        LogDiagnostic(eSeverity, strFile, uLine,
            uCol, szMsgId, args...);
    }

    template<typename... Args>
    void LogWarning(antlr4::Token* pToken, 
                    const char* szMsgId, Args... args) {
        ForwardDiagnostic(DiagnosticSeverity::Warning, 
            pToken, szMsgId, args...);
    }

    template<typename... Args>
    void LogError(antlr4::Token* pToken, 
          const char* szMsgId, Args... args)
    {
        ForwardDiagnostic(DiagnosticSeverity::Error, 
            pToken, szMsgId, args...);
    }

    guint32 m_dwWarningCount = 0;
    guint32 m_dwErrorCount = 0;
};

std::string GetReconstructedText(
    antlr4::ParserRuleContext* pCtx,
    antlr4::TokenStream* pTokens,
    size_t offset = 0,
    size_t roffset = 0 );

/**
 * @brief Why ResolveNsPrefix() stopped
 */
enum enumPathStop
{
    //! the whole path was resolved
    pathStopResolved = 0,
    //! the next component exists, but it is not a namespace,
    //! e.g. a variable, an FB instance or a struct
    pathStopNonNamespace,
    //! the next component is not declared (yet); for a using
    //! directive this is a forward reference to retry later
    pathStopUnresolved,
};

/**
 * @brief Resolve a namespace path and report where it stopped
 *
 * Same lookup rules as ResolvePath(), but it resolves only the
 * namespace prefix of a qualified name and reports how far it
 * got:
 *
 * - success: the resolved CStSymbol is returned and iPos is the
 *   index of the last component in vecNames;
 * - failure: the CStNamespaceDecl of the innermost namespace is
 *   returned and iPos is the index of that namespace's name in
 *   vecNames.  vecNames[iPos + 1] is therefore the component
 *   that stopped the walk.
 *
 * A stop has two different meanings, reported through eStop:
 *
 * - pathStopNonNamespace: the namespace part ended and
 *   vecNames[iPos + 1] names a variable, an FB instance, a
 *   struct or another non-namespace symbol.  The caller resolves
 *   the rest in the scope of the returned node.
 * - pathStopUnresolved: vecNames[iPos + 1] is not declared yet.
 *   For a using directive this is a forward reference: keep the
 *   (namespace, iPos) anchor and retry once the unit has been
 *   fully declared.
 *
 * The first (innermost) lexical level that binds vecNames[0]
 * decides the anchor: once that level's namespace chain ends, no
 * outer level is consulted, so a local binding is never shadowed
 * by a longer namespace path further out.
 *
 * When not even the first component resolves as a namespace,
 * iPos is set to (size_t)-1 and the returned node is the
 * namespace enclosing the starting scope.  For an absolute path
 * the global namespace is reported instead, with iPos == 0.
 *
 * @param vecNames the path components as produced by
 *                 BreakdownQualified(), e.g. {"@","A","B"}.
 *                 '@' (GLOBAL_SCOPE_MARK) is the internal id of
 *                 the global scope, standing for the leading '_'
 *                 of an ST qualified name.  It is kept distinct
 *                 from '_' so that a path whose leading component
 *                 really is the identifier '_' (e.g. '_._.B')
 *                 cannot be mistaken for the global root.
 * @param pCtx     the CStParseContext carrying the symbol table
 * @param iPos     [out] index of the last resolved component
 * @param eStop    [out] why the walk stopped, see enumPathStop
 * @return the resolved symbol on success, otherwise the innermost
 *         CStNamespaceDecl reached
 */
ObjPtr ResolveNsPrefix(
    const std::vector<std::string>& vecNames,
    void* pCtx,
    size_t& iPos,
    enumPathStop& eStop );

gint32 ResolveUsingNs(
    void* pctx, ObjPtr pHostNode );

gint32 ResolveTypeRef( CStParseContext* pctx,
    const std::vector<std::string>& vecNames,
    enumSymKind eKind,
    std::vector< ObjPtr >& vecUsings,
    CStScope* pCurScope,
    SymPtr& pSym );

bool IsElementaryType( gint32 iType );

bool IsCompoundType( gint32 iType );
bool IsDirectType( gint32 iType );

gint32 ResolveTypeToDirectType(
    CStParseContext* pctx,
    const std::vector<std::string>& vecNames,
    enumSymKind eKind,
    std::vector< ObjPtr >& vecUsings,
    CStScope* pCurScope,
    SymPtr& pSym, gint32& iType );

gint32 ReplaceAmbiguousNode(
    CStParseContext* pCtx,
    CStAmbiguousTypeDeclNode* ptd,
    gint32 iDirectType,
    ObjPtr& pNode );
}

#endif // ST_PARSE_CONTEXT_H
