#include "analyzer/LoopVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include <iostream>

using namespace clang;

namespace statik {

bool LoopVisitor::VisitForStmt(ForStmt* forLoop) {
  if (!forLoop) {
    return true;
  }

  SourceLocation loc = forLoop->getForLoc();
  addLoop(forLoop, loc, "for");

  // Get the newly added loop and set up parent relationship
  LoopInfo* newLoop = &loops_.back();
  if (!loop_stack_.empty()) {
    newLoop->setParent(loop_stack_.top());
  }

  // Analyze loop bounds and identify induction variable
  analyzeForLoopBounds(forLoop, *newLoop);

  // Push this loop onto stack and traverse body
  loop_stack_.push(newLoop);

  if (forLoop->getBody()) {
    TraverseStmt(forLoop->getBody());
  }

  // After traversing, mark the induction variable
  markInductionVariable(*newLoop);

  // Run dependency analysis
  finalizeDependencyAnalysis(*newLoop);

  // Finalize metrics before popping
  newLoop->finalizeMetrics();
  loop_stack_.pop();
  return true;
}

bool LoopVisitor::VisitWhileStmt(WhileStmt* whileLoop) {
  if (!whileLoop) {
    return true;
  }

  SourceLocation loc = whileLoop->getWhileLoc();
  addLoop(whileLoop, loc, "while");

  LoopInfo* newLoop = &loops_.back();
  if (!loop_stack_.empty()) {
    newLoop->setParent(loop_stack_.top());
  }

  loop_stack_.push(newLoop);

  if (whileLoop->getBody()) {
    TraverseStmt(whileLoop->getBody());
  }

  markInductionVariable(*newLoop);
  finalizeDependencyAnalysis(*newLoop);
  newLoop->finalizeMetrics();
  loop_stack_.pop();
  return true;
}

bool LoopVisitor::VisitDoStmt(DoStmt* doLoop) {
  if (!doLoop) {
    return true;
  }

  SourceLocation loc = doLoop->getDoLoc();
  addLoop(doLoop, loc, "do-while");

  LoopInfo* newLoop = &loops_.back();
  if (!loop_stack_.empty()) {
    newLoop->setParent(loop_stack_.top());
  }

  loop_stack_.push(newLoop);

  if (doLoop->getBody()) {
    TraverseStmt(doLoop->getBody());
  }

  markInductionVariable(*newLoop);
  finalizeDependencyAnalysis(*newLoop);
  newLoop->finalizeMetrics();
  loop_stack_.pop();
  return true;
}

bool LoopVisitor::VisitVarDecl(VarDecl* varDecl) {
  if (!varDecl || !isInsideLoop()) {
    return true;
  }

  const std::string varName = varDecl->getNameAsString();
  VariableScope scope = determineVariableScope(varDecl);
  SourceLocation loc = varDecl->getLocation();

  VariableInfo varInfo(varName, varDecl, scope, loc);
  getCurrentLoop()->addVariable(varInfo);

  std::cout << "  Found variable declaration: " << varName 
            << " (scope: " << (scope == VariableScope::LOOP_LOCAL ? "local" : "external")
            << ")\n";

  return true;
}

bool LoopVisitor::VisitDeclRefExpr(DeclRefExpr* declRef) {
  if (!declRef || !isInsideLoop()) {
    return true;
  }

  if (auto* varDecl = dyn_cast<VarDecl>(declRef->getDecl())) {
    const std::string varName = varDecl->getNameAsString();
    SourceLocation loc = declRef->getLocation();
    SourceManager& sm = context_->getSourceManager();
    unsigned line = sm.getSpellingLineNumber(loc);

    // Determine if this is a read or write by looking at parent context
    bool isWrite = isWriteAccess(declRef);
    bool isRead = !isWrite;
    
    VariableUsage usage(loc, line, isRead, isWrite);
    
    LoopInfo* currentLoop = getCurrentLoop();
    auto it = currentLoop->variables.find(varName);
    if (it == currentLoop->variables.end()) {
      // Variable not yet tracked, add it
      VariableScope scope = determineVariableScope(varDecl);
      VariableInfo varInfo(varName, varDecl, scope, varDecl->getLocation());
      currentLoop->addVariable(varInfo);
    }
    
    currentLoop->addVariableUsage(varName, usage);
    
    // Don't spam output for induction variables
    if (varName != currentLoop->bounds.iterator_var) {
      std::cout << "  Variable " << varName << " " 
                << (isWrite ? "written" : "read") << " at line " << line << "\n";
    }
  }

  return true;
}

bool LoopVisitor::VisitBinaryOperator(BinaryOperator* binOp) {
  if (!binOp || !isInsideLoop()) {
    return true;
  }
  
  LoopInfo* currentLoop = getCurrentLoop();
  
  if (isArithmeticOp(binOp)) {
    currentLoop->incrementArithmeticOps();
    std::cout << "  Arithmetic operation at line " 
              << context_->getSourceManager().getSpellingLineNumber(binOp->getOperatorLoc()) 
              << "\n";
  } else if (isComparisonOp(binOp)) {
    currentLoop->incrementComparisons();
  } else if (binOp->isAssignmentOp()) {
    currentLoop->incrementAssignments();
    std::cout << "  Assignment operation at line " 
              << context_->getSourceManager().getSpellingLineNumber(binOp->getOperatorLoc()) 
              << "\n";
  }
  
  return true;
}

bool LoopVisitor::VisitUnaryOperator(UnaryOperator* unaryOp) {
  if (!unaryOp || !isInsideLoop()) {
    return true;
  }
  
  if (unaryOp->isIncrementDecrementOp()) {
    getCurrentLoop()->incrementArithmeticOps();
  }
  
  return true;
}

bool LoopVisitor::VisitCallExpr(CallExpr* callExpr) {
  if (!callExpr || !isInsideLoop()) {
    return true;
  }
  
  getCurrentLoop()->incrementFunctionCalls();
  
  std::cout << "  Function call at line " 
            << context_->getSourceManager().getSpellingLineNumber(callExpr->getBeginLoc()) 
            << "\n";
  
  return true;
}

void LoopVisitor::analyzeForLoopBounds(ForStmt* forLoop, LoopInfo& info) {
  info.bounds.init_expr = forLoop->getInit();
  info.bounds.condition_expr = forLoop->getCond();
  info.bounds.increment_expr = forLoop->getInc();

  // Try to extract iterator variable name from initialization
  if (auto* declStmt = dyn_cast_or_null<DeclStmt>(forLoop->getInit())) {
    if (declStmt->isSingleDecl()) {
      if (auto* varDecl = dyn_cast<VarDecl>(declStmt->getSingleDecl())) {
        info.bounds.iterator_var = varDecl->getNameAsString();
      }
    }
  }

  // Check for simple pattern
  if (!info.bounds.iterator_var.empty() && info.bounds.condition_expr &&
      info.bounds.increment_expr) {
    info.bounds.is_simple_pattern = true;
  }

  if (info.bounds.is_simple_pattern) {
    std::cout << "  Simple iterator pattern detected: "
              << info.bounds.iterator_var << " (depth " << info.depth << ")\n";
  }
}

void LoopVisitor::markInductionVariable(LoopInfo& loop) {
  // Mark the loop iterator as an induction variable
  if (!loop.bounds.iterator_var.empty()) {
    auto it = loop.variables.find(loop.bounds.iterator_var);
    if (it != loop.variables.end()) {
      it->second.setRole(VariableRole::INDUCTION_VAR);
      std::cout << "  Marked " << loop.bounds.iterator_var 
                << " as induction variable (safe for parallelization)\n";
    }
  }
}

void LoopVisitor::finalizeDependencyAnalysis(LoopInfo& loop) {
  dependency_analyzer_->analyzeDependencies(loop);
  bool has_deps = dependency_analyzer_->hasDependencies(loop);
  loop.setHasDependencies(has_deps);
}

bool LoopVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr* arrayExpr) {
  if (!arrayExpr || !isInsideLoop()) {
    return true;
  }

  const std::string arrayName = extractArrayBaseName(arrayExpr);

  SourceLocation loc = arrayExpr->getExprLoc();
  SourceManager& sm = context_->getSourceManager();
  unsigned line = sm.getSpellingLineNumber(loc);

  ArrayAccess access(arrayName, arrayExpr->getIdx(), loc, line, false);
  getCurrentLoop()->addArrayAccess(access);

  std::cout << "  Found array access: " << arrayName << "[] at line " << line
            << " (depth " << getCurrentLoop()->depth << ")\n";

  return true;
}

