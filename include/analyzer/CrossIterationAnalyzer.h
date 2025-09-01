#pragma once

#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"
#include "analyzer/ArrayAccess.h"
#include "analyzer/LoopInfo.h"
#include <string>
#include <vector>
#include <map>

namespace statik {

// kinds of conflicts between array accesses across iterations
enum class IterationConflictType {
  NO_CONFLICT,
  WRITE_AFTER_READ,
  READ_AFTER_WRITE,
  WRITE_AFTER_WRITE,
  STRIDE_CONFLICT       // non-unit stride or complex indexing
};

// record of a single cross-iteration conflict
struct CrossIterationConflict {
  std::string array_name;
  IterationConflictType type;
  std::string index_pattern;  
  unsigned source_line;
  unsigned sink_line;
  std::string description;
  
  CrossIterationConflict(const std::string& array, IterationConflictType conflict_type,
                        const std::string& pattern, unsigned src_line, unsigned sink_line,
                        const std::string& desc)
      : array_name(array), type(conflict_type), index_pattern(pattern),
        source_line(src_line), sink_line(sink_line), description(desc) {}
};

// analyzes loops for cross-iteration array conflicts
class CrossIterationAnalyzer {
public:
  explicit CrossIterationAnalyzer(clang::ASTContext* context) : context_(context) {}
  
  void analyzeCrossIterationConflicts(LoopInfo& loop);
  bool hasCrossIterationConflicts(const LoopInfo& loop) const;
  
  void setVerbose(bool verbose) { verbose_ = verbose; }
  
private:
  clang::ASTContext* context_;
  std::vector<CrossIterationConflict> conflicts_;
  bool verbose_ = false;
  
  void analyzeArrayAccessPattern(const std::string& array_name, 
                               const std::vector<ArrayAccess>& accesses,
                               const std::string& induction_var);
  
  bool detectsStridePattern(clang::Expr* index, const std::string& induction_var, int& stride);
  bool hasOffsetFromInduction(clang::Expr* index, const std::string& induction_var, int& offset);
  IterationConflictType classifyConflict(const ArrayAccess& access1, const ArrayAccess& access2,
                                       int offset1, int offset2, int stride);
  
  std::string describeConflict(IterationConflictType type, const std::string& array_name,
                             const std::string& pattern);
};

} // namespace statik
