/*
 * =====================================================================================
 *
 *       Filename:  parse_context.cpp
 *
 *    Description:  implementations of classes for parser context
 *
 *        Version:  1.0
 *        Created:  10/06/2026 10:11:37 PM
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

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <regex>
#include "antlr4-runtime.h"
#include "stlexer.h"
#include "stparser.h"
#include "pragma.h"
#include "defines.h"
#include "parse_context.h"
#include "pragma_expr/pragma_exprLexer.h"

namespace rpcf
{

std::string CStIncludeManager::ResolveIncludePath(
    const std::string& strPath ) const
{
    namespace fs = std::filesystem;
    
    if( strPath.empty() )
        return std::string( "" );

    fs::path pathTarget( strPath );

    // Verify absolute paths instantly
    if( pathTarget.is_absolute() )
    {
        if( fs::exists( pathTarget ) )
            return fs::canonical( 
                pathTarget ).string();
        return std::string( "" );
    }

    // 1. Resolve relative to active file dir
    if( !m_vecIncludeStack.empty() )
    {
        gint32 iActiveIdx = 
            m_vecIncludeStack.back();
            
        if( iActiveIdx >= 0 && 
            iActiveIdx < static_cast<gint32>(
                m_vecFileRegistry.size()) )
        {
            fs::path pathCurrentFile( 
                m_vecFileRegistry[iActiveIdx]
                    .m_strAbsolutePath );
                    
            fs::path pathParentDir = 
                pathCurrentFile.parent_path();
                
            fs::path pathCombined = 
                pathParentDir / pathTarget;

            if( fs::exists( pathCombined ) )
            {
                return fs::canonical( 
                    pathCombined ).string();
            }
        }
    }

    // 2. Iterate registered -I flags
    for( const auto& strSearchDir : m_vecSearchPaths )
    {
        if( strSearchDir.empty() )
            continue;

        fs::path pathBase( strSearchDir );
        fs::path pathCombined = 
            pathBase / pathTarget;

        if( fs::exists( pathCombined ) )
            return fs::canonical( 
                pathCombined ).string();
    }

    // 3. Check current execution working dir
    if( fs::exists( pathTarget ) )
    {
        return fs::canonical( 
            pathTarget ).string();
    }

    return std::string( "" );
}

bool CStIncludeManager::IsCyclicInclude(
    const std::string& strAbsolutePath,
    int32_t& outExistingIdx)
{
    namespace fs = std::filesystem;
    if (!fs::exists(strAbsolutePath))
        return false;

    std::string strCanonical =
        fs::canonical(strAbsolutePath).string();

    // Find if it already exists in the registry
    int32_t iFoundIdx = -1;
    for (const auto& fileInfo : m_vecFileRegistry)
    {
        if (fileInfo.m_strAbsolutePath == strCanonical)
        {
            iFoundIdx = fileInfo.m_iFileIdx;
            break;
        }
    }

    if (iFoundIdx != -1)
    {
        outExistingIdx = iFoundIdx;
        // If the registered index is currently
        // sitting inside the active stack, it's a
        // loop!
        auto it = std::find(
            m_vecIncludeStack.begin(),
            m_vecIncludeStack.end(),
            iFoundIdx);
        return (it != m_vecIncludeStack.end());
    }

    outExistingIdx = -1;
    return false;
}

int32_t CStIncludeManager::RegisterNewFile(
    const std::string& strAbsolutePath)
{
    namespace fs = std::filesystem;
    CStFileInfo info;
    info.m_strAbsolutePath =
        fs::canonical(strAbsolutePath).string();

    info.m_iFileIdx =
        static_cast<int32_t>(m_vecFileRegistry.size());

    m_vecFileRegistry.push_back(info);
    return info.m_iFileIdx;
}

bool CStTokenPipeline::InjectIncludeStream(
    const std::string& strFilePath,
    antlr4::Token* pOriginalIncludeToken,
    size_t nInsertIndex )
{
    gint32 iFileIdx = -1;

    // Cyclic verification
    if( m_pIncludeMgr->IsCyclicInclude(
        strFilePath, iFileIdx ) )
    {
        std::cerr << "stparsr error: Cyclic include detected on file: " 
            << strFilePath << "\n";
        return false;
    }

    if (iFileIdx == -1)
        iFileIdx = m_pIncludeMgr->RegisterNewFile(
            strFilePath );

    m_pIncludeMgr->PushFile(iFileIdx);

    // Standard ANTLR file stream payload
    // extraction pass
    auto pFileStream = std::make_unique<
        std::ifstream>( strFilePath );
    if( !pFileStream->is_open() )
    {
        std::cerr << "stparser error: Unable to open file stream: "
            << strFilePath << "\n";
        m_pIncludeMgr->PopFile();
        return false;
    }

    auto pInput = std::make_unique
        <antlr4::ANTLRInputStream>( *pFileStream );

    auto pLexer = std::make_unique<stlexer>( pInput.get() );

    auto pTokenStream = std::make_unique
        <antlr4::CommonTokenStream>( pLexer.get() );
    pTokenStream->fill();

    std::vector<antlr4::Token*> vecRawTokens =
        pTokenStream->getTokens();

    m_vecIncludeStreams.push_back(
        std::move( pTokenStream ) );

    m_vecStreams.push_back( std::move( pInput ) );
    m_vecLexers.push_back( std::move( pLexer ) );
    m_vecFileStreams.push_back(
        std::move( pFileStream ) );

    std::vector<CStToken> vecFileTokens;
    vecFileTokens.reserve( vecRawTokens.size() + 1 );

    size_t stFileLastLine = 1;

    for( auto* pTok : vecRawTokens )
    {
        if (pTok->getType() == antlr4::Token::EOF)
            continue;

        CStToken cstTok;

        cstTok.m_pOriginalToken = pTok;

        cstTok.m_iFileIdx = iFileIdx;

        cstTok.m_stOrigLine = pTok->getLine();

        cstTok.m_stCharPos =
            pTok->getCharPositionInLine();

        cstTok.m_stVirtualLine =
            m_stGlobalLineCounter + (pTok->getLine() - 1);

        stFileLastLine = pTok->getLine();

        vecFileTokens.push_back( cstTok );
    }

    m_stGlobalLineCounter += stFileLastLine;

    // Append your cloned endmarker pragma token
    // directly behind this file's payload
    if (pOriginalIncludeToken != nullptr)
    {
        CStToken endMarkerTok;

        std::string strText =
            "{endincl '" + strFilePath + "'}";

        auto pFactory = antlr4::
            CommonTokenFactory::DEFAULT.get();

        auto pUnique = pFactory->create(
            {nullptr, nullptr},
            stlexer::TOK_PRAGMA, strText,
            antlr4::Token::DEFAULT_CHANNEL,
            0, 0, 0, 0 );

        endMarkerTok.m_pBackup =
            std::move( pUnique );

        endMarkerTok.m_pOriginalToken =
            endMarkerTok.m_pBackup.get();

        endMarkerTok.m_iFileIdx =
            m_pIncludeMgr->GetParentFileIdx();

        endMarkerTok.m_stOrigLine =
            pOriginalIncludeToken->getLine();

        endMarkerTok.m_stCharPos =
            pOriginalIncludeToken->getCharPositionInLine();

        endMarkerTok.m_stVirtualLine =
            m_stGlobalLineCounter;

        endMarkerTok.m_eVirtualType =
            ECStVirtualTokenType::EndIncludeMarker;

        endMarkerTok.m_strMarkerPath = strText;

        m_vecMasterTokens[ nInsertIndex ] = endMarkerTok;
    }

    // Safely insert all wrapped tokens inline
    // directly ahead of the include token's spot
    m_vecMasterTokens.insert(
        m_vecMasterTokens.begin() + nInsertIndex,
        vecFileTokens.begin(),
        vecFileTokens.end()
    );

    return true;
}

// --- ANTLR4 Core Stream Interface ---

// Helper method to find the next valid
// token on the target channel
ssize_t CStPragmaFilteringTokenStream::
GetNextTokenOnChannel(
    ssize_t sstStartIdx,
    ssize_t sstDirection )
{
    if( m_pPipeline == nullptr )
        return -1;

    auto& vecMaster =
        m_pPipeline->m_vecMasterTokens;
    ssize_t sstMax = static_cast<ssize_t>
        ( vecMaster.size());

    ssize_t sstCurr = sstStartIdx;
    while( sstCurr >= 0 &&
       sstCurr < sstMax )
    {
        auto* pTok =
            vecMaster[sstCurr].m_pOriginalToken;

        // Verify token sits on the
        // DEFAULT_CHANNEL (usually 0)
        if( pTok != nullptr )
        {
            if( pTok->getType() == antlr4::Token::EOF )
                return sstCurr;

            if( pTok->getChannel() ==
                    antlr4::Token::DEFAULT_CHANNEL )
                return sstCurr;
        }

        sstCurr += sstDirection;
    }

    return -1;
}

// Refactored lookahead supporting
// channel filters
antlr4::Token*
CStPragmaFilteringTokenStream::LT(
    ssize_t k )
{
    if( m_pPipeline == nullptr ||
        k == 0 )
    {
        return nullptr;
    }

    ssize_t direction = 1;
    if( k < 0 )
    {
        // Handle negative lookback
        // if needed by reversing search
        direction = -1;
        k = -k;
    }

    ssize_t sstCurrIdx = static_cast<ssize_t>
        ( m_stCurrentIndex );

    ssize_t sstFoundIdx = -1;
    ssize_t sstCount = 0;

    // Advance k times across matching
    // channel tokens
    if( direction < 0 )
        sstCurrIdx--;
    while( sstCount < k )
    {
        sstFoundIdx = GetNextTokenOnChannel(
                sstCurrIdx, direction );

        if( sstFoundIdx == -1 )
        {
            if( direction > 0 )
                return m_pPipeline-> m_pEofToken.get();
            else
                return nullptr;
        }

        sstCount++;
        sstCurrIdx = sstFoundIdx + direction;
    }

    return m_pPipeline->
        m_vecMasterTokens[sstFoundIdx]
            .m_pOriginalToken;
}


bool CStPragmaFilteringTokenStream::EvaluateInclude(
    const std::string& strIncText )
{
    antlr4::ANTLRInputStream input(strIncText);
    pragma_exprLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    // Use the extended parser with predicates
    CStPragmaParser parser(&tokens);

    parser.SetParseContext( m_pContext );

    // Invoke the root rule of your micro-grammar
    // to build the parse tree.  This triggers the
    // LL(*) state machine over the pragma
    // expression tokens.
    auto pTree = parser.include_directive();
    if (pTree == nullptr)
        return false;

    CStPragmaEvaluator evaluator( m_pContext );
    std::any anyResult = evaluator.visit(pTree);

    // Extract and return the underlying boolean
    // value safely.
    try
    {
        bool bOutcome =
            std::any_cast<bool>(anyResult);
        return bOutcome;
    }
    catch (const std::bad_any_cast& e)
    {
        std::cerr << "stparser error: 'include' directive did not compile. Default to false\n";
        return false;
    }
    return false;
}

gint32 CStPragmaFilteringTokenStream::HandlePragma(
    CStToken* pPragmaToken )
{
    auto pToken =
        pPragmaToken->m_pOriginalToken;
    guint32 dwType = pToken->getType();
    if( dwType == stlexer::TOK_PRAGMA )
    {
        std::string strText =
            pToken->getText();

        std::regex reIf(
            "^\\{[ \t]*if[ \t]+",
            std::regex_constants::icase );

        std::regex reInclude(
            "^\\{[ \t]*include[ \t]+",
            std::regex_constants::icase );

        std::regex reEndIncl(
            "^\\{[ \t]*endincl[ \t]*}?",
            std::regex_constants::icase );


        // Handle conditional pruning blocks
        if( std::regex_search( strText, reIf ) )
        {
            /*size_t nStartIndex = pTokens->index();

            size_t nNextIndex = 
                ProcessAndPruneWholeBlock(
                    pTokens, nStartIndex );

            pTokens->seek( nNextIndex );
            */
            return 1;
        }

        // Handle End of Include marker token
        // boundary transitions
        if( std::regex_search( strText, reEndIncl ) )
        {
            size_t nStartIndex = this->index();

            auto pPipeline =
                this->GetPipeLine();
            auto pIncMgr =
                pPipeline->GetIncludeManager();

            if( pIncMgr != nullptr )
            {
                gint32 iParentIdx =
                    pIncMgr->GetParentFileIdx();

                auto& vecMaster =
                    pPipeline->m_vecMasterTokens;
                if( vecMaster.size() > nStartIndex )
                {
                    auto& oToken =
                        vecMaster[ nStartIndex ];
                    if( oToken.m_iFileIdx ==
                        iParentIdx )
                    {
                        pIncMgr->PopFile();
                    }
                }
            }

            // Advance past the marker token
            // seamlessly so it does not trigger
            // parser errors
            this->consume();
            return 1;
        }

        // Handle standalone inline active
        // inclusion paths if skipped or evaluated
        // on the fly
        if( std::regex_search( strText, reInclude ) )
        {
            // Optional: Handle inline processing
            // steps if needed or skip active
            // markers
            std::string strClause =
                strText.substr(1, strText.size() - 2);
            EvaluateInclude( strClause );
        }
    }
    return 0;
}
// Refactored consume tracking
void CStPragmaFilteringTokenStream::consume()
{
    if( m_pPipeline == nullptr )
        return;

    if( m_stCurrentIndex == ( size_t )-1 )
        return;

    // Locate the current active token
    // position on the default channel
    ssize_t sstActiveIdx = GetNextTokenOnChannel(
            m_stCurrentIndex, 1 );

    if( sstActiveIdx != -1 )
    {
        // Advance past the consumed token
        m_stCurrentIndex = GetNextTokenOnChannel(
            sstActiveIdx + 1, 1 );
        if( m_stCurrentIndex == ( size_t )-1 )
            return;
    }
    else
    {
        return;
    }
    auto& vecMaster =
        m_pPipeline->m_vecMasterTokens;

    auto dwIdx = m_stCurrentIndex;
    auto itr = vecMaster.begin() + dwIdx;
    while( itr != vecMaster.end() )
    {
        auto& oToken = *itr;
        auto pToken = oToken.m_pOriginalToken;
        if( pToken->getChannel() !=
            antlr4::Token::DEFAULT_CHANNEL )
        {
            itr++;
            dwIdx++;
            continue;
        }
        if( pToken->getType() !=
            stlexer::TOK_PRAGMA )
            break;
        auto iSavedIdx = m_stCurrentIndex;
        seek( dwIdx );
        if( HandlePragma( &oToken ) == 0 )
            seek( iSavedIdx );
        break;
    }
}


