#pragma once

#include "clang/AST/Decl.h"
#include "clang/Basic/SourceLocation.h"
#include <string>
#include <vector>

namespace statik {

enum class VariableScope {
  LOOP_LOCAL,    // Declared inside the loop
  FUNCTION_LOCAL, // Declared in function but outside loop
  GLOBAL         // Global or external variable
};

enum class VariableRole {
  INDUCTION_VAR,  // Loop counter (i, j, k) - safe to ignore for dependencies
  DATA_VAR,       // Regular data variable
  ARRAY_INDEX     // Used as array index
};

struct VariableUsage {
  clang::SourceLocation location;
  unsigned line_number;
  bool is_read;
  bool is_write;
  
  VariableUsage(clang::SourceLocation loc, unsigned line, bool read, bool write)
      : location(loc), line_number(line), is_read(read), is_write(write) {}
};

struct VariableInfo {
  std::string name;
  clang::VarDecl* decl;
  VariableScope scope;
  VariableRole role;
  clang::SourceLocation declaration_location;
  
  std::vector<VariableUsage> usages;  // All uses within the loop
  
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
    for (const auto& usage : usages) {
      if (usage.is_write) return true;
    }
    return false;
  }
  
  bool hasReads() const {
    for (const auto& usage : usages) {
      if (usage.is_read) return true;
    }
    return false;
  }
  
  bool isInductionVariable() const {
    return role == VariableRole::INDUCTION_VAR;
  }
  
  bool isPotentialDependency() const {
    // Induction variables don't create problematic dependencies
    return !isInductionVariable() && hasWrites() && hasReads();
  }
};

} // namespace statik