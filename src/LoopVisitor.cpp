#include "analyzer/LoopVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ParentMapContext.h"
#include "analyzer/FunctionCallAnalyzer.h"
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

  if (verbose_) {
    std::cout << "  Found variable declaration: " << varName 
              << " (scope: " << (scope == VariableScope::LOOP_LOCAL ? "local" : "external")
              << ")\n";
  }

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
    
    // Don't spam output for induction variables, and respect verbose flag
    if (verbose_ && varName != currentLoop->bounds.iterator_var) {
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
    if (verbose_) {
      std::cout << "  Arithmetic operation at line " 
                << context_->getSourceManager().getSpellingLineNumber(binOp->getOperatorLoc()) 
                << "\n";
    }
  } else if (isComparisonOp(binOp)) {
    currentLoop->incrementComparisons();
  } else if (binOp->isAssignmentOp()) {
    currentLoop->incrementAssignments();
    if (verbose_) {
      std::cout << "  Assignment operation at line " 
                << context_->getSourceManager().getSpellingLineNumber(binOp->getOperatorLoc()) 
                << "\n";
    }
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
    
    LoopInfo* currentLoop = getCurrentLoop();
    currentLoop->incrementFunctionCalls();
    
    if (verbose_) {
      std::cout << " Function call at line "
                << context_->getSourceManager().getSpellingLineNumber(callExpr->getBeginLoc())
                << "\n";
    }
    
    // Analyze the function call and store the result in LoopInfo
    FunctionCallAnalyzer temp_analyzer(context_);
    temp_analyzer.visitCallExpr(callExpr, *currentLoop);
    
    // Extract the function name and safety info
    std::string func_name;
    if (auto* funcDecl = callExpr->getDirectCallee()) {
        if (funcDecl->getDeclName().isIdentifier()) {
            func_name = funcDecl->getNameAsString();
        }
    }
    
    if (func_name.empty()) {
        func_name = "unknown_function";
    }
    
    // Check if it's safe using the same logic as FunctionCallAnalyzer
    bool is_safe = true;
    static const std::set<std::string> unsafe_functions = {
        "printf", "fprintf", "sprintf", "puts", "putchar",
        "scanf", "fscanf", "sscanf", "getchar", "gets", "fgets",
        "malloc", "calloc", "realloc", "free",
        "fopen", "fclose", "fread", "fwrite", "fseek", "ftell",
        "exit", "abort", "system", "rand", "srand", "time"
    };
    
    if (unsafe_functions.find(func_name) != unsafe_functions.end()) {
        is_safe = false;
    }
    
    // Store in LoopInfo
    currentLoop->addDetectedFunctionCall(func_name, is_safe);
    
    return true;
}

void LoopVisitor::analyzeForLoopBounds(ForStmt* forLoop, LoopInfo& info) {
    info.bounds.init_expr = nullptr;
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
    
    if (verbose_ && info.bounds.is_simple_pattern) {
        std::cout << " Simple iterator pattern detected: "
                  << info.bounds.iterator_var << " (depth " << info.depth << ")\n";
    }
}

void LoopVisitor::markInductionVariable(LoopInfo& loop) {
  // Mark the loop iterator as an induction variable
  if (!loop.bounds.iterator_var.empty()) {
    auto it = loop.variables.find(loop.bounds.iterator_var);
    if (it != loop.variables.end()) {
      it->second.setRole(VariableRole::INDUCTION_VAR);
      if (verbose_) {
        std::cout << "  Marked " << loop.bounds.iterator_var 
                  << " as induction variable (safe for parallelization)\n";
      }
    }
  }
}

