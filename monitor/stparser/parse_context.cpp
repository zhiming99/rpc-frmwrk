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
#include "parse_context.h"
#include "pragma_expr/pragma_exprLexer.h"

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
        std::cerr << "CSt Compiler Error: Cyclic "
            << "include detected on file: " 
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
        std::cerr << "stparser Error: Unable "
            << "to open file stream: "
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
        if( pTok != nullptr &&
            pTok->getChannel() ==
                antlr4::Token::DEFAULT_CHANNEL )
        {
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

    if( k < 0 )
    {
        // Handle negative lookback
        // if needed by reversing search
        return nullptr;
    }

    ssize_t sstCurrIdx = static_cast<ssize_t>
        ( m_stCurrentIndex );

    ssize_t sstFoundIdx = -1;
    ssize_t sstCount = 0;

    // Advance k times across matching
    // channel tokens
    while( sstCount < k )
    {
        sstFoundIdx = GetNextTokenOnChannel(
                sstCurrIdx, 1 );

        if( sstFoundIdx == -1 )
            return m_pPipeline-> m_pEofToken.get();

        sstCount++;
        sstCurrIdx = sstFoundIdx + 1;
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
        std::cerr << "Error: 'include' "
            << "directive did not compile. "
            << "Default to false\n";
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
        stop == nullptr ||
        m_pPipeline == nullptr )
    {
        return std::string( "" );
    }

    // Resolve the true absolute vector locations
    ssize_t sstStartIdx =
        FindMasterVectorIndex( start );
    ssize_t sstStopIdx =
        FindMasterVectorIndex( stop );

    if( sstStartIdx == -1 ||
        sstStopIdx == -1 ||
        sstStartIdx > sstStopIdx )
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

