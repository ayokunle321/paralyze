#include "analyzer/PointerAnalyzer.h"
#include "clang/AST/Type.h"
#include <iostream>

using namespace clang;

namespace statik {

void PointerAnalyzer::analyzePointerUsage(LoopInfo& loop) {
  pointer_ops_.clear();
  detected_pointers_.clear();
  
  std::cout << "  Analyzing pointer usage in loop at line " 
           << loop.line_number << "\n";
  
  // The actual pointer detection happens through the visitor methods
  // called from LoopVisitor and this method summarizes the results
  
  PointerRisk risk = getPointerRisk(loop);
  
  switch (risk) {
    case PointerRisk::SAFE:
      std::cout << "  No pointer operations detected\n";
      break;
    case PointerRisk::POTENTIAL_ALIAS:
      std::cout << "  Pointer operations detected - potential aliasing risk\n";
      break;
    case PointerRisk::UNSAFE:
      std::cout << "  Complex pointer operations - unsafe for parallelization\n";
      break;
  }
}

PointerRisk PointerAnalyzer::getPointerRisk(const LoopInfo& loop) const {
  if (pointer_ops_.empty()) {
    return PointerRisk::SAFE;
  }
  
  // Check for complex patterns that are definitely unsafe
  if (hasComplexPointerArithmetic() || hasMultiplePointerDereferences()) {
    return PointerRisk::UNSAFE;
  }
  
  // Any pointer dereferences are potentially risky due to aliasing
  for (const auto& op : pointer_ops_) {
    if (op.is_dereference) {
      return PointerRisk::POTENTIAL_ALIAS;
    }
  }
  
  return PointerRisk::SAFE;
}

void PointerAnalyzer::visitUnaryOperator(UnaryOperator* unaryOp, LoopInfo& loop) {
  if (!unaryOp) {
    return;
  }
  
  SourceLocation loc = unaryOp->getOperatorLoc();
  SourceManager& sm = context_->getSourceManager();
  unsigned line = sm.getSpellingLineNumber(loc);
  
  switch (unaryOp->getOpcode()) {
    case UO_Deref: { // *ptr
      std::string ptrName = extractPointerName(unaryOp->getSubExpr());
      if (!ptrName.empty()) {
        recordPointerOperation(ptrName, loc, true, false, false);
        std::cout << "  Pointer dereference: *" << ptrName 
                 << " at line " << line << "\n";
      }
      break;
    }
    case UO_AddrOf: { // &var
      std::string varName = extractPointerName(unaryOp->getSubExpr());
      if (!varName.empty()) {
        recordPointerOperation(varName, loc, false, true, false);
        std::cout << "  Address-of operation: &" << varName 
                 << " at line " << line << "\n";
      }
      break;
    }
    case UO_PreInc:
    case UO_PostInc:
    case UO_PreDec:
    case UO_PostDec: {
      // Check if incrementing/decrementing a pointer
      if (isPointerType(unaryOp->getSubExpr()->getType())) {
        std::string ptrName = extractPointerName(unaryOp->getSubExpr());
        if (!ptrName.empty()) {
          recordPointerOperation(ptrName, loc, false, false, true);
          std::cout << "  Pointer arithmetic: " << ptrName 
                   << "++ at line " << line << "\n";
        }
      }
      break;
    }
    default:
      break;
  }
}

void PointerAnalyzer::visitBinaryOperator(BinaryOperator* binOp, LoopInfo& loop) {
  if (!binOp) {
    return;
  }
  
  // Check for pointer arithmetic: ptr + offset, ptr - offset
  Expr* lhs = binOp->getLHS();
  Expr* rhs = binOp->getRHS();
  
  if (binOp->getOpcode() == BO_Add || binOp->getOpcode() == BO_Sub) {
    // Check if left operand is a pointer
    if (isPointerType(lhs->getType())) {
      std::string ptrName = extractPointerName(lhs);
      if (!ptrName.empty()) {
        SourceLocation loc = binOp->getOperatorLoc();
        recordPointerOperation(ptrName, loc, false, false, true);
        
        SourceManager& sm = context_->getSourceManager();
        unsigned line = sm.getSpellingLineNumber(loc);
        std::cout << "  Pointer arithmetic: " << ptrName 
                 << " +/- offset at line " << line << "\n";
      }
    }
  }
  
  // Check for pointer assignments that might create aliasing
  if (binOp->isAssignmentOp()) {
    if (isPointerType(lhs->getType()) && isPointerType(rhs->getType())) {
      std::string lhsName = extractPointerName(lhs);
      std::string rhsName = extractPointerName(rhs);
      
      if (!lhsName.empty() && !rhsName.empty()) {
        SourceLocation loc = binOp->getOperatorLoc();
        SourceManager& sm = context_->getSourceManager();
        unsigned line = sm.getSpellingLineNumber(loc);
        
        std::cout << "  Pointer assignment: " << lhsName << " = " << rhsName 
                 << " at line " << line << " (potential aliasing)\n";
      }
    }
  }
}

void PointerAnalyzer::visitMemberExpr(MemberExpr* memberExpr, LoopInfo& loop) {
  if (!memberExpr) {
    return;
  }
  
  // Check for struct/class member access through pointers
  if (memberExpr->isArrow()) { // ptr->member
    std::string ptrName = extractPointerName(memberExpr->getBase());
    if (!ptrName.empty()) {
      SourceLocation loc = memberExpr->getMemberLoc();
      recordPointerOperation(ptrName, loc, true, false, false);
      
      SourceManager& sm = context_->getSourceManager();
      unsigned line = sm.getSpellingLineNumber(loc);
      std::cout << "  Pointer member access: " << ptrName 
               << "->member at line " << line << "\n";
    }
  }
}

bool PointerAnalyzer::isPointerType(QualType type) {
  return type->isPointerType() || type->isArrayType();
}

std::string PointerAnalyzer::extractPointerName(Expr* expr) {
  if (!expr) {
    return "";
  }
  
  // Use instance method with recursion limit to prevent infinite loops
  return extractPointerNameRecursive(expr, 0);
}

std::string PointerAnalyzer::extractPointerNameRecursive(Expr* expr, int depth) {
  // Prevent infinite recursion
  if (depth > 10) {
    return "complex_expr";
  }
  
  if (!expr) {
    return "";
  }
  
  expr = expr->IgnoreParenImpCasts();
  
  if (auto* declRef = dyn_cast<DeclRefExpr>(expr)) {
    return declRef->getDecl()->getNameAsString();
  } else if (auto* arrayExpr = dyn_cast<ArraySubscriptExpr>(expr)) {
    return extractPointerNameRecursive(arrayExpr->getBase(), depth + 1);
  } else if (auto* binOp = dyn_cast<BinaryOperator>(expr)) {
    // Handle pointer arithmetic like ptr + offset
    if (binOp->getOpcode() == BO_Add || binOp->getOpcode() == BO_Sub) {
      std::string lhs = extractPointerNameRecursive(binOp->getLHS(), depth + 1);
      if (!lhs.empty() && lhs != "complex_expr") {
        return lhs + "_offset";
      } else {
        return "complex_expr";
      }
    } else {
      return "complex_expr";
    }
  } else if (auto* unaryOp = dyn_cast<UnaryOperator>(expr)) {
    // Handle unary operations like *ptr, &var
    return extractPointerNameRecursive(unaryOp->getSubExpr(), depth + 1);
  } else {
    return "complex_expr";
  }
}

void PointerAnalyzer::recordPointerOperation(const std::string& name, 
                                           SourceLocation loc,
                                           bool deref, bool addr, bool arith) {
  SourceManager& sm = context_->getSourceManager();
  unsigned line = sm.getSpellingLineNumber(loc);
  
  pointer_ops_.emplace_back(name, loc, line, deref, addr, arith);
  detected_pointers_.insert(name);
}

bool PointerAnalyzer::hasComplexPointerArithmetic() const {
  int arithmetic_ops = 0;
  for (const auto& op : pointer_ops_) {
    if (op.is_arithmetic) {
      arithmetic_ops++;
    }
  }
  
  // More than 2 pointer arithmetic operations is getting complex
  return arithmetic_ops > 2;
}

bool PointerAnalyzer::hasMultiplePointerDereferences() const {
  int dereference_count = 0;
  for (const auto& op : pointer_ops_) {
    if (op.is_dereference) {
      dereference_count++;
    }
  }
  
  // Multiple pointer dereferences suggest complex data structures
  return dereference_count > 3;
}

} // namespace statik