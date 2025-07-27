#pragma once

#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include <string>
#include <vector>
#include <set>

namespace statik {

enum class FunctionCallSafety {
  SAFE,              // No function calls or only safe built-ins
  POTENTIALLY_SAFE,  // Calls to known safe functions (math functions, etc)
  UNSAFE             // Unknown functions or functions with side effects
};

struct FunctionCall {
  std::string function_name;
  clang::SourceLocation location;
  unsigned line_number;
  bool is_builtin;
  bool is_math_function;
  bool has_side_effects;   // Conservative assumption
  
  FunctionCall(const std::string& name, clang::SourceLocation loc, 
              unsigned line, bool builtin, bool math, bool side_effects)
      : function_name(name), location(loc), line_number(line),
        is_builtin(builtin), is_math_function(math), has_side_effects(side_effects) {}
};

class FunctionCallAnalyzer {
public:
  explicit FunctionCallAnalyzer(clang::ASTContext* context) : context_(context) {}
  
  void analyzeFunctionCalls(LoopInfo& loop);
  FunctionCallSafety getFunctionCallSafety(const LoopInfo& loop) const;
  
  void visitCallExpr(clang::CallExpr* callExpr, LoopInfo& loop);
  
  // Verbose control
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

} // namespace statik