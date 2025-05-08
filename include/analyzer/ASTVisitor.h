#pragma once

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"

namespace statik {

class AnalyzerVisitor : public clang::RecursiveASTVisitor<AnalyzerVisitor> {
public:
    explicit AnalyzerVisitor(clang::ASTContext* context) : context_(context) {}
    
    bool VisitFunctionDecl(clang::FunctionDecl* func);
    
private:
    clang::ASTContext* context_;
};

class AnalyzerConsumer : public clang::ASTConsumer {
public:
    explicit AnalyzerConsumer(clang::ASTContext* context) 
        : visitor_(context) {}
        
    void HandleTranslationUnit(clang::ASTContext& context) override {
        visitor_.TraverseDecl(context.getTranslationUnitDecl());
    }
    
private:
    AnalyzerVisitor visitor_;
};

} // namespace statik