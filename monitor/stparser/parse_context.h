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

// Simple symbol table entry
struct TypeEntry {
    std::string name;
    EnumTypeCategory category;
    // Additional type info can be added here
};

class SymbolTable {
public:
    void addType(
        const std::string& strName,
        EnumTypeCategory cat)
    { types[strName] = {strName, cat}; }

    EnumTypeCategory lookup(const std::string& name) const
    {
        auto it = types.find(name);
        if (it != types.end()) {
            return it->second.category;
        }
        return EnumTypeCategory::Unknown;
    }

    bool isKnown(const std::string& name) const {
        return types.find(name) != types.end();
    }

private:
    std::map<std::string, TypeEntry> types;
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

// Token classification types for tracking your structural markers
enum class ECStVirtualTokenType
{
    None,
    EndIncludeMarker
};

struct CStToken
{
    // Pointer to the raw underlying ANTLR token
    // generated during the lexer pass
    antlr4::Token*       m_pOriginalToken = nullptr;

    // The exact file index inside the include file
    // stack. -1 represents the root source file to
    // parse.
    gint32               m_iFileIdx       = -1;

    // Continuous adjusted global line counter across
    // all included files
    size_t               m_stVirtualLine  = 0;

    // The true, real physical line number inside its
    // original source file
    size_t               m_stOrigLine     = 0;

    // The physical column character offset position
    // inside its original source file
    size_t               m_stCharPos      = 0;

    // Identifies if this is an injected structural
    // marker rather than standard source code
    ECStVirtualTokenType m_eVirtualType   =
        ECStVirtualTokenType::None;

    // Stores custom rewritten pragma payload text
    // (e.g., "{endincl 'path/to/file.st'}")
    std::string          m_strMarkerPath;

    std::shared_ptr<antlr4::CommonToken> m_pBackup;

    CStToken()
    {}

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
struct CStParseContext {

    CStParseContext()
    {
        m_pvecIncludePaths.NewObj();
        m_pvecSrcFiles.NewObj();
    }
    
    // Symbol table for type resolution
    SymbolTable* m_pSymTab = nullptr;

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
        if (!m_pSymTab)
            return EnumTypeCategory::Invalid;
        return m_pSymTab->lookup(strName);
    }

    // Check if a type is known in the symbol table
    bool IsTypeKnown(
        const std::string& strName) const
    {
        if (!m_pSymTab)
            return false;
        return m_pSymTab->isKnown( strName );
    }

    auto GetMacros() const
    { return m_oMacros.m_mapMacros; }

    auto GetMacros()
    { return m_oMacros.m_mapMacros; }
};

#endif // ST_PARSE_CONTEXT_H
