#pragma once

#include "clang/AST/Decl.h"
#include "clang/Basic/SourceLocation.h"
#include <string>
#include <vector>

namespace statik {

enum class VariableScope {
    LOOP_LOCAL,      // declared inside the loop
    FUNCTION_LOCAL,  // declared in function but outside loop
    GLOBAL          
};

enum class VariableRole {
    INDUCTION_VAR,  
    DATA_VAR,       
    ARRAY_INDEX     
};

struct VariableUsage {
    clang::SourceLocation location;
    unsigned line_number;
    bool is_read;
    bool is_write;

    VariableUsage(clang::SourceLocation loc, unsigned line, bool read, bool write)
        : location(loc), line_number(line), is_read(read), is_write(write) {}
};

// stores info about a variable and its uses
struct VariableInfo {
    std::string name;
    clang::VarDecl* decl;
    VariableScope scope;
    VariableRole role;
    clang::SourceLocation declaration_location;
    std::vector<VariableUsage> usages;

    VariableInfo(const std::string& var_name, clang::VarDecl* var_decl,
                 VariableScope var_scope, clang::SourceLocation decl_loc)
        : name(var_name), decl(var_decl), scope(var_scope), role(VariableRole::DATA_VAR),
          declaration_location(decl_loc) {}

    void addUsage(const VariableUsage& usage) {
        usages.push_back(usage);
    }

    void setRole(VariableRole var_role) {
        role = var_role;
    }

    bool hasWrites() const {
        for (const auto& u : usages) if (u.is_write) return true;
        return false;
    }

    bool hasReads() const {
        for (const auto& u : usages) if (u.is_read) return true;
        return false;
    }

    bool isInductionVariable() const {
        return role == VariableRole::INDUCTION_VAR;
    }

    size_t getWriteCount() const {
        size_t count = 0;
        for (const auto& u : usages) if (u.is_write) count++;
        return count;
    }

    size_t getReadCount() const {
        size_t count = 0;
        for (const auto& u : usages) if (u.is_read) count++;
        return count;
    }

    bool isPotentialDependency() const {
        // only variables that are written and read and not induction vars
        return !isInductionVariable() && hasWrites() && hasReads();
    }
};

} // namespace statik