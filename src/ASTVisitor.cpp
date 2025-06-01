#include "analyzer/ASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include <iostream>

using namespace clang;

namespace statik {

bool AnalyzerVisitor::VisitFunctionDecl(FunctionDecl* func) {
  if (!func || !func->hasBody()) {
    return true;
  }

  const std::string funcName = func->getNameAsString();
  SourceLocation loc = func->getLocation();

  std::cout << "Found function: " << funcName;
  if (loc.isValid()) {
    SourceManager& sm = context_->getSourceManager();
    unsigned line = sm.getSpellingLineNumber(loc);
    std::cout << " at line " << line;
  }
  std::cout << "\n";

  // Let the loop visitor handle loop detection
  loop_visitor_.TraverseStmt(func->getBody());

  return true;
}

void AnalyzerVisitor::runAnalysis() { 
  loop_visitor_.printLoopSummary(); 
}

} // namespace statik