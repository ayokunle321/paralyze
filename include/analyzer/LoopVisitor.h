#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include <vector>

namespace statik {

class LoopVisitor : public clang::RecursiveASTVisitor<LoopVisitor> {
public:
    explicit LoopVisitor(clang::ASTContext* context) : context_(context) {}
    
    bool VisitForStmt(clang::ForStmt* forLoop);
    bool VisitWhileStmt(clang::WhileStmt* whileLoop);
    bool VisitDoStmt(clang::DoStmt* doLoop);
    
    const std::vector<LoopInfo>& getLoops() const { return loops_; }
    void printLoopSummary() const;
    
private:
    clang::ASTContext* context_;
    std::vector<LoopInfo> loops_;
    
    void addLoop(clang::Stmt* stmt, clang::SourceLocation loc, const std::string& type);
};

} // namespace statik