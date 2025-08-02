#pragma once

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "analyzer/LoopVisitor.h"
#include "analyzer/DependencyAnalyzer.h"

namespace statik {

class AnalyzerVisitor : public clang::RecursiveASTVisitor<AnalyzerVisitor> {
public:
    explicit AnalyzerVisitor(clang::ASTContext* context)
        : context_(context), 
          dependency_analyzer_(context),
          loop_visitor_(context, &dependency_analyzer_), 
          generate_pragmas_(false), verbose_(false) {}
          
    void enablePragmaGeneration(const std::string& output_file, const std::string& input_file) {
        generate_pragmas_ = true;
        output_filename_ = output_file;
        input_filename_ = input_file;
    }
    
    void setVerbose(bool verbose) {
        verbose_ = verbose;
        loop_visitor_.setVerbose(verbose);
    }

    bool VisitFunctionDecl(clang::FunctionDecl* func);
    void runAnalysis();

private:
    clang::ASTContext* context_;
    DependencyAnalyzer dependency_analyzer_;
    LoopVisitor loop_visitor_;
    bool generate_pragmas_;
    std::string output_filename_;
    std::string input_filename_;
    bool verbose_;
};

class AnalyzerConsumer : public clang::ASTConsumer {
public:
    explicit AnalyzerConsumer(clang::ASTContext* context) : visitor_(context) {}
        
    void enablePragmaGeneration(const std::string& output_file, const std::string& input_file) {
        visitor_.enablePragmaGeneration(output_file, input_file);
    }
    
    void setVerbose(bool verbose) {
        visitor_.setVerbose(verbose);
    }

    void HandleTranslationUnit(clang::ASTContext& context) override {
        visitor_.TraverseDecl(context.getTranslationUnitDecl());
        visitor_.runAnalysis();
    }

private:
    AnalyzerVisitor visitor_;
};

} // namespace statik