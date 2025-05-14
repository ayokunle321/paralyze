#pragma once

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "analyzer/LoopInfo.h"

namespace statik {

class AnalyzerVisitor : public clang::RecursiveASTVisitor<AnalyzerVisitor> {
public:
    explicit AnalyzerVisitor(clang::ASTContext* context) : context_(context) {}
    
    bool VisitFunctionDecl(clang::FunctionDecl* func);
    bool VisitForStmt(clang::ForStmt* forLoop);
    bool VisitWhileStmt(clang::WhileStmt* whileLoop);
    bool VisitDoStmt(clang::DoStmt* doLoop);
    
    void printResults() const;
    
private:
    clang::ASTContext* context_;
    std::vector<LoopInfo> loops_;
};

class AnalyzerConsumer : public clang::ASTConsumer {
public:
    explicit AnalyzerConsumer(clang::ASTContext* context) 
        : visitor_(context) {}
        
    void HandleTranslationUnit(clang::ASTContext& context) override {
        visitor_.TraverseDecl(context.getTranslationUnitDecl());
        visitor_.printResults();
    }
    
private:
    AnalyzerVisitor visitor_;
};

} // namespace statik