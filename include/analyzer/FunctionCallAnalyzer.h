#pragma once

#include "analyzer/LoopInfo.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include <set>
#include <string>
#include <vector>

namespace paralyze
{

// categorizes safety of function calls inside a loop
enum class FunctionCallSafety
{
  SAFE,             // no calls or only safe built-ins
  POTENTIALLY_SAFE, // known safe functions (math)
  UNSAFE            // unknown or side-effect functions
};

// record of a single function call in the source
struct FunctionCall
{
  std::string function_name;
  clang::SourceLocation location;
  unsigned line_number;
  bool is_builtin;
  bool is_math_function;
  bool has_side_effects; // conservative assumption

  FunctionCall(const std::string& name, clang::SourceLocation loc, unsigned line, bool builtin,
               bool math, bool side_effects)
      : function_name(name), location(loc), line_number(line), is_builtin(builtin),
        is_math_function(math), has_side_effects(side_effects)
  {
  }
};

// analyzes function calls in loops to check safety for parallelization
class FunctionCallAnalyzer
{
public:
  explicit FunctionCallAnalyzer(clang::ASTContext* context) : context_(context) {}

  void analyzeFunctionCalls(LoopInfo& loop);
  FunctionCallSafety getFunctionCallSafety(const LoopInfo& loop) const;
  void visitCallExpr(clang::CallExpr* callExpr, LoopInfo& loop);
  void setVerbose(bool verbose) { verbose_ = verbose; }

private:
  clang::ASTContext* context_;
  std::vector<FunctionCall> function_calls_;
  std::set<std::string> safe_math_functions_;
  bool verbose_ = false;

  void initializeSafeFunctions();
  bool isSafeMathFunction(const std::string& name) const;
  bool isBuiltinFunction(clang::CallExpr* callExpr) const;
  std::string getFunctionName(clang::CallExpr* callExpr) const;
  bool hasPotentialSideEffects(const std::string& function_name) const;
};

} // namespace paralyze