std::string LoopVisitor::extractArrayBaseName(ArraySubscriptExpr* arrayExpr) {
  Expr* base = arrayExpr->getBase()->IgnoreParenImpCasts();

  // For multi-dimensional arrays, traverse down to base
  while (auto* innerArray = dyn_cast<ArraySubscriptExpr>(base)) {
    base = innerArray->getBase()->IgnoreParenImpCasts();
  }

  if (auto* declRef = dyn_cast<DeclRefExpr>(base)) {
    return declRef->getDecl()->getNameAsString();
  }

  return "unknown";
}

bool LoopVisitor::isWriteAccess(DeclRefExpr* declRef) {
  // Look at parent nodes to determine if this is a write access
  auto parents = context_->getParents(*declRef);
  
  for (const auto& parent : parents) {
    if (auto* binaryOp = parent.get<BinaryOperator>()) {
      // Check if this declRef is the LHS of an assignment
      if (binaryOp->isAssignmentOp() && binaryOp->getLHS() == declRef) {
        return true;
      }
    } else if (auto* unaryOp = parent.get<UnaryOperator>()) {
      // Check for increment/decrement operators
      if (unaryOp->isIncrementDecrementOp()) {
        return true;
      }
    } else if (auto* compoundAssign = parent.get<CompoundAssignOperator>()) {
      // +=, -=, *=, etc.
      if (compoundAssign->getLHS() == declRef) {
        return true;
      }
    }
  }
  
  return false; // Default to read access
}

