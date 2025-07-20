#include "analyzer/FunctionCallAnalyzer.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/SourceManager.h" 
#include <iostream>
#include <algorithm>

using namespace clang;

namespace statik {

void FunctionCallAnalyzer::analyzeFunctionCalls(LoopInfo& loop) {
  function_calls_.clear();
  initializeSafeFunctions();
  
  std::cout << "  Analyzing function calls in loop at line " 
           << loop.line_number << "\n";
  
  // Function call detection happens through visitCallExpr
  // This method summarizes the safety assessment
  
  FunctionCallSafety safety = getFunctionCallSafety(loop);
  
  switch (safety) {
    case FunctionCallSafety::SAFE:
      std::cout << "  No problematic function calls detected\n";
      break;
    case FunctionCallSafety::POTENTIALLY_SAFE:
      std::cout << "  Safe function calls detected (math functions only)\n";
      break;
    case FunctionCallSafety::UNSAFE:
      std::cout << "  Unsafe function calls detected - not parallelizable\n";
      break;
  }
}

FunctionCallSafety FunctionCallAnalyzer::getFunctionCallSafety(const LoopInfo& loop) const {
  // Check the stored function call info in LoopInfo
  if (loop.detected_function_calls.empty()) {
    return FunctionCallSafety::SAFE;
  }
  
  // Check the stored safety flags
  if (loop.hasUnsafeFunctionCalls()) {
    return FunctionCallSafety::UNSAFE;
  }
  
  // If we have function calls but they're all safe, check if they're math functions
  bool has_potentially_safe_calls = !loop.detected_function_calls.empty();
  
  if (has_potentially_safe_calls) {
    return FunctionCallSafety::POTENTIALLY_SAFE;
  }
  
  return FunctionCallSafety::SAFE;
}

void FunctionCallAnalyzer::visitCallExpr(CallExpr* callExpr, LoopInfo& loop) {
  if (!callExpr) {
    return;
  }
  
  std::string funcName = getFunctionName(callExpr);
  if (funcName.empty()) {
    funcName = "unknown_function";
  }
  
  SourceLocation loc = callExpr->getBeginLoc();
  SourceManager& sm = context_->getSourceManager();
  unsigned line = sm.getSpellingLineNumber(loc);
  
  bool is_builtin = isBuiltinFunction(callExpr);
  bool is_math = isSafeMathFunction(funcName);
  bool has_side_effects = hasPotentialSideEffects(funcName);
  
  function_calls_.emplace_back(funcName, loc, line, is_builtin, is_math, has_side_effects);
  
  std::cout << "  Function call: " << funcName << " at line " << line;
  
  if (is_builtin) {
    std::cout << " (builtin)";
  } else if (is_math) {
    std::cout << " (math function - potentially safe)";
  } else if (has_side_effects) {
    std::cout << " (UNSAFE - potential side effects)";
  }
  
  std::cout << "\n";
}

void FunctionCallAnalyzer::initializeSafeFunctions() {
  // Common math functions that are generally safe for parallelization
  safe_math_functions_ = {
    "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
    "exp", "exp2", "expm1", "log", "log10", "log2", "log1p",
    "sqrt", "cbrt", "pow", "hypot",
    "ceil", "floor", "trunc", "round", "nearbyint", "rint",
    "fabs", "abs", "fmod", "remainder", "remquo",
    "fmin", "fmax", "fdim", "fma",
    "isfinite", "isinf", "isnan", "isnormal", "signbit"
  };
}

bool FunctionCallAnalyzer::isSafeMathFunction(const std::string& name) const {
  return safe_math_functions_.find(name) != safe_math_functions_.end();
}

bool FunctionCallAnalyzer::isBuiltinFunction(CallExpr* callExpr) const {
  if (auto* funcDecl = callExpr->getDirectCallee()) {
    return funcDecl->getBuiltinID() != 0;
  }
  return false;
}

std::string FunctionCallAnalyzer::getFunctionName(CallExpr* callExpr) const {
  if (!callExpr) {
    return "";
  }
  
  try {
    if (auto* funcDecl = callExpr->getDirectCallee()) {
      if (funcDecl->getDeclName().isIdentifier()) {
        return funcDecl->getNameAsString();
      }
    }
    
    // Try to get name from function pointer or member call
    if (Expr* callee = callExpr->getCallee()) {
      callee = callee->IgnoreParenImpCasts();
      
      if (auto* declRef = dyn_cast<DeclRefExpr>(callee)) {
        if (declRef->getDecl()) {
          return declRef->getDecl()->getNameAsString();
        }
      }
      
      if (auto* memberExpr = dyn_cast<MemberExpr>(callee)) {
        if (memberExpr->getMemberDecl()) {
          return memberExpr->getMemberDecl()->getNameAsString();
        }
      }
    }
  } catch (...) {
    // Return safe default if anything goes wrong
    return "unknown_function";
  }
  
  return "";
}

bool FunctionCallAnalyzer::hasPotentialSideEffects(const std::string& function_name) const {
  // Conservative approach: assume unknown functions have side effects
  
  // Known safe functions
  if (isSafeMathFunction(function_name)) {
    return false;
  }
  
  // Functions that are generally safe (read-only operations)
  static const std::set<std::string> safe_functions = {
    "strlen", "strcmp", "strncmp", "strchr", "strstr",
    "memcmp", "isalpha", "isdigit", "isspace", "toupper", "tolower"
  };
  
  if (safe_functions.find(function_name) != safe_functions.end()) {
    return false;
  }
  
  // Functions that definitely have side effects
  static const std::set<std::string> unsafe_functions = {
    "printf", "fprintf", "sprintf", "puts", "putchar",
    "scanf", "fscanf", "sscanf", "getchar", "gets", "fgets",
    "malloc", "calloc", "realloc", "free",
    "fopen", "fclose", "fread", "fwrite", "fseek", "ftell",
    "exit", "abort", "system", "rand", "srand", "time"
  };
  
  if (unsafe_functions.find(function_name) != unsafe_functions.end()) {
    return true;
  }
  
  // Conservative default: assume unknown functions have side effects
  return true;
}

} // namespace statik