void LoopVisitor::finalizeDependencyAnalysis(LoopInfo& loop) {
  // Pass verbose flag to dependency analyzer
  if (dependency_analyzer_) {
    dependency_analyzer_->setVerbose(verbose_);
  }
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
    
    // Determine if this is a write access by checking parent context
    bool is_write = false;
    auto parents = context_->getParents(*arrayExpr);
    for (const auto& parent : parents) {
        if (auto* binaryOp = parent.get<BinaryOperator>()) {
            // Check if this array access is the LHS of an assignment
            if (binaryOp->isAssignmentOp() && binaryOp->getLHS() == arrayExpr) {
                is_write = true;
                break;
            }
        }
    }
    
    // Create ArrayAccess with proper write/read flag and subscript
    ArrayAccess access(arrayName, arrayExpr->getIdx(), loc, line, is_write);
    getCurrentLoop()->addArrayAccess(access);
    
    if (verbose_) {
      // Show the actual subscript expression for debugging
      std::string subscript_str = "unknown";
      if (arrayExpr->getIdx()) {
          // Use the same exprToString logic as ArrayDependencyAnalyzer
          Expr* idx = arrayExpr->getIdx()->IgnoreParenImpCasts();
          if (auto* declRef = dyn_cast<DeclRefExpr>(idx)) {
              subscript_str = declRef->getDecl()->getNameAsString();
          } else if (auto* binOp = dyn_cast<BinaryOperator>(idx)) {
              // Handle expressions like i-1, i+1
              if (auto* lhs = dyn_cast<DeclRefExpr>(binOp->getLHS())) {
                  if (auto* rhs = dyn_cast<IntegerLiteral>(binOp->getRHS())) {
                      std::string var = lhs->getDecl()->getNameAsString();
                      int constant = rhs->getValue().getSExtValue();
                      if (binOp->getOpcode() == BO_Add) {
                          subscript_str = var + "+" + std::to_string(constant);
                      } else if (binOp->getOpcode() == BO_Sub) {
                          subscript_str = var + "-" + std::to_string(constant);
                      }
                  }
              }
          } else if (auto* intLit = dyn_cast<IntegerLiteral>(idx)) {
              subscript_str = std::to_string(intLit->getValue().getSExtValue());
          }
      }
      
      std::cout << " Found array access: " << arrayName << "[" << subscript_str << "] at line " << line
                << " (" << (is_write ? "WRITE" : "READ") << ")"
                << " (depth " << getCurrentLoop()->depth << ")\n";
    }
    
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
  if (verbose_) {
    std::cout << "Found " << type << " loop at line " << line << " (depth "
                << depth << ")\n";
  }
}

void LoopVisitor::printLoopSummary() const {
    std::cout << "\n=== Analysis Results ===\n";
    
    if (loops_.empty()) {
        std::cout << "No loops detected in the analyzed code.\n";
        return;
    }
    
    // Print table header
    std::cout << "Loop   Location    Type     Status          Reason\n";
    std::cout << "----   --------    ----     ------          ------\n";
    
    size_t parallelizable_count = 0;
    
    for (size_t i = 0; i < loops_.size(); i++) {
        const auto& loop = loops_[i];
        
        // Format: L1     line 6      for      PARALLELIZABLE  Simple array operations
        std::cout << "L" << (i + 1);
        
        // Pad to align columns
        std::cout << "     line " << loop.line_number;
        if (loop.line_number < 10) std::cout << " ";  // Extra space for single digits
        
        std::cout << "     " << loop.loop_type;
        if (loop.loop_type.length() < 5) std::cout << " ";  // Pad short types
        
        // Determine status and reason
        std::string status, reason;
        if (loop.isParallelizable()) {
            status = "PARALLELIZABLE";
            parallelizable_count++;
            
            // Determine why it's safe
            if (loop.bounds.is_simple_pattern && !loop.array_accesses.empty()) {
                reason = "Simple array operations";
            } else if (loop.bounds.is_simple_pattern) {
                reason = "Simple iterator pattern";
            } else {
                reason = "No dependencies detected";
            }
        } else {
            status = "UNSAFE";
            
            // Determine why it's unsafe
            bool has_array_deps = false;
            bool has_function_calls = false;
            
            // Check if this loop has unsafe function calls
            if (loop.hasUnsafeFunctionCalls()) {
                has_function_calls = true;
            }
            
            // Check if this loop has array dependencies (we'd need to check the actual dependencies)
            // For now, we'll infer from the other conditions
            if (!has_function_calls && loop.has_dependencies) {
                has_array_deps = true;
            }
            
            if (has_function_calls && has_array_deps) {
                reason = "Function calls + dependencies";
            } else if (has_function_calls) {
                reason = "Function call side effects";
            } else if (has_array_deps) {
                reason = "Loop-carried dependency";
            } else {
                reason = "Complex dependencies";
            }
        }
        
        std::cout << "     " << status;
        
        // Pad status column
        for (int pad = status.length(); pad < 15; pad++) {
            std::cout << " ";
        }
        
        std::cout << " " << reason << "\n";
    }
    
    // Summary line
    std::cout << "\nParallelizable: " << parallelizable_count << "/" << loops_.size() 
              << " loops (" << (loops_.size() > 0 ? (parallelizable_count * 100 / loops_.size()) : 0) << "%)\n";
}

} // namespace statik