#pragma once

#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "analyzer/ArrayAccess.h"
#include "analyzer/LoopBounds.h"
#include "analyzer/VariableInfo.h"
#include "analyzer/LoopMetrics.h"
#include <vector>
#include <map>

namespace statik {

// Forward declaration
struct Dependency;

struct LoopInfo {
  clang::Stmt* stmt;
  clang::SourceLocation location;
  unsigned line_number;
  std::string loop_type; // "for", "while", "do-while"

  // Nesting information
  unsigned depth;        // 0 = outermost, 1 = nested once, etc.
  LoopInfo* parent_loop; // nullptr if outermost
  std::vector<LoopInfo*> child_loops;

  // Array access patterns within this loop
  std::vector<ArrayAccess> array_accesses;

  // Loop bounds information
  LoopBounds bounds;
  
  // Variable usage tracking
  std::map<std::string, VariableInfo> variables;
  
  // Performance metrics
  LoopMetrics metrics;
  
  // Dependency information
  std::vector<Dependency> dependencies;
  bool has_dependencies;

  LoopInfo(clang::Stmt* s, clang::SourceLocation loc, unsigned line,
           const std::string& type)
      : stmt(s), location(loc), line_number(line), loop_type(type), depth(0),
        parent_loop(nullptr), has_dependencies(false) {}

  void addArrayAccess(const ArrayAccess& access) {
    array_accesses.push_back(access);
    metrics.memory_accesses++; // Count array access as memory operation
  }

  void setParent(LoopInfo* parent) {
    parent_loop = parent;
    if (parent) {
      depth = parent->depth + 1;
      parent->child_loops.push_back(this);
    }
  }
  
  void addVariable(const VariableInfo& var_info) {
    variables[var_info.name] = var_info;
  }
  
  void addVariableUsage(const std::string& var_name, const VariableUsage& usage) {
    auto it = variables.find(var_name);
    if (it != variables.end()) {
      it->second.addUsage(usage);
    }
  }
  
  void incrementArithmeticOps() { metrics.arithmetic_ops++; }
  void incrementFunctionCalls() { metrics.function_calls++; }
  void incrementComparisons() { metrics.comparisons++; }
  void incrementAssignments() { metrics.assignments++; }
  
  void finalizeMetrics() {
    metrics.calculateHotness();
  }
  
  void setHasDependencies(bool deps) {
    has_dependencies = deps;
  }

  bool isOutermost() const { return depth == 0; }
  bool isHot() const { return metrics.hotness_score > 10.0; }
  bool isParallelizable() const { return !has_dependencies; }
};

} // namespace statik