bool LoopVisitor::isArithmeticOp(BinaryOperator* binOp) {
  return binOp->isAdditiveOp() || binOp->isMultiplicativeOp();
}

bool LoopVisitor::isComparisonOp(BinaryOperator* binOp) {
  return binOp->isComparisonOp();
}

VariableScope LoopVisitor::determineVariableScope(VarDecl* varDecl) const {
  // Simple heuristic for now - we can improve this later
  if (!varDecl) {
    return VariableScope::GLOBAL;
  }

  // Check if declared within current loop body (rough approximation)
  SourceManager& sm = context_->getSourceManager();
  SourceLocation declLoc = varDecl->getLocation();
  
  if (!isInsideLoop()) {
    return VariableScope::FUNCTION_LOCAL;
  }
  
  // For now, assume variables found while traversing loop are loop-local
  // This is a simplification - real implementation would check scopes more carefully
  return VariableScope::LOOP_LOCAL;
}

void LoopVisitor::addLoop(Stmt* stmt, SourceLocation loc,
                          const std::string& type) {
  if (!loc.isValid()) {
    std::cout << "Warning: Invalid source location for " << type << " loop\n";
    return;
  }

  SourceManager& sm = context_->getSourceManager();
  unsigned line = sm.getSpellingLineNumber(loc);

  loops_.emplace_back(stmt, loc, line, type);

  unsigned depth = static_cast<unsigned>(loop_stack_.size());
  std::cout << "Found " << type << " loop at line " << line << " (depth "
            << depth << ")\n";
}

void LoopVisitor::printLoopSummary() const {
  std::cout << "\n=== Loop Analysis Summary ===\n";
  std::cout << "Total loops detected: " << loops_.size() << "\n";

  // Count different categories
  size_t outermost_count = 0;
  size_t hot_loops = 0;
  size_t parallelizable = 0;
  
  for (const auto& loop : loops_) {
    if (loop.isOutermost()) outermost_count++;
    if (loop.isHot()) hot_loops++;
    if (loop.isOutermost() && loop.isParallelizable()) parallelizable++;
  }
  
  std::cout << "Outermost loops: " << outermost_count << "\n";
  std::cout << "Hot loops (high computation): " << hot_loops << "\n";
  std::cout << "Safe for parallelization: " << parallelizable << "\n\n";

  for (const auto& loop : loops_) {
    // Indent based on depth
    for (unsigned i = 0; i < loop.depth; i++) {
      std::cout << "  ";
    }

    std::cout << loop.loop_type << " loop at line " << loop.line_number;

    if (loop.bounds.is_simple_pattern) {
      std::cout << " (" << loop.bounds.iterator_var << ")";
    }

    // Show hotness score
    std::cout << " [hotness: " << loop.metrics.hotness_score << "]";
    
    if (loop.isHot()) {
      std::cout << " HOT";
    }

    if (loop.isOutermost()) {
      if (loop.isParallelizable()) {
        std::cout << " SAFE TO PARALLELIZE";
      } else {
        std::cout << " HAS DEPENDENCIES";
      }
    }

    std::cout << "\n";
    
    // Print detailed analysis for outermost loops
    if (loop.isOutermost() && !loop.variables.empty()) {
      for (unsigned i = 0; i <= loop.depth; i++) std::cout << "  ";
      std::cout << "Variable analysis:\n";
      
      for (const auto& var_pair : loop.variables) {
        const auto& var = var_pair.second;
        for (unsigned i = 0; i <= loop.depth + 1; i++) std::cout << "  ";
        
        std::cout << var.name << ": ";
        if (var.isInductionVariable()) {
          std::cout << "INDUCTION VAR (safe)";
        } else if (var.isPotentialDependency()) {
          std::cout << "READ+WRITE (dependency)";
        } else if (var.hasWrites()) {
          std::cout << "WRITE-ONLY (safe)";
        } else if (var.hasReads()) {
          std::cout << "READ-ONLY (safe)";
        }
        std::cout << " [" << var.usages.size() << " uses]\n";
      }
    }
  }
}

} // namespace statik