size_t CStPragmaFilteringTokenStream::LA(
    ssize_t k)
{
    antlr4::Token* pTok = LT(k);
    if( pTok != nullptr )
    {
        return pTok->getType();
    }
    return antlr4::Token::INVALID_TYPE;
}

antlr4::Token* CStPragmaFilteringTokenStream::get(
    size_t index) const
{
    if( m_pPipeline == nullptr || 
        index >= m_pPipeline->
            m_vecMasterTokens.size() )
    {
        return m_pPipeline->m_pEofToken.get();
    }
    return m_pPipeline->
        m_vecMasterTokens[index]
            .m_pOriginalToken;
}

size_t CStPragmaFilteringTokenStream::size()
{
    return m_pPipeline != nullptr ? 
        m_pPipeline->m_vecMasterTokens.size() : 0;
}

size_t CStPragmaFilteringTokenStream::index()
{
    return m_stCurrentIndex;
}

void CStPragmaFilteringTokenStream::seek(
    size_t index)
{
    m_stCurrentIndex = index;
}

antlr4::TokenSource* 
CStPragmaFilteringTokenStream::getTokenSource() const
{
    if( m_pPipeline != nullptr )
        return m_pPipeline->m_pRootTokenSource;
    return nullptr;
}

// --- Range Text Extractions ---

