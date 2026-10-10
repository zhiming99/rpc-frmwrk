#ifndef ST_PARSER_EXT_H
#define ST_PARSER_EXT_H

#include "stparser.h"
#include "parse_context.h"
#include "clsids.h"

/**
 * Extended parser that adds semantic predicates for type disambiguation.
 * 
 * The predicates check the pendingCategory set by StParseListener
 * to decide which alternative in var_decl_init_set to take.
 */
class CStParser : public stparser {
public:
    CStParser(antlr4::TokenStream* input) :
        stparser(input), m_pContext(nullptr)
    {}

    void SetParseContext(CStParseContext* ctx)
    {
        m_pContext = ctx;
    }

    CStParseContext* GetParseContext() const
    { return m_pContext; }

    gint32 SkipVariableList();

    // ================================================================
    // Semantic predicates for var_decl_init_set disambiguation
    // ================================================================

    void PredictVarDeclInitSimpleSet();
    void PredictVarDeclInit();

    /**
     * Predicate: returns true if the pending type is a simple type (INT, BOOL, etc.)
     */
    bool IsSimpleType() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Invalid )
            PredictVarDeclInit();
        return m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Simple;
    }

    /**
     * Predicate: returns true if the pending type is an array type
     */
    bool IsArrayType() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Invalid )
            PredictVarDeclInit();
        return m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Array;
    }

    /**
     * Predicate: returns true if the pending type is a struct type
     */
    bool IsStructType() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Invalid )
            PredictVarDeclInit();
        return m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Struct;
    }

    /**
     * Predicate: returns true if the pending type is a function block type
     */
    bool IsFBType() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Invalid )
            PredictVarDeclInit();
        return m_pContext->m_iPendingCategory ==
            EnumTypeCategory::FunctionBlock;
    }

    /**
     * Predicate: returns true if the pending type is an interface type
     */
    bool IsInterfaceType() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Invalid )
            PredictVarDeclInit();
        return m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Interface;
    }

    bool IsAmbiguousType() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Invalid )
            PredictVarDeclInit();
        return m_pContext->m_iPendingCategory ==
            EnumTypeCategory::Ambiguous;
    }

    /**
     * Predicate: returns true if the pending type is an enumerated type
     */
    bool IsSimpleSpec() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::Invalid )
            PredictVarDeclInitSimpleSet();
        return m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::SimpleSpec;
    }
    bool IsSubrangeSpec() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::Invalid )
            PredictVarDeclInitSimpleSet();
        return m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::SubrangeSpec;
    }
    /**
     * Predicate: returns true if the pending type is a string type
     */
    bool IsStringSpec() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::Invalid )
            PredictVarDeclInitSimpleSet();
        return m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::StringSpec;
    }

    bool IsRefSpec() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::Invalid )
            PredictVarDeclInitSimpleSet();
        return m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::RefSpec;
    }

    bool IsAmbiguousSpec() {
        if (!m_pContext) return false;
        if( m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::Invalid )
            PredictVarDeclInitSimpleSet();
        return m_pContext->m_iPendingSpec ==
            EnumSimpleSpecType::Invalid;
    }

    bool IsElemTypeName() override;

    /**
     * Helper: get the pending type name (for debugging)
     */
    std::string GetPendingTypeName() const {
        if (!m_pContext) return "";
        return m_pContext->m_strPendingTypeName;
    }

    static bool IsElementaryType(int tokenType);
    static bool IsStringType(int tokenType);

private:
    CStParseContext* m_pContext;
};

#endif // ST_PARSER_EXT_H
