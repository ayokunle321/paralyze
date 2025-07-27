#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/DependencyAnalyzer.h"
#include <vector>
#include <stack>
#include <memory>

namespace statik {

class LoopVisitor : public clang::RecursiveASTVisitor<LoopVisitor> {
public:
  explicit LoopVisitor(clang::ASTContext* context) 
      : context_(context), 
        dependency_analyzer_(std::make_unique<DependencyAnalyzer>(context)),
        verbose_(false) {}

  bool VisitForStmt(clang::ForStmt* forLoop);
  bool VisitWhileStmt(clang::WhileStmt* whileLoop);
  bool VisitDoStmt(clang::DoStmt* doLoop);
  bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* arrayExpr);
  bool VisitVarDecl(clang::VarDecl* varDecl);
  bool VisitDeclRefExpr(clang::DeclRefExpr* declRef);
  bool VisitBinaryOperator(clang::BinaryOperator* binOp);
  bool VisitUnaryOperator(clang::UnaryOperator* unaryOp);
  bool VisitCallExpr(clang::CallExpr* callExpr);

  const std::vector<LoopInfo>& getLoops() const { return loops_; }
  void printLoopSummary() const;
  void setVerbose(bool verbose) { verbose_ = verbose; }

private:
  clang::ASTContext* context_;
  std::vector<LoopInfo> loops_;
  std::stack<LoopInfo*> loop_stack_; // Track nesting hierarchy
  std::unique_ptr<DependencyAnalyzer> dependency_analyzer_;
  bool verbose_; 

  void addLoop(clang::Stmt* stmt, clang::SourceLocation loc,
               const std::string& type);
  void analyzeForLoopBounds(clang::ForStmt* forLoop, LoopInfo& info);
  void markInductionVariable(LoopInfo& loop);
  void finalizeDependencyAnalysis(LoopInfo& loop);
  std::string extractArrayBaseName(clang::ArraySubscriptExpr* arrayExpr);
  VariableScope determineVariableScope(clang::VarDecl* varDecl) const;
  bool isWriteAccess(clang::DeclRefExpr* declRef);
  bool isArithmeticOp(clang::BinaryOperator* binOp);
  bool isComparisonOp(clang::BinaryOperator* binOp);
  bool isInsideLoop() const { return !loop_stack_.empty(); }
  LoopInfo* getCurrentLoop() const {
    return loop_stack_.empty() ? nullptr : loop_stack_.top();
  }
};

} // namespace statik