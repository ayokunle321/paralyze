#include "analyzer/LoopVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"
#include <iostream>

using namespace clang;

namespace statik {

bool LoopVisitor::VisitForStmt(ForStmt* forLoop) {
    if (!forLoop) {
        return true;
    }
    
    SourceLocation loc = forLoop->getForLoc();
    addLoop(forLoop, loc, "for");
    
    // Set current loop context and traverse the body
    LoopInfo* prev_loop = current_loop_;
    current_loop_ = &loops_.back();
    
    if (forLoop->getBody()) {
        TraverseStmt(forLoop->getBody());
    }
    
    current_loop_ = prev_loop;  // Restore previous context
    return true;
}

bool LoopVisitor::VisitWhileStmt(WhileStmt* whileLoop) {
    if (!whileLoop) {
        return true;
    }
    
    SourceLocation loc = whileLoop->getWhileLoc();
    addLoop(whileLoop, loc, "while");
    
    LoopInfo* prev_loop = current_loop_;
    current_loop_ = &loops_.back();
    
    if (whileLoop->getBody()) {
        TraverseStmt(whileLoop->getBody());
    }
    
    current_loop_ = prev_loop;
    return true;
}

bool LoopVisitor::VisitDoStmt(DoStmt* doLoop) {
    if (!doLoop) {
        return true;
    }
    
    SourceLocation loc = doLoop->getDoLoc();
    addLoop(doLoop, loc, "do-while");
    
    LoopInfo* prev_loop = current_loop_;
    current_loop_ = &loops_.back();
    
    if (doLoop->getBody()) {
        TraverseStmt(doLoop->getBody());
    }
    
    current_loop_ = prev_loop;
    return true;
}

bool LoopVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr* arrayExpr) {
    if (!arrayExpr || !isInsideLoop()) {
        return true;
    }
    
    // Get the base array expression
    Expr* base = arrayExpr->getBase()->IgnoreParenImpCasts();
    std::string arrayName = "unknown";
    
    if (auto declRef = dyn_cast<DeclRefExpr>(base)) {
        arrayName = declRef->getDecl()->getNameAsString();
    }
    
    SourceLocation loc = arrayExpr->getExprLoc();
    SourceManager& sm = context_->getSourceManager();
    unsigned line = sm.getSpellingLineNumber(loc);
    
    // For now, assume all accesses are reads - we'll improve this later
    ArrayAccess access(arrayName, arrayExpr->getIdx(), loc, line, false);
    current_loop_->addArrayAccess(access);
    
    std::cout << "  Found array access: " << arrayName << "[] at line " << line << "\n";
    
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
                  << loop.line_number;
        if (!loop.array_accesses.empty()) {
            std::cout << " (" << loop.array_accesses.size() << " array accesses)";
        }
        std::cout << "\n";
        
        // Print array access details
        for (const auto& access : loop.array_accesses) {
            std::cout << "    - " << access.array_name << "[] at line " 
                      << access.line_number << "\n";
        }
    }
}

} // namespace statik