std::string CStPragmaFilteringTokenStream::getText()
{
    if( m_pPipeline == nullptr || 
        m_pPipeline-> m_vecMasterTokens.empty() ) 
        return "";

    auto& vecMaster = 
        m_pPipeline->m_vecMasterTokens;

    return getText(
        vecMaster.front().m_pOriginalToken, 
        vecMaster.back().m_pOriginalToken);
}

std::string CStPragmaFilteringTokenStream::getText(
    antlr4::RuleContext* ctx)
{
    if( ctx == nullptr )
        return "";

    auto* pParserCtx = dynamic_cast<
        antlr4::ParserRuleContext*>( ctx );
        
    if( pParserCtx == nullptr )
        return "";

    return getText( 
        pParserCtx->start, pParserCtx->stop );
}

std::string CStPragmaFilteringTokenStream::getText(
    const antlr4::misc::Interval& interval)
{
    if( m_pPipeline == nullptr || 
        m_pPipeline->
            m_vecMasterTokens.empty() ) 
    {
        return "";
    }
    size_t stStart = 
        static_cast<size_t>(interval.a);
    size_t stStop  = 
        static_cast<size_t>(interval.b);
    
    auto& vecMaster = 
        m_pPipeline->m_vecMasterTokens;
    if( stStart >= vecMaster.size() ) 
        return "";

    if( stStop >= vecMaster.size() ) 
    {
        stStop = vecMaster.size() - 1;
    }
    
    return getText(
        vecMaster[stStart].m_pOriginalToken, 
        vecMaster[stStop].m_pOriginalToken);
}

