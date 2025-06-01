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

  // Analyze loop bounds
  analyzeForLoopBounds(forLoop, *newLoop);

  // Push this loop onto stack and traverse body
  loop_stack_.push(newLoop);

  if (forLoop->getBody()) {
    TraverseStmt(forLoop->getBody());
  }

  loop_stack_.pop(); // Remove from stack when done
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

  loop_stack_.pop();
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

  // Count outermost loops
  size_t outermost_count = 0;
  for (const auto& loop : loops_) {
    if (loop.isOutermost()) {
      outermost_count++;
    }
  }
  std::cout << "Outermost loops (parallelization candidates): "
            << outermost_count << "\n\n";

  for (const auto& loop : loops_) {
    // Indent based on depth
    for (unsigned i = 0; i < loop.depth; i++) {
      std::cout << "  ";
    }

    std::cout << loop.loop_type << " loop at line " << loop.line_number;

    if (loop.bounds.is_simple_pattern) {
      std::cout << " (" << loop.bounds.iterator_var << ")";
    }

    if (!loop.array_accesses.empty()) {
      std::cout << " [" << loop.array_accesses.size() << " accesses]";
    }

    if (loop.isOutermost()) {
      std::cout << " ← PARALLELIZABLE?";
    }

    std::cout << "\n";
  }
}

} // namespace statik