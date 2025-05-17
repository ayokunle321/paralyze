#pragma once

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "analyzer/LoopVisitor.h"

namespace statik {

class AnalyzerVisitor : public clang::RecursiveASTVisitor<AnalyzerVisitor> {
public:
    explicit AnalyzerVisitor(clang::ASTContext* context) 
        : context_(context), loop_visitor_(context) {}
    
    bool VisitFunctionDecl(clang::FunctionDecl* func);
    
    void runAnalysis();
    
private:
    clang::ASTContext* context_;
    LoopVisitor loop_visitor_;
};

class AnalyzerConsumer : public clang::ASTConsumer {
public:
    explicit AnalyzerConsumer(clang::ASTContext* context) 
        : visitor_(context) {}
        
    void HandleTranslationUnit(clang::ASTContext& context) override {
        visitor_.TraverseDecl(context.getTranslationUnitDecl());
        visitor_.runAnalysis();
    }
    
private:
    AnalyzerVisitor visitor_;
};

} // namespace statik