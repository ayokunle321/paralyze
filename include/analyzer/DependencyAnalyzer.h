#pragma once
#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/ArrayDependencyAnalyzer.h"
#include <string>
#include <vector>
#include <memory>

namespace statik {

enum class DependencyType {
  NO_DEPENDENCY,    // Safe to parallelize
  FLOW_DEPENDENCY,  // Read after write (RAW)
  ANTI_DEPENDENCY,  // Write after read (WAR)
  OUTPUT_DEPENDENCY, // Write after write (WAW)
  LOOP_CARRIED      // Dependency crosses iterations
};

struct Dependency {
  std::string variable_name;
  DependencyType type;
  unsigned source_line;
  unsigned sink_line;
  std::string description;
  
  Dependency(const std::string& var, DependencyType dep_type,
             unsigned src, unsigned sink, const std::string& desc)
      : variable_name(var), type(dep_type), source_line(src),
        sink_line(sink), description(desc) {}
};

class DependencyAnalyzer {
public:
  explicit DependencyAnalyzer(clang::ASTContext* context) 
      : context_(context), 
        array_analyzer_(std::make_unique<ArrayDependencyAnalyzer>(context)) {}
  
  void analyzeDependencies(LoopInfo& loop);
  bool hasDependencies(const LoopInfo& loop) const;
  
private:
  clang::ASTContext* context_;
  std::unique_ptr<ArrayDependencyAnalyzer> array_analyzer_;
  
  void analyzeScalarDependencies(LoopInfo& loop);
  void checkVariableForDependency(const std::string& var_name,
                                const VariableInfo& var_info,
                                LoopInfo& loop);
  bool isLoopCarriedDependency(const VariableUsage& write_usage,
                             const VariableUsage& read_usage) const;
};

} // namespace statik