// Finds the absolute index of a token pointer
// inside the master tracking vector database
ssize_t CStPragmaFilteringTokenStream::
FindMasterVectorIndex(
    antlr4::Token* pTargetTok ) const
{
    if( pTargetTok == nullptr ||
        m_pPipeline == nullptr )
    {
        return -1;
    }

    auto& vecMaster =
        m_pPipeline->m_vecMasterTokens;

    auto it = std::find_if(
        vecMaster.begin(),
        vecMaster.end(),
        [pTargetTok](const CStToken& wrapped) {
            return wrapped.m_pOriginalToken ==
                   pTargetTok;
        });

    if( it != vecMaster.end() )
    {
        return std::distance(
            vecMaster.begin(), it );
    }

    return -1;
}

// Fixed continuous range text extraction layout
std::string CStPragmaFilteringTokenStream::getText(
    antlr4::Token* start,
    antlr4::Token* stop )
{
    if( start == nullptr ||
        m_pPipeline == nullptr )
    {
        return std::string( "" );
    }

    // Resolve the true absolute vector locations
    ssize_t sstStartIdx =
        FindMasterVectorIndex( start );
    ssize_t sstStopIdx = ( stop != nullptr ?
        FindMasterVectorIndex( stop ): -1 );

    if( sstStartIdx == -1 ||
        ( sstStartIdx > sstStopIdx && 
          sstStopIdx >= 0 ) )
    {
        return std::string( "" );
    }

    // Single virtual {endincl} payload intercept
    if( sstStartIdx == sstStopIdx )
    {
        const CStToken& oCStTok =
            m_pPipeline->
                m_vecMasterTokens[sstStartIdx];

        if( oCStTok.m_eVirtualType ==
            ECStVirtualTokenType::
                EndIncludeMarker )
        {
            return oCStTok.m_strMarkerPath;
        }
    }

    if( sstStopIdx == -1 )
        sstStopIdx = sstStartIdx;

    std::string strResult = "";
    for( ssize_t i = sstStartIdx;
         i <= sstStopIdx; ++i )
    {
        const CStToken& oCStTok =
            m_pPipeline->
                m_vecMasterTokens[i];

        if( oCStTok.m_eVirtualType ==
            ECStVirtualTokenType::
                EndIncludeMarker )
        {
            strResult +=
                oCStTok.m_strMarkerPath;
        }
        else if( oCStTok.m_pOriginalToken
                 != nullptr )
        {
            // Includes all channels (comments/WS)
            strResult += oCStTok.
                m_pOriginalToken->getText();
        }
    }
    return strResult;
}


