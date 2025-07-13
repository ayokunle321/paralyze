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

enum class ArrayDependencyType {
  NO_DEPENDENCY,        // A[i] and A[j] don't conflict
  SAME_INDEX,          // A[i] and A[i] - potential conflict if one is write
  CONSTANT_OFFSET,     // A[i] and A[i+1] - definite loop-carried dependency
  UNKNOWN_RELATION     // Can't determine relationship - assume unsafe
};

struct ArrayDependency {
  std::string array_name;
  ArrayDependencyType type;
  unsigned source_line;
  unsigned sink_line;
  std::string source_index;  // String representation of index expr
  std::string sink_index;
  
  ArrayDependency(const std::string& array, ArrayDependencyType dep_type,
                 unsigned src_line, unsigned sink_line,
                 const std::string& src_idx, const std::string& sink_idx)
      : array_name(array), type(dep_type), source_line(src_line),
        sink_line(sink_line), source_index(src_idx), sink_index(sink_idx) {}
};

class ArrayDependencyAnalyzer {
public:
  explicit ArrayDependencyAnalyzer(clang::ASTContext* context) 
      : context_(context), 
        cross_iteration_analyzer_(std::make_unique<CrossIterationAnalyzer>(context)) {}
  
  void analyzeArrayDependencies(LoopInfo& loop);
  bool hasArrayDependencies(const LoopInfo& loop) const;
  
private:
  clang::ASTContext* context_;
  std::vector<ArrayDependency> detected_dependencies_;
  std::unique_ptr<CrossIterationAnalyzer> cross_iteration_analyzer_;
  
  ArrayDependencyType compareArrayIndices(clang::Expr* index1, clang::Expr* index2,
                                        const std::string& induction_var);
  std::string exprToString(clang::Expr* expr);
  bool isSimpleInductionAccess(clang::Expr* index, const std::string& induction_var);
  bool hasConstantOffset(clang::Expr* index1, clang::Expr* index2);
  void checkArrayAccessPair(const ArrayAccess& access1, const ArrayAccess& access2,
                          const std::string& induction_var);
};

} // namespace statik