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

    bool TraverseForStmt(clang::ForStmt* forLoop);
    bool TraverseWhileStmt(clang::WhileStmt* whileLoop);
    bool TraverseDoStmt(clang::DoStmt* doLoop);
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
    std::stack<size_t> loop_stack_;     
    
    LoopInfo* getCurrentLoop() {
        if (loop_stack_.empty()) return nullptr;
        return &loops_[loop_stack_.top()];
    }
    
    const LoopInfo* getCurrentLoop() const {
        if (loop_stack_.empty()) return nullptr;
        return &loops_[loop_stack_.top()];
    }

    bool verbose_;
    std::map<unsigned, LineArrayAccesses> line_access_summaries_;

    bool isInsideLoop() const { return !loop_stack_.empty(); }
    void addLoop(clang::Stmt* stmt, clang::SourceLocation loc, const std::string& type);
    void analyzeForLoopBounds(clang::ForStmt* forLoop, LoopInfo& info);
    void markInductionVariable(LoopInfo& loop);
    void finalizeDependencyAnalysis(LoopInfo& loop);

    std::string extractArrayBaseName(clang::ArraySubscriptExpr* arrayExpr);
    std::string extractSubscriptString(clang::Expr* idx);
    void printArrayAccessSummary();

    std::string extractPointerBaseName(clang::Expr* expr);
    bool isWriteAccessUnary(clang::UnaryOperator* unaryOp);

    bool isWriteAccess(clang::DeclRefExpr* declRef);
    bool isArithmeticOp(clang::BinaryOperator* binOp);
    bool isComparisonOp(clang::BinaryOperator* binOp);
    VariableScope determineVariableScope(clang::VarDecl* varDecl) const;
};

} // namespace statik