// Establishes a speculative lookahead 
// checkpoint and returns a unique marker ID
ssize_t CStPragmaFilteringTokenStream::mark()
{
    // ANTLR4 expects a unique index tracking 
    // marker. Returning the current absolute 
    // stream index acts as a perfect marker ID.
    return static_cast<ssize_t>(
        m_stCurrentIndex );
}

// Releases a previously allocated checkpoint
void CStPragmaFilteringTokenStream::
    release( ssize_t marker )
{
    // Because our vector buffer is stable 
    // and managed inside m_vecMasterTokens, 
    // no physical allocation occurs during 
    // mark(). This can be a safe no-op block.
    (void)marker; 
}

// Returns the active source stream identifier 
// text for diagnostic compilers log formatters
std::string CStPragmaFilteringTokenStream::
    getSourceName() const
{
    std::string strDefault(
        "CStVirtualEngineStream" );

    if( m_pPipeline == nullptr )
        return strDefault;

    auto& ts = 
        m_pPipeline->m_pRootTokenSource;

    if( ts != nullptr )
        return ts->getSourceName();
    
    return strDefault;
}

gint32 CStParseContext::DeclareNamespace(
    const std::string& strFirst,
    const std::vector<stparser::Namespace_nameContext*>& rest) 
{
    gint32 ret = 0;
    do{
        // Construct the complete trailing path
        std::vector< std::string > vecComps;
        vecComps.push_back( strFirst );
        for( size_t i = 0; i < rest.size(); i++ )
            vecComps.push_back( rest[i]->getText() );

        bool bCreate = false;
        for( auto strName : vecComps )
        {
            SymVector sv;
            m_oSymTab.FindByKind( symNamespace, sv );
            stdstr strDup;
            if( sv.size() )
            {
                auto it = std::find_if(
                    sv.begin(), sv.end(),
                    [ strName ](const SymPtr& pSym) {
                        return pSym->m_strName == strName;
                    });
                if( it != sv.end() )
                {
                    SymPtr pDup = *it;
                    sv.clear();
                    sv.push_back( pDup );
                }
                else
                {
                    sv.clear();
                }
            }

            if( sv.empty() )
            {
                ObjPtr pObj;
                ret = pObj.NewObj(
                    clsid( CStNamespaceDecl ) );
                if( ERROR( ret ) )
                    break;
                auto pnsd = ( CStNamespaceDecl* )pObj;
                bCreate = true;
                auto pSym = m_oSymTab.DeclareScopeOwner(
                    symNamespace,
                    scopeNamespace,
                    strName, pObj );
                if( !pSym.IsEmpty() )
                {
                    pnsd->m_pSymbol = pSym;
                    auto pScope =
                        m_oSymTab.GetCurrentScope();
                    pScope->m_pSymbol = pSym;
                }
                continue;
            }
            else if( !sv.empty() && bCreate )
            {
                std::cerr << "stparser error: Duplicated namespace name'"
                    << strName << "'\n";
                ret = -EEXIST;
                break;
            }
            else
            {
                SymPtr pSym = sv[0];
                if( pSym.IsEmpty() )
                {
                    std::cerr << "stparser error: Scope object is empty '"
                        << strName << "'\n";
                    ret = -EFAULT;
                    break;
                }
                ScopePtr pScope( pSym->m_pDefScope );
                m_oSymTab.PushScope( pScope );
            }
        }

    }while( 0 );
    return ret;
}

void CStParseContext::LeaveNamespace(
    const std::string& strFirst,
    const std::vector<stparser::Namespace_nameContext*>& rest)
{
    // Calculate the total number of layers pushed: 1
    // (for strFirst) + number of trailing parts
    size_t uTotalLayers = 1 + rest.size();

    // Safely unwind the LIFO scope stack layers
    for (size_t i = 0; i < uTotalLayers; ++i)
        m_oSymTab.PopScope();
}

