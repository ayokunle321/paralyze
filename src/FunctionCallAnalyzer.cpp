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
  
  if (verbose_) {
    std::cout << "  Analyzing function calls in loop at line " 
             << loop.line_number << "\n";
  }
  
  FunctionCallSafety safety = getFunctionCallSafety(loop);
  
  if (verbose_) {
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
}

FunctionCallSafety FunctionCallAnalyzer::getFunctionCallSafety(const LoopInfo& loop) const {
  if (loop.detected_function_calls.empty()) {
    return FunctionCallSafety::SAFE;
  }
  
  if (loop.hasUnsafeFunctionCalls()) {
    return FunctionCallSafety::UNSAFE;
  }
  
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
  
  if (verbose_) {
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
}

void FunctionCallAnalyzer::initializeSafeFunctions() {
  // math functions that don't have side effects
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
    
    // handle function pointers and member calls
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
    return "unknown_function";
  }
  
  return "";
}

bool FunctionCallAnalyzer::hasPotentialSideEffects(const std::string& function_name) const {
  // assume unknown functions have side effects
  
  if (isSafeMathFunction(function_name)) {
    return false;
  }
  
  //read-only string/char functions
  static const std::set<std::string> safe_functions = {
    "strlen", "strcmp", "strncmp", "strchr", "strstr",
    "memcmp", "isalpha", "isdigit", "isspace", "toupper", "tolower"
  };
  
  if (safe_functions.find(function_name) != safe_functions.end()) {
    return false;
  }
  
  // functions that definitely have side effects
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
  
  // default to unsafe for unknown functions
  return true;
}

} // namespace statik