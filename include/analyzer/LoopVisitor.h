#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/DependencyAnalyzer.h"
#include <stack>
#include <vector>
#include <map>

namespace statik {

struct LineArrayAccesses {
    unsigned line_number;
    std::vector<std::pair<std::string, bool>> accesses; // {pattern, is_write}
};

class LoopVisitor : public clang::RecursiveASTVisitor<LoopVisitor> {
public:
    explicit LoopVisitor(clang::ASTContext* context, DependencyAnalyzer* analyzer)
        : context_(context), dependency_analyzer_(analyzer), verbose_(false) {}

    bool VisitForStmt(clang::ForStmt* forLoop);
    bool VisitWhileStmt(clang::WhileStmt* whileLoop);
    bool VisitDoStmt(clang::DoStmt* doLoop);
    bool VisitVarDecl(clang::VarDecl* varDecl);
    bool VisitDeclRefExpr(clang::DeclRefExpr* declRef);
    bool VisitBinaryOperator(clang::BinaryOperator* binOp);
    bool VisitUnaryOperator(clang::UnaryOperator* unaryOp);
    bool VisitCallExpr(clang::CallExpr* callExpr);
    bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* arrayExpr);

    const std::vector<LoopInfo>& getLoops() const { return loops_; }
    void printLoopSummary() const;
    void setVerbose(bool verbose) { verbose_ = verbose; }

private:
    clang::ASTContext* context_;
    DependencyAnalyzer* dependency_analyzer_;
    std::vector<LoopInfo> loops_;
    std::stack<LoopInfo*> loop_stack_;
    bool verbose_;
    std::map<unsigned, LineArrayAccesses> line_access_summaries_;

    bool isInsideLoop() const { return !loop_stack_.empty(); }
    LoopInfo* getCurrentLoop() { return loop_stack_.empty() ? nullptr : loop_stack_.top(); }

    void addLoop(clang::Stmt* stmt, clang::SourceLocation loc, const std::string& type);
    void analyzeForLoopBounds(clang::ForStmt* forLoop, LoopInfo& info);
    void markInductionVariable(LoopInfo& loop);
    void finalizeDependencyAnalysis(LoopInfo& loop);

    std::string extractArrayBaseName(clang::ArraySubscriptExpr* arrayExpr);
    std::string extractSubscriptString(clang::Expr* idx);
    void printArrayAccessSummary();

    bool isWriteAccess(clang::DeclRefExpr* declRef);
    bool isArithmeticOp(clang::BinaryOperator* binOp);
    bool isComparisonOp(clang::BinaryOperator* binOp);
    VariableScope determineVariableScope(clang::VarDecl* varDecl) const;
};

} // namespace statik