std::string GetReconstructedText(
    antlr4::ParserRuleContext* pCtx,
    antlr4::TokenStream* pTokens,
    size_t offset,
    size_t roffset )
{
    if (!pCtx || !pTokens ||
        !pCtx->start || !pCtx->stop) {
        return "";
    }

    std::string strResult = "";
    size_t uStartIdx =
        pCtx->start->getTokenIndex() + offset;
    size_t uStopIdx =
        pCtx->stop->getTokenIndex() - roffset;

    // Loop through the token pipeline indices
    // sequentially
    for (size_t i = uStartIdx; i <= uStopIdx; ++i) {
        antlr4::Token* pTok = pTokens->get(i);
        if (!pTok ||
            ( pTok->getType() == antlr4::Token::EOF ) ||
            ( pTok->getChannel() !=
                antlr4::Token::DEFAULT_CHANNEL ) )
            continue;

        std::string strTokText = pTok->getText();
        if (strTokText.empty())
            continue;

        if (!strResult.empty()) {
            strResult += ",";
        }
        strResult += strTokText;
    }

    return strResult;
}

ObjPtr ResolveNsPrefix(
    const std::vector<std::string>& vecNames,
    void* pctx,
    size_t& iPos,
    enumPathStop& eStop )
{
    ObjPtr pResult;
    iPos = ( size_t )-1;
    eStop = pathStopUnresolved;

    if( vecNames.empty() || pctx == nullptr )
        return pResult;

    auto pCtx = reinterpret_cast
        < CStParseContext* >( pctx );
    auto& ost = pCtx->m_oSymTab;

    CStScope* pCurScope = ost.GetCurrentScope();

    bool bAbsolute =
        ( vecNames[0] == GLOBAL_SCOPE_MARK );
    size_t iStartIdx = bAbsolute ? 1 : 0;

    while( pCurScope != nullptr || bAbsolute )
    {
        CStScope* pSearchRunner =
            ( !bAbsolute ? pCurScope
            : ( CStScope* )ost.GetGlobalScope() );

        SymPtr pLastNs;
        size_t iLastIdx = ( size_t )-1;
        bool bChainValid = true;

        for( size_t i = iStartIdx;
            i < vecNames.size(); ++i )
        {
            const std::string& strName = vecNames[i];
            SymVector vecSyms;
            ost.FindByKind(symNamespace,
                vecSyms, pSearchRunner);

            CStScope* pNextScope = nullptr;
            SymPtr pNextNs;
            for( auto& elem : vecSyms )
            {
                if( elem->m_strName == strName )
                {
                    pNextScope = elem->m_pDefScope;
                    pNextNs = elem;
                    break;
                }
            }

            if( pNextScope == nullptr )
            {
                bChainValid = false;
                break; // Break component chain loop
            }

            pLastNs = pNextNs;
            iLastIdx = i;
            pSearchRunner = pNextScope;
        }

        if (bChainValid)
        {
            if( iLastIdx == ( size_t )-1 )
            {
                // only the absolute root marker was
                // given
                iPos = 0;
                ScopePtr pGlobal = ost.GetGlobalScope();
                if( pGlobal.IsEmpty() ||
                    pGlobal->m_pSymbol.IsEmpty() )
                    return pResult;
                eStop = pathStopResolved;
                return ObjPtr( pGlobal->m_pSymbol );
            }
            iPos = iLastIdx;
            eStop = pathStopResolved;
            return ObjPtr( pLastNs );
        }

        // The namespace prefix ends at this level.  Do
        // not fall back to outer scopes, or they would
        // shadow the locally bound first component.
        // Tell the two possible stops apart: a
        // non-namespace symbol means the caller
        // continues in pLastNs's scope, while a
        // missing symbol means the name is not
        // declared yet (a using directive may retry it
        // later).
        if( iLastIdx != ( size_t )-1 )
        {
            iPos = iLastIdx;
            if( iLastIdx + 1 < vecNames.size() )
            {
                SymPtr pNextSym = pSearchRunner->LookupLocal(
                    vecNames[ iLastIdx + 1 ] );
                if( !pNextSym.IsEmpty() &&
                    pNextSym->m_eKind != symNamespace )
                    eStop = pathStopNonNamespace;
            }
            return pLastNs->m_pDeclNode;
        }

        // If forced absolute or already at root, fail
        if (bAbsolute ||
            pCurScope == ost.GetGlobalScope())
            break;

        // Fall back one level up the lexical tree
        pCurScope = pCurScope->m_pParent;

        // Final fallback step: Try the absolute root
        if (pCurScope == nullptr)
            bAbsolute = true;
    }

    // Nothing matched at all: anchor at the global
    // namespace for an absolute path, otherwise at the
    // namespace enclosing the scope resolution started
    // from.
    if( bAbsolute )
    {
        iPos = 0;
        ScopePtr pGlobal = ost.GetGlobalScope();
        if( pGlobal.IsEmpty() ||
            pGlobal->m_pSymbol.IsEmpty() )
            return pResult;
        return pGlobal->m_pSymbol->m_pDeclNode;
    }

    CStScope* pScope = ost.GetCurrentScope();
    while( pScope != nullptr )
    {
        if( ( pScope->m_eKind == scopeNamespace ||
              pScope->m_eKind == scopeGlobal ) &&
            !pScope->m_pSymbol.IsEmpty() )
            return pScope->m_pSymbol->m_pDeclNode;
        pScope = pScope->m_pParent;
    }

    return pResult; // Return null if unfound
}

