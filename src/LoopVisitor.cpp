#include "analyzer/LoopVisitor.h"
#include "clang/AST/Stmt.h"
#include <iostream>

using namespace clang;

namespace statik {

bool LoopVisitor::VisitForStmt(ForStmt* forLoop) {
    if (!forLoop) {
        return true;
    }
    
    SourceLocation loc = forLoop->getForLoc();
    addLoop(forLoop, loc, "for");
    
    return true;
}

bool LoopVisitor::VisitWhileStmt(WhileStmt* whileLoop) {
    if (!whileLoop) {
        return true;
    }
    
    SourceLocation loc = whileLoop->getWhileLoc();
    addLoop(whileLoop, loc, "while");
    
    return true;
}

bool LoopVisitor::VisitDoStmt(DoStmt* doLoop) {
    if (!doLoop) {
        return true;
    }
    
    SourceLocation loc = doLoop->getDoLoc();
    addLoop(doLoop, loc, "do-while");
    
    return true;
}

void LoopVisitor::addLoop(Stmt* stmt, SourceLocation loc, const std::string& type) {
    if (!loc.isValid()) {
        std::cout << "Warning: Invalid source location for " << type << " loop\n";
        return;
    }
    
    SourceManager& sm = context_->getSourceManager();
    unsigned line = sm.getSpellingLineNumber(loc);
    
    loops_.emplace_back(stmt, loc, line, type);
    std::cout << "Found " << type << " loop at line " << line << "\n";
}

void LoopVisitor::printLoopSummary() const {
    std::cout << "\n=== Loop Analysis Summary ===\n";
    std::cout << "Total loops detected: " << loops_.size() << "\n";
    
    for (const auto& loop : loops_) {
        std::cout << "  " << loop.loop_type << " loop at line " 
                  << loop.line_number << "\n";
    }
}

} // namespace statik