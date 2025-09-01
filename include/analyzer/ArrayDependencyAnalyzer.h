#pragma once
#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"
#include "analyzer/ArrayAccess.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/CrossIterationAnalyzer.h"
#include <string>
#include <vector>
#include <memory>

namespace statik {

// kinds of dependencies between array accesses
enum class ArrayDependencyType {
  NO_DEPENDENCY,
  SAME_INDEX,          // e.g. A[i] vs A[i] - write conflict
  CONSTANT_OFFSET,     // e.g. A[i] vs A[i+1] - loop-carried
  UNKNOWN_RELATION     // assume unsafe
};

// record of a single dependency found between two accesses
struct ArrayDependency {
  std::string array_name;
  ArrayDependencyType type;
  unsigned source_line;
  unsigned sink_line;
  std::string source_index;
  std::string sink_index;
  
  ArrayDependency(const std::string& array, ArrayDependencyType dep_type,
                 unsigned src_line, unsigned sink_line,
                 const std::string& src_idx, const std::string& sink_idx)
      : array_name(array), type(dep_type), source_line(src_line),
        sink_line(sink_line), source_index(src_idx), sink_index(sink_idx) {}
};

// main analyzer for checking array dependencies inside a loop
class ArrayDependencyAnalyzer {
public:
  explicit ArrayDependencyAnalyzer(clang::ASTContext* context) 
      : context_(context), verbose_(false),
        cross_iteration_analyzer_(std::make_unique<CrossIterationAnalyzer>(context)) {}
  
  void analyzeArrayDependencies(LoopInfo& loop);
  bool hasArrayDependencies(const LoopInfo& loop) const;
  
  void setVerbose(bool verbose) { verbose_ = verbose; }
  
private:
  clang::ASTContext* context_;  
  bool verbose_;
  std::vector<ArrayDependency> detected_dependencies_;
  std::unique_ptr<CrossIterationAnalyzer> cross_iteration_analyzer_;
  
  // helpers for comparing access patterns
  ArrayDependencyType compareArrayIndices(clang::Expr* index1, clang::Expr* index2,
                                        const std::string& induction_var);
  std::string exprToString(clang::Expr* expr);
  bool isSimpleInductionAccess(clang::Expr* index, const std::string& induction_var);
  bool hasConstantOffset(clang::Expr* index1, clang::Expr* index2);
  void checkArrayAccessPair(const ArrayAccess& access1, const ArrayAccess& access2,
                          const std::string& induction_var);
};

} // namespace statik