static std::vector< ObjPtr >
GetUsingNs( ObjPtr& pObj )
{
    CStNamespaceDecl* pnsd = pObj;
    if( pnsd != nullptr )
        return pnsd->m_vecUsings;
    CStDataTypeDeclNode* pdtd = pObj;
    if( pdtd != nullptr )
        return pdtd->m_vecUsings;
    return std::vector< ObjPtr >();
}

gint32 ResolveUsingNs(
    void* pctx, ObjPtr pHostNode )
{
    gint32 ret = 0;
    do{
        auto pCtx = reinterpret_cast
            < CStParseContext* >( pctx );
        auto ost = pCtx->m_oSymTab;
        DebugPrint( ret, "Entering ResolveUsingNs" );
        bool bUnresolved = false;
        auto vecUsings = GetUsingNs( pHostNode );
        for( auto pObj : vecUsings )
        {
            CStUsingDirective* pud = pObj;
            if( pud == nullptr )
                continue;
            auto& vecNames = pud->m_vecNamespaces;
            for( auto& elem: vecNames )
            {
                if( !elem.second.IsEmpty() )
                    continue;

                auto vecComps = 
                    ost.BreakdownQualified( elem.first );

                if( vecComps.empty() )
                    continue;

                auto pSym = CStNamespaceDecl::ResolvePath(
                    vecComps, pctx );
                if( !pSym.IsEmpty() )
                {
                    elem.second = SymPtr( pSym );
                    DebugPrint( 0,
                    "Resoved namespace reference %s",
                    elem.first.c_str() );
                }
                else
                {
                    DebugPrint( 0,
                    "undefined namespace reference %s",
                    elem.first.c_str() );
                    bUnresolved = true;
                }
            }
        }
        if( bUnresolved )
        {
            ost.AddUnsolved( pHostNode,
                ([]( ObjPtr& pObj, void* pctx )->gint32
                {
                    return ResolveUsingNs( pctx, pObj );
                }));
        }
    }while( 0 );
    return ret;
}

