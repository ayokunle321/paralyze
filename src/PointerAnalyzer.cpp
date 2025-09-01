#include "analyzer/PointerAnalyzer.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"
#include <iostream>

using namespace clang;

namespace statik {

void PointerAnalyzer::analyzePointerUsage(LoopInfo& loop) {
  pointer_ops_.clear();
  detected_pointers_.clear();
  
  if (verbose_) {
    std::cout << "  Analyzing pointer usage in loop at line " 
             << loop.line_number << "\n";
  }
  
  PointerRisk risk = getPointerRisk(loop);
  
  if (verbose_) {
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
}

PointerRisk PointerAnalyzer::getPointerRisk(const LoopInfo& loop) const {
  if (pointer_ops_.empty()) {
    return PointerRisk::SAFE;
  }
  
  if (hasComplexPointerArithmetic() || hasMultiplePointerDereferences()) {
    return PointerRisk::UNSAFE;
  }
  
  // any dereferencing is risky due to potential aliasing
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
    case UO_Deref: {
      std::string ptrName = extractPointerName(unaryOp->getSubExpr());
      if (!ptrName.empty()) {
        recordPointerOperation(ptrName, loc, true, false, false);
        if (verbose_) {
          std::cout << "  Pointer dereference: *" << ptrName 
                   << " at line " << line << "\n";
        }
      }
      break;
    }
    case UO_AddrOf: {
      std::string varName = extractPointerName(unaryOp->getSubExpr());
      if (!varName.empty()) {
        recordPointerOperation(varName, loc, false, true, false);
        if (verbose_) {
          std::cout << "  Address-of operation: &" << varName 
                   << " at line " << line << "\n";
        }
      }
      break;
    }
    case UO_PreInc:
    case UO_PostInc:
    case UO_PreDec:
    case UO_PostDec: {
      if (isPointerType(unaryOp->getSubExpr()->getType())) {
        std::string ptrName = extractPointerName(unaryOp->getSubExpr());
        if (!ptrName.empty()) {
          recordPointerOperation(ptrName, loc, false, false, true);
          if (verbose_) {
            std::cout << "  Pointer arithmetic: " << ptrName 
                     << "++ at line " << line << "\n";
          }
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
  
  Expr* lhs = binOp->getLHS();
  Expr* rhs = binOp->getRHS();
  
  if (binOp->getOpcode() == BO_Add || binOp->getOpcode() == BO_Sub) {
    if (isPointerType(lhs->getType())) {
      std::string ptrName = extractPointerName(lhs);
      if (!ptrName.empty()) {
        SourceLocation loc = binOp->getOperatorLoc();
        recordPointerOperation(ptrName, loc, false, false, true);
        
        if (verbose_) {
          SourceManager& sm = context_->getSourceManager();
          unsigned line = sm.getSpellingLineNumber(loc);
          std::cout << "  Pointer arithmetic: " << ptrName 
                   << " +/- offset at line " << line << "\n";
        }
      }
    }
  }
  
  // check for pointer assignments that create aliasing
  if (binOp->isAssignmentOp()) {
    if (isPointerType(lhs->getType()) && isPointerType(rhs->getType())) {
      std::string lhsName = extractPointerName(lhs);
      std::string rhsName = extractPointerName(rhs);
      
      if (!lhsName.empty() && !rhsName.empty()) {
        if (verbose_) {
          SourceLocation loc = binOp->getOperatorLoc();
          SourceManager& sm = context_->getSourceManager();
          unsigned line = sm.getSpellingLineNumber(loc);
          
          std::cout << "  Pointer assignment: " << lhsName << " = " << rhsName 
                   << " at line " << line << " (potential aliasing)\n";
        }
      }
    }
  }
}

void PointerAnalyzer::visitMemberExpr(MemberExpr* memberExpr, LoopInfo& loop) {
  if (!memberExpr) {
    return;
  }
  
  if (memberExpr->isArrow()) {
    std::string ptrName = extractPointerName(memberExpr->getBase());
    if (!ptrName.empty()) {
      SourceLocation loc = memberExpr->getMemberLoc();
      recordPointerOperation(ptrName, loc, true, false, false);
      
      if (verbose_) {
        SourceManager& sm = context_->getSourceManager();
        unsigned line = sm.getSpellingLineNumber(loc);
        std::cout << "  Pointer member access: " << ptrName 
                 << "->member at line " << line << "\n";
      }
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
  
  return extractPointerNameRecursive(expr, 0);
}

std::string PointerAnalyzer::extractPointerNameRecursive(Expr* expr, int depth) {
  // prevent infinite recursion
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
  
  return arithmetic_ops > 2;
}

bool PointerAnalyzer::hasMultiplePointerDereferences() const {
  int dereference_count = 0;
  for (const auto& op : pointer_ops_) {
    if (op.is_dereference) {
      dereference_count++;
    }
  }
  
  return dereference_count > 3;
}

} // namespace statik