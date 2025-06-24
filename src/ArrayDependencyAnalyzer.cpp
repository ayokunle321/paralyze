#include "analyzer/ArrayDependencyAnalyzer.h"
#include "clang/AST/Expr.h"
#include <iostream>

using namespace clang;

namespace statik {

void ArrayDependencyAnalyzer::analyzeArrayDependencies(LoopInfo& loop) {
  detected_dependencies_.clear();
  
  std::cout << "  Analyzing array dependencies for " 
           << loop.array_accesses.size() << " array accesses\n";
  
  // Compare every pair of array accesses
  for (size_t i = 0; i < loop.array_accesses.size(); i++) {
    for (size_t j = i + 1; j < loop.array_accesses.size(); j++) {
      const ArrayAccess& access1 = loop.array_accesses[i];
      const ArrayAccess& access2 = loop.array_accesses[j];
      
      // Only check accesses to the same array
      if (access1.array_name == access2.array_name) {
        checkArrayAccessPair(access1, access2, loop.bounds.iterator_var);
      }
    }
  }
  
  std::cout << "  Found " << detected_dependencies_.size() 
           << " potential array dependencies\n";
}

bool ArrayDependencyAnalyzer::hasArrayDependencies(const LoopInfo& loop) const {
  // Any detected dependency (except NO_DEPENDENCY) is problematic
  for (const auto& dep : detected_dependencies_) {
    if (dep.type != ArrayDependencyType::NO_DEPENDENCY) {
      return true;
    }
  }
  return false;
}

void ArrayDependencyAnalyzer::checkArrayAccessPair(const ArrayAccess& access1, 
                                                   const ArrayAccess& access2,
                                                   const std::string& induction_var) {
  // Skip if both are just reads - no dependency
  if (!access1.is_write && !access2.is_write) {
    return;
  }
  
  ArrayDependencyType dep_type = compareArrayIndices(access1.subscript, 
                                                   access2.subscript, 
                                                   induction_var);
  
  if (dep_type != ArrayDependencyType::NO_DEPENDENCY) {
    std::string idx1_str = exprToString(access1.subscript);
    std::string idx2_str = exprToString(access2.subscript);
    
    ArrayDependency dependency(access1.array_name, dep_type,
                             access1.line_number, access2.line_number,
                             idx1_str, idx2_str);
    
    detected_dependencies_.push_back(dependency);
    
    std::cout << "  Array dependency: " << access1.array_name 
             << "[" << idx1_str << "] vs [" << idx2_str << "] - ";
    
    switch (dep_type) {
      case ArrayDependencyType::SAME_INDEX:
        std::cout << "SAME INDEX (write conflict)";
        break;
      case ArrayDependencyType::CONSTANT_OFFSET:
        std::cout << "CONSTANT OFFSET (loop-carried)";
        break;
      case ArrayDependencyType::UNKNOWN_RELATION:
        std::cout << "UNKNOWN (assume unsafe)";
        break;
      default:
        break;
    }
    std::cout << "\n";
  }
}

ArrayDependencyType ArrayDependencyAnalyzer::compareArrayIndices(Expr* index1, 
                                                                Expr* index2,
                                                                const std::string& induction_var) {
  if (!index1 || !index2) {
    return ArrayDependencyType::UNKNOWN_RELATION;
  }
  
  // Add safety checks to prevent crashes on malformed expressions
  try {
    std::string idx1_str = exprToString(index1);
    std::string idx2_str = exprToString(index2);
    
    // If expression parsing failed, be conservative
    if (idx1_str == "error_expr" || idx2_str == "error_expr") {
      return ArrayDependencyType::UNKNOWN_RELATION;
    }
    
    // Check if both are simple induction variable accesses (A[i] and A[i])
    if (isSimpleInductionAccess(index1, induction_var) && 
        isSimpleInductionAccess(index2, induction_var)) {
      return ArrayDependencyType::SAME_INDEX;
    }
    
    // Check for constant offset patterns like A[i] vs A[i+1]
    if (hasConstantOffset(index1, index2)) {
      return ArrayDependencyType::CONSTANT_OFFSET;
    }
    
    // If indices look different but we can't prove they don't conflict
    if (idx1_str != idx2_str) {
      return ArrayDependencyType::UNKNOWN_RELATION;
    }
    
    return ArrayDependencyType::NO_DEPENDENCY;
  } catch (...) {
    // Conservative fallback - assume unsafe if analysis fails
    return ArrayDependencyType::UNKNOWN_RELATION;
  }
}

bool ArrayDependencyAnalyzer::isSimpleInductionAccess(Expr* index, 
                                                     const std::string& induction_var) {
  if (!index || induction_var.empty()) {
    return false;
  }
  
  // Look for simple DeclRefExpr that matches induction variable
  index = index->IgnoreParenImpCasts();
  if (auto* declRef = dyn_cast<DeclRefExpr>(index)) {
    return declRef->getDecl()->getNameAsString() == induction_var;
  }
  
  return false;
}

bool ArrayDependencyAnalyzer::hasConstantOffset(Expr* index1, Expr* index2) {
  if (!index1 || !index2) {
    return false;
  }
  
  // This is a simplified check - real implementation would need more sophisticated
  // expression analysis to detect patterns like i vs i+1, i vs i-1, etc.
  std::string str1 = exprToString(index1);
  std::string str2 = exprToString(index2);
  
  // Look for obvious patterns like "i" vs "i + 1" or "i" vs "i - 1"
  if ((str1.find("+") != std::string::npos || str1.find("-") != std::string::npos) &&
      (str2.find("+") != std::string::npos || str2.find("-") != std::string::npos)) {
    return true;  // Conservative: assume any arithmetic means potential offset
  }
  
  return false;
}

std::string ArrayDependencyAnalyzer::exprToString(Expr* expr) {
  if (!expr) {
    return "null";
  }
  
  // Add try-catch equivalent behavior for robustness
  try {
    expr = expr->IgnoreParenImpCasts();
    
    if (auto* declRef = dyn_cast<DeclRefExpr>(expr)) {
      if (declRef->getDecl()) {
        return declRef->getDecl()->getNameAsString();
      }
      return "unknown_var";
    }
    
    if (auto* binOp = dyn_cast<BinaryOperator>(expr)) {
      std::string lhs = exprToString(binOp->getLHS());
      std::string rhs = exprToString(binOp->getRHS());
      
      // Prevent infinite recursion in complex expressions
      if (lhs == "complex_expr" || rhs == "complex_expr") {
        return "complex_expr";
      }
      
      std::string op;
      switch (binOp->getOpcode()) {
        case BO_Add: op = " + "; break;
        case BO_Sub: op = " - "; break;
        case BO_Mul: op = " * "; break;
        case BO_Div: op = " / "; break;
        default: op = " ? "; break;
      }
      
      return lhs + op + rhs;
    }
    
    if (auto* intLit = dyn_cast<IntegerLiteral>(expr)) {
      return std::to_string(intLit->getValue().getSExtValue());
    }
    
    // Handle more expression types safely
    if (auto* unaryOp = dyn_cast<UnaryOperator>(expr)) {
      std::string sub = exprToString(unaryOp->getSubExpr());
      switch (unaryOp->getOpcode()) {
        case UO_Minus: return "-" + sub;
        case UO_Plus: return "+" + sub;
        default: return "unary_op(" + sub + ")";
      }
    }
    
    return "complex_expr";
  } catch (...) {
    // Fallback for any unexpected issues
    return "error_expr";
  }
}

} // namespace statik