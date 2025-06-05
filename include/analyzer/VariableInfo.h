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
  clang::SourceLocation declaration_location;
  
  std::vector<VariableUsage> usages;  // All uses within the loop
  
  VariableInfo(const std::string& var_name, clang::VarDecl* var_decl, 
               VariableScope var_scope, clang::SourceLocation decl_loc)
      : name(var_name), decl(var_decl), scope(var_scope), 
        declaration_location(decl_loc) {}
        
  void addUsage(const VariableUsage& usage) {
    usages.push_back(usage);
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
};

} // namespace statik