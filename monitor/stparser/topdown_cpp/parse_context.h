#ifndef ST_PARSE_CONTEXT_H
#define ST_PARSE_CONTEXT_H

#include <string>
#include <map>
#include <vector>

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

// Parse context shared between listener and parser predicates
struct CStParseContext {
    // Symbol table for type resolution
    SymbolTable* m_pSymTab = nullptr;

    // Pending type category - set by listener, read by predicates
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
        if (!m_pSymTab) return
            EnumTypeCategory::Invalid;
        return m_pSymTab->lookup(strName);
    }

    // Check if a type is known in the symbol table
    bool IsTypeKnown(const std::string& strName) const
    {
        if (!m_pSymTab) return false;
        return m_pSymTab->isKnown(strName);
    }
};

#endif // ST_PARSE_CONTEXT_H
