#include "analyzer/ASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include <iostream>

using namespace clang;

namespace statik {

bool AnalyzerVisitor::VisitFunctionDecl(FunctionDecl* func) {
    if (!func->hasBody()) {
        return true;  // Skip declarations without definitions
    }
    
    std::string funcName = func->getNameAsString();
    SourceLocation loc = func->getLocation();
    
    std::cout << "Found function: " << funcName;
    if (loc.isValid()) {
        SourceManager& sm = context_->getSourceManager();
        unsigned line = sm.getSpellingLineNumber(loc);
        std::cout << " at line " << line;
    }
    std::cout << "\n";
    
    return true;
}

bool AnalyzerVisitor::VisitForStmt(ForStmt* forLoop) {
    SourceLocation loc = forLoop->getForLoc();
    SourceManager& sm = context_->getSourceManager();
    unsigned line = sm.getSpellingLineNumber(loc);
    
    loops_.emplace_back(forLoop, loc, line, "for");
    
    std::cout << "Found for loop at line " << line << "\n";
    
    return true;
}

void AnalyzerVisitor::printResults() const {
    std::cout << "\n=== Analysis Results ===\n";
    std::cout << "Total loops found: " << loops_.size() << "\n";
    
    for (const auto& loop : loops_) {
        std::cout << "  " << loop.loop_type << " loop at line " 
                  << loop.line_number << "\n";
    }
}

} // namespace statik