gint32 ResolveTypeRef( CStParseContext* pCtx,
    const std::vector<std::string>& vecNames,
    enumSymKind eKind,
    std::vector< ObjPtr >& vecUsings,
    CStScope* pTopScope,
    SymPtr& pSym )
{
    pSym.Clear();

    if( vecNames.empty() || pCtx == nullptr )
        return -EINVAL;

    auto ost = pCtx->m_oSymTab;
    const std::string& strName = vecNames.back();

    bool bAbsolute = false;
    if( vecNames[ 0 ] == GLOBAL_SCOPE_MARK )
        bAbsolute = true;

    if( bAbsolute && vecNames.size() == 1 )
    {
        std::cerr << "stparser error: invalid '_'\n"; 
        return -EINVAL;
    }

    std::vector<std::string> vecPrefixPath;
    size_t uStart = bAbsolute ? 1 : 0;
    if (vecNames.size() > 1)
        vecPrefixPath.insert(vecPrefixPath.begin(),
            vecNames.begin() + uStart,
            vecNames.end() - 1);

    // 2. ABSOLUTE PATH LOOKUP ENGINE
    if( bAbsolute )
    {
        ScopePtr pScope = ost.GetGlobalScope();
        bool bChainValid = true;

        for( const auto& strPart : vecPrefixPath )
        {
            SymVector vecOut;
            ost.LookupAllLocal(
                strPart, pScope, vecOut );
            ScopePtr pNextScope;
            for( auto elem : vecOut )
            {
                if( elem->m_strName == strPart && 
                    elem->m_eKind == symNamespace )
                {
                    pNextScope = elem->m_pDefScope;
                    break;
                }
            }
            if( pNextScope.IsEmpty() )
            {
                bChainValid = false;
                break;
            }
            pScope = pNextScope;
        }

        if( bChainValid && !pScope.IsEmpty() )
        {
            SymVector vecOut;
            ost.LookupAllLocal(
                strName, pScope, vecOut );
            for( auto elem : vecOut )
            {
                if( elem->m_eKind == eKind &&
                    elem->m_strName == strName )
                {
                    pSym = elem;
                    return 0;
                }
            }
        }
        return -ENOENT;
    }

    // 3. RELATIVE LEXICAL PASS (No Imports Blended)
    ScopePtr pStartScope = (
        pTopScope == nullptr ?
        ost.GetCurrentScope():
        ScopePtr( pTopScope ) );
    ScopePtr pCurScope = pStartScope;

    while( !pCurScope.IsEmpty() )
    {
        ScopePtr pRunScope = pCurScope;
        bool bPathOk = true;

        for( const auto& strPart : vecPrefixPath )
        {
            SymVector vecOut;
            ost.LookupAllLocal(
                strPart, pRunScope, vecOut );
            ScopePtr pNext;
            for( auto elem : vecOut )
            {
                if( elem->m_strName == strPart &&
                    elem->m_eKind == symNamespace )
                {
                    pNext = elem->m_pDefScope;
                    break;
                }
            }
            if( pNext.IsEmpty() )
            {
                bPathOk = false;
                break;
            }
            pRunScope = pNext;
        }

        if( bPathOk && !pRunScope.IsEmpty() )
        {
            SymVector vecOut;
            ost.LookupAllLocal(
                strName, pRunScope, vecOut );
            for( auto elem : vecOut )
            {
                if( elem->m_eKind == eKind &&
                    elem->m_strName == strName )
                {
                    pSym = elem;
                    return 0;
                }
            }
        }
        pCurScope = pCurScope->m_pParent;
    }

    // 4. IMPORTED DIRECTIVES PASS (Evaluate USING)
    for( auto pObj : vecUsings )
    {
        CStUsingDirective* pud = pObj;
        if( pud == nullptr )
            continue;

        for( auto& elem : pud->m_vecNamespaces )
        {
            if( !elem.second.IsEmpty() )
                continue;

            auto vecComps = ost.BreakdownQualified(
                elem.first
            );
            if( vecComps.empty() )
                continue;

            ScopePtr pRunScope;
            size_t uStartIdx = 0;
            
            if( vecComps[ 0 ] == GLOBAL_SCOPE_MARK )
            {
                pRunScope = ost.GetGlobalScope();
                uStartIdx = 1;
            }
            else
                pRunScope = pStartScope;

            vecComps.insert( vecComps.end(),
                vecPrefixPath.begin(),
                vecPrefixPath.end() );

            bool bPathOk = true;
            for( size_t i = uStartIdx;
                 i < vecComps.size(); ++i )
            {
                const auto& strPart = vecComps[ i ];
                SymVector vecOut;
                ost.LookupAllLocal( strPart, pRunScope,
                                    vecOut );
                ScopePtr pNext;
                for( auto elem : vecOut )
                {
                    if( elem->m_strName == strPart &&
                        elem->m_eKind == symNamespace )
                    {
                        pNext = elem->m_pDefScope;
                        break;
                    }
                }
                if( pNext.IsEmpty() )
                {
                    bPathOk = false;
                    break;
                }
                pRunScope = pNext;
            }

            if( bPathOk && !pRunScope.IsEmpty() )
            {
                SymVector vecOut;
                ost.LookupAllLocal(
                    strName, pRunScope, vecOut );
                for( auto elem : vecOut )
                {
                    if( elem->m_eKind == eKind &&
                        elem->m_strName == strName )
                    {
                        pSym = elem;
                        return 0;
                    }
                }
            }
        }
    }

    return -ENOENT;
}

bool IsCompositeType( gint32 iType )
{
    static gint32 arrTypes[] = {
        stparser::TOK_RANGE, stparser::TOK_ARRAY,
        stparser::TOK_STRUCT,
        stparser::TOK_STRING_TYPE,
        stparser::TOK_REF, stparser::TOK_CLASS,
        stparser::TOK_INTERFACE,
        stparser::TOK_FUNCTION,
        stparser::TOK_FUNCTION_BLOCK,
        stparser::TOK_NAMESPACE, stparser::TOK_PROGRAM,
        stparser::TOK_PROPERTY, stparser::TOK_RESOURCE,
        stparser::TOK_TASK,
    };
    constexpr size_t iSize =
        sizeof( arrTypes )/sizeof( gint32 );
    for( size_t i = 0; i < iSize; i++ )
    {
        if( iType == arrTypes[ i ] )
            return true;
    }
    return false;
}

bool IsGrammarType( gint32 iType )
{
    if( CStParser::IsElementaryType( iType ) ||
        IsCompositeType( iType ) )
        return true;
    return false;
}

gint32 ResolveTypeToBaseType(
    CStParseContext* pCtx,
    const std::vector<std::string>& vecNames,
    enumSymKind eKind,
    std::vector< ObjPtr >& vecUsings,
    CStScope* pTopScope,
    int& iType )
{
    gint32 ret = 0;
    do{
        iType = -1;
        SymPtr pSym;
        ret = ResolveTypeRef( pCtx,
            vecNames, eKind, vecUsings,
            pTopScope, pSym );
        if( ERROR( ret ) )
            break;
LBL_RETRY:
        ObjPtr pNode = pSym->m_pDeclNode;
        if( pNode.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }
        CStTypeDeclNode* ptd = pNode;
        if( ptd == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( IsGrammarType( ptd->m_iTokid ) )
            iType = ptd->m_iTokid;

        if( iType != -1 )
            break;

        if( ptd->m_oTypeRef.second != nullptr )
        {
            pSym = ptd->m_oTypeRef.second;
            goto LBL_RETRY;
        }
        ret = -ENOENT;
        break;

    }while( 0 );
    return ret;
}

}
