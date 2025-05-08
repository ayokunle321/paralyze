#include "analyzer/ASTVisitor.h"
#include "clang/AST/ASTContext.h"
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

} // namespace statik