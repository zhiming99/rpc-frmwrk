#ifndef ST_PARSE_LISTENER_H
#define ST_PARSE_LISTENER_H

#include "parse_context.h"
#include "stparser.h"
#include "stparserBaseListener.h"
#include "antlr4-runtime.h"
#include "clsids.h"
using namespace rpcf;

/**
 * Custom listener that resolves type categories before predicates are evaluated.
 * 
 * The flow:
 * 1. Parser enters var_decl_init_set rule
 * 2. enterVar_decl_init_set() is called - we look ahead to find the type name
 * 3. We resolve the type name against the symbol table and set pendingCategory
 * 4. Predicates then check pendingCategory to decide which alternative to take
 */
class CStParseListener : public stparserBaseListener {
public:
    CStParseListener(CStParseContext* ctx) : m_pCtx(ctx), m_pTokenStream(nullptr) {}

    void SetParseContext(CStParseContext* ctx) { m_pCtx = ctx; }
    
    void SetTokenStream(antlr4::TokenStream* ts) { m_pTokenStream = ts; }

    /**
     * Called when entering var_decl_init_set rule.
     * Here we look ahead to find the type name and resolve its category.
     */
    void enterVar_decl_init_set(stparser::Var_decl_init_setContext* ctx) override 
    {}

    /**
     * Reset the context after leaving the rule
     */
    void exitVar_decl_init_set(stparser::Var_decl_init_setContext* ctx) override {
        if (m_pCtx) {
            m_pCtx->m_iPendingCategory = EnumTypeCategory::Invalid;
            m_pCtx->m_strPendingTypeName.clear();
        }
    }

    /**
     * Called when entering var_decl_init_simple_set rule.
     * Further disambiguates between simple_spec, subrange_spec, ref_spec, string_spec.
     */
    void enterVar_decl_init_simple_set(stparser::Var_decl_init_simple_setContext* ctx) override 
    {}

    /**
     * Reset the context after leaving var_decl_init_simple_set
     */
    void exitVar_decl_init_simple_set(stparser::Var_decl_init_simple_setContext* ctx) override {
        // Keep pendingCategory set from parent for potential use
    }

private:
    CStParseContext* m_pCtx = nullptr;
    antlr4::TokenStream* m_pTokenStream = nullptr;
};

#endif // ST_PARSE_LISTENER_H
