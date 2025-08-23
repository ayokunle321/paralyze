#include "analyzer/LoopVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ParentMapContext.h"
#include "analyzer/FunctionCallAnalyzer.h"
#include <iostream>

using namespace clang;

namespace statik {

bool LoopVisitor::VisitForStmt(ForStmt* forLoop) {
    if (!forLoop) {
        return true;
    }

    SourceLocation loc = forLoop->getForLoc();
    addLoop(forLoop, loc, "for");

    LoopInfo* newLoop = &loops_.back();
    if (!loop_stack_.empty()) {
        newLoop->setParent(loop_stack_.top());
    }

    analyzeForLoopBounds(forLoop, *newLoop);

    // Process the loop body
    loop_stack_.push(newLoop);
    if (forLoop->getBody()) {
        TraverseStmt(forLoop->getBody());
    }

    markInductionVariable(*newLoop);
    finalizeDependencyAnalysis(*newLoop);
    newLoop->finalizeMetrics();
    loop_stack_.pop();
    return true;
}

bool LoopVisitor::VisitWhileStmt(WhileStmt* whileLoop) {
    if (!whileLoop) {
        return true;
    }

    SourceLocation loc = whileLoop->getWhileLoc();
    addLoop(whileLoop, loc, "while");
    LoopInfo* newLoop = &loops_.back();
    if (!loop_stack_.empty()) {
        newLoop->setParent(loop_stack_.top());
    }

    loop_stack_.push(newLoop);
    if (whileLoop->getBody()) {
        TraverseStmt(whileLoop->getBody());
    }

    markInductionVariable(*newLoop);
    finalizeDependencyAnalysis(*newLoop);
    newLoop->finalizeMetrics();
    loop_stack_.pop();
    return true;
}

bool LoopVisitor::VisitDoStmt(DoStmt* doLoop) {
    if (!doLoop) {
        return true;
    }

    SourceLocation loc = doLoop->getDoLoc();
    addLoop(doLoop, loc, "do-while");
    LoopInfo* newLoop = &loops_.back();
    if (!loop_stack_.empty()) {
        newLoop->setParent(loop_stack_.top());
    }

    loop_stack_.push(newLoop);
    if (doLoop->getBody()) {
        TraverseStmt(doLoop->getBody());
    }

    markInductionVariable(*newLoop);
    finalizeDependencyAnalysis(*newLoop);
    newLoop->finalizeMetrics();
    loop_stack_.pop();
    return true;
}

bool LoopVisitor::VisitVarDecl(VarDecl* varDecl) {
    if (!varDecl || !isInsideLoop()) {
        return true;
    }

    const std::string varName = varDecl->getNameAsString();
    VariableScope scope = determineVariableScope(varDecl);
    SourceLocation loc = varDecl->getLocation();
    VariableInfo varInfo(varName, varDecl, scope, loc);

    if (loop_stack_.empty()) {
        return true;
    }
    loop_stack_.top()->addVariable(varInfo);

    return true;
}

bool LoopVisitor::VisitDeclRefExpr(DeclRefExpr* declRef) {
    if (!declRef || !isInsideLoop()) {
        return true;
    }

    if (auto* varDecl = dyn_cast<VarDecl>(declRef->getDecl())) {
        const std::string varName = varDecl->getNameAsString();
        SourceLocation loc = declRef->getLocation();
        SourceManager& sm = context_->getSourceManager();
        unsigned line = sm.getSpellingLineNumber(loc);

        bool isWrite = isWriteAccess(declRef);
        bool isRead = !isWrite;

        VariableUsage usage(loc, line, isRead, isWrite);
        
        if (loop_stack_.empty()) {
            return true;
        }
        LoopInfo* currentLoop = loop_stack_.top();
        
        auto it = currentLoop->variables.find(varName);
        if (it == currentLoop->variables.end()) {
            VariableScope scope = determineVariableScope(varDecl);
            VariableInfo varInfo(varName, varDecl, scope, varDecl->getLocation());
            currentLoop->addVariable(varInfo);
        }
        
        currentLoop->addVariableUsage(varName, usage);
    }
    return true;
}

bool LoopVisitor::VisitBinaryOperator(BinaryOperator* binOp) {
    if (!binOp || !isInsideLoop()) {
        return true;
    }

    LoopInfo* currentLoop = getCurrentLoop();
    if (isArithmeticOp(binOp)) {
        currentLoop->incrementArithmeticOps();
    } else if (isComparisonOp(binOp)) {
        currentLoop->incrementComparisons();
    } else if (binOp->isAssignmentOp()) {
        currentLoop->incrementAssignments();
        if (verbose_) {
            std::cout << "  Assignment operation at line "
                      << context_->getSourceManager().getSpellingLineNumber(binOp->getOperatorLoc())
                      << "\n";
        }
    }

    return true;
}

bool LoopVisitor::VisitUnaryOperator(UnaryOperator* unaryOp) {
    if (!unaryOp || !isInsideLoop()) {
        return true;
    }

    if (unaryOp->isIncrementDecrementOp()) {
        getCurrentLoop()->incrementArithmeticOps();
    }

    return true;
}

bool LoopVisitor::VisitCallExpr(CallExpr* callExpr) {
    if (!callExpr || !isInsideLoop()) {
        return true;
    }

    LoopInfo* currentLoop = getCurrentLoop();
    currentLoop->incrementFunctionCalls();

    if (verbose_) {
        std::cout << "  Function call at line "
                  << context_->getSourceManager().getSpellingLineNumber(callExpr->getBeginLoc())
                  << "\n";
    }

    // Analyze function call for safety
    FunctionCallAnalyzer temp_analyzer(context_);
    temp_analyzer.visitCallExpr(callExpr, *currentLoop);

    std::string func_name;
    if (auto* funcDecl = callExpr->getDirectCallee()) {
        if (funcDecl->getDeclName().isIdentifier()) {
            func_name = funcDecl->getNameAsString();
        }
    }
    if (func_name.empty()) {
        func_name = "unknown_function";
    }

    // Check safety using predefined unsafe function list
    bool is_safe = true;
    static const std::set<std::string> unsafe_functions = {
        "printf", "fprintf", "sprintf", "puts", "putchar",
        "scanf", "fscanf", "sscanf", "getchar", "gets", "fgets",
        "malloc", "calloc", "realloc", "free",
        "fopen", "fclose", "fread", "fwrite", "fseek", "ftell",
        "exit", "abort", "system", "rand", "srand", "time"
    };

    if (unsafe_functions.find(func_name) != unsafe_functions.end()) {
        is_safe = false;
    }

    currentLoop->addDetectedFunctionCall(func_name, is_safe);

    return true;
}

void LoopVisitor::analyzeForLoopBounds(ForStmt* forLoop, LoopInfo& info) {
    info.bounds.init_expr = nullptr;
    info.bounds.condition_expr = forLoop->getCond();
    info.bounds.increment_expr = forLoop->getInc();

    // Extract iterator variable name
    if (auto* declStmt = dyn_cast_or_null<DeclStmt>(forLoop->getInit())) {
        if (declStmt->isSingleDecl()) {
            if (auto* varDecl = dyn_cast<VarDecl>(declStmt->getSingleDecl())) {
                info.bounds.iterator_var = varDecl->getNameAsString();
            }
        }
    }

    // Check for simple loop pattern
    if (!info.bounds.iterator_var.empty() && info.bounds.condition_expr &&
        info.bounds.increment_expr) {
        info.bounds.is_simple_pattern = true;
    }

    if (verbose_ && info.bounds.is_simple_pattern) {
        std::cout << "  Simple iterator pattern detected: "
                  << info.bounds.iterator_var << " (depth " << info.depth << ")\n";
    }
}

void LoopVisitor::markInductionVariable(LoopInfo& loop) {
    if (!loop.bounds.iterator_var.empty()) {
        auto it = loop.variables.find(loop.bounds.iterator_var);
        if (it != loop.variables.end()) {
            it->second.setRole(VariableRole::INDUCTION_VAR);
            if (verbose_) {
                std::cout << "  Marked " << loop.bounds.iterator_var
                          << " as induction variable (safe for parallelization)\n";
            }
        }
    }
}

void LoopVisitor::finalizeDependencyAnalysis(LoopInfo& loop) {
    if (verbose_) {
        printArrayAccessSummary();
    }

    line_access_summaries_.clear();

    if (dependency_analyzer_) {
        dependency_analyzer_->setVerbose(verbose_);
    }
    dependency_analyzer_->analyzeDependencies(loop);
    bool has_deps = dependency_analyzer_->hasDependencies(loop);
    loop.setHasDependencies(has_deps);
}

bool LoopVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr* arrayExpr) {
    if (!arrayExpr || !isInsideLoop()) {
        return true;
    }

    const std::string arrayName = extractArrayBaseName(arrayExpr);
    SourceLocation loc = arrayExpr->getExprLoc();
    SourceManager& sm = context_->getSourceManager();
    unsigned line = sm.getSpellingLineNumber(loc);

    // Check if this is a write by looking at parent context
    bool is_write = false;
    auto parents = context_->getParents(*arrayExpr);
    for (const auto& parent : parents) {
        if (auto* binaryOp = parent.get<BinaryOperator>()) {
            if (binaryOp->isAssignmentOp() && binaryOp->getLHS() == arrayExpr) {
                is_write = true;
                break;
            }
        }
    }

    ArrayAccess access(arrayName, arrayExpr->getIdx(), loc, line, is_write);
    getCurrentLoop()->addArrayAccess(access);

    // Collect for clean summary output
    if (verbose_) {
        std::string subscript_str = extractSubscriptString(arrayExpr->getIdx());
        std::string access_pattern = arrayName + "[" + subscript_str + "]";

        line_access_summaries_[line].line_number = line;
        line_access_summaries_[line].accesses.push_back({access_pattern, is_write});
    }

    return true;
}

std::string LoopVisitor::extractArrayBaseName(ArraySubscriptExpr* arrayExpr) {
    Expr* base = arrayExpr->getBase()->IgnoreParenImpCasts();

    // Handle multi-dimensional arrays
    while (auto* innerArray = dyn_cast<ArraySubscriptExpr>(base)) {
        base = innerArray->getBase()->IgnoreParenImpCasts();
    }

    if (auto* declRef = dyn_cast<DeclRefExpr>(base)) {
        return declRef->getDecl()->getNameAsString();
    }

    return "unknown";
}

std::string LoopVisitor::extractSubscriptString(Expr* idx) {
    if (!idx) return "unknown";

    idx = idx->IgnoreParenImpCasts();

    if (auto* declRef = dyn_cast<DeclRefExpr>(idx)) {
        return declRef->getDecl()->getNameAsString();
    } else if (auto* binOp = dyn_cast<BinaryOperator>(idx)) {
        if (auto* lhs = dyn_cast<DeclRefExpr>(binOp->getLHS())) {
            if (auto* rhs = dyn_cast<IntegerLiteral>(binOp->getRHS())) {
                std::string var = lhs->getDecl()->getNameAsString();
                int constant = rhs->getValue().getSExtValue();
                if (binOp->getOpcode() == BO_Add) {
                    return var + "+" + std::to_string(constant);
                } else if (binOp->getOpcode() == BO_Sub) {
                    return var + "-" + std::to_string(constant);
                }
            }
        }
        return "complex";
    } else if (auto* intLit = dyn_cast<IntegerLiteral>(idx)) {
        return std::to_string(intLit->getValue().getSExtValue());
    }

    return "unknown";
}

void LoopVisitor::printArrayAccessSummary() {
    if (!verbose_ || line_access_summaries_.empty()) {
        return;
    }

    for (const auto& [line_num, summary] : line_access_summaries_) {
        std::cout << "  Array accesses: ";
        for (size_t i = 0; i < summary.accesses.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << summary.accesses[i].first
                      << " (" << (summary.accesses[i].second ? "WRITE" : "READ") << ")";
        }
        std::cout << " at line " << line_num << "\n";
    }
}

bool LoopVisitor::isWriteAccess(DeclRefExpr* declRef) {
    auto parents = context_->getParents(*declRef);
    for (const auto& parent : parents) {
        if (auto* binaryOp = parent.get<BinaryOperator>()) {
            if (binaryOp->isAssignmentOp() && binaryOp->getLHS() == declRef) {
                return true;
            }
        } else if (auto* unaryOp = parent.get<UnaryOperator>()) {
            if (unaryOp->isIncrementDecrementOp()) {
                return true;
            }
        } else if (auto* compoundAssign = parent.get<CompoundAssignOperator>()) {
            if (compoundAssign->getLHS() == declRef) {
                return true;
            }
        }
    }
    return false;
}

bool LoopVisitor::isArithmeticOp(BinaryOperator* binOp) {
    return binOp->isAdditiveOp() || binOp->isMultiplicativeOp();
}

bool LoopVisitor::isComparisonOp(BinaryOperator* binOp) {
    return binOp->isComparisonOp();
}

VariableScope LoopVisitor::determineVariableScope(VarDecl* varDecl) const {
    if (!varDecl) {
        return VariableScope::GLOBAL;
    }
    
    SourceLocation declLoc = varDecl->getLocation();
    if (declLoc.isInvalid()) {
        return VariableScope::GLOBAL;
    }
    
    // If not analyzing any loop, variable is function-scoped
    if (loop_stack_.empty()) {
        return VariableScope::FUNCTION_LOCAL;
    }
    
    // Get current loop from stack (const-safe access)
    if (loop_stack_.empty()) {
        return VariableScope::FUNCTION_LOCAL;
    }
    LoopInfo* currentLoop = loop_stack_.top();
    if (!currentLoop || !currentLoop->stmt) {
        return VariableScope::FUNCTION_LOCAL;
    }
    
    // Get loop boundaries for comparison
    SourceLocation loopStart = currentLoop->stmt->getBeginLoc();
    SourceLocation loopEnd = currentLoop->stmt->getEndLoc();
    
    if (loopStart.isInvalid() || loopEnd.isInvalid()) {
        return VariableScope::FUNCTION_LOCAL;
    }
    
    SourceManager& sm = context_->getSourceManager();
    
    // Check if variable declared inside loop body
    if (sm.getFileID(declLoc) == sm.getFileID(loopStart)) {
        unsigned declOffset = sm.getFileOffset(declLoc);
        unsigned loopStartOffset = sm.getFileOffset(loopStart);
        unsigned loopEndOffset = sm.getFileOffset(loopEnd);
        
        if (declOffset > loopStartOffset && declOffset < loopEndOffset) {
            if (verbose_) {
                std::cout << "  Variable '" << varDecl->getNameAsString() 
                         << "' declared inside loop body -> LOOP_LOCAL\n";
            }
            return VariableScope::LOOP_LOCAL;
        }
    }
    
    // Handle for-loop induction variables (e.g., "for(int i = 0; ...)")
    if (auto* forLoop = dyn_cast<ForStmt>(currentLoop->stmt)) {
        if (auto* declStmt = dyn_cast_or_null<DeclStmt>(forLoop->getInit())) {
            if (declStmt->isSingleDecl() && declStmt->getSingleDecl() == varDecl) {
                if (verbose_) {
                    std::cout << "  Variable '" << varDecl->getNameAsString() 
                             << "' is for-loop induction variable -> LOOP_LOCAL\n";
                }
                return VariableScope::LOOP_LOCAL;
            }
        }
    }
    
    return VariableScope::FUNCTION_LOCAL;
}

void LoopVisitor::addLoop(Stmt* stmt, SourceLocation loc, const std::string& type) {
    if (!loc.isValid()) {
        std::cout << "Warning: Invalid source location for " << type << " loop\n";
        return;
    }

    SourceManager& sm = context_->getSourceManager();
    unsigned line = sm.getSpellingLineNumber(loc);
    loops_.emplace_back(stmt, loc, line, type);
    unsigned depth = static_cast<unsigned>(loop_stack_.size());

    if (verbose_) {
        std::cout << "Found " << type << " loop at line " << line << " (depth "
                  << depth << ")\n";
    }
}

void LoopVisitor::printLoopSummary() const {
    std::cout << "\n=== Loop Analysis Results ===\n";
    
    if (loops_.empty()) {
        std::cout << "No loops detected in the analyzed code.\n";
        std::cout << "============================\n";
        return;
    }

    // Count parallelizable loops
    size_t parallelizable_count = 0;
    for (const auto& loop : loops_) {
        if (loop.isParallelizable()) {
            parallelizable_count++;
        }
    }

    std::cout << "Found " << loops_.size() << " loop" << (loops_.size() > 1 ? "s" : "") 
              << ", " << parallelizable_count << " parallelizable\n\n";

    std::cout << "┌─────┬──────┬───────────┬─────────────────┬──────────────────────────┐\n";
    std::cout << "│ ID  │ Line │ Type      │ Status          │ Reason                   │\n";
    std::cout << "├─────┼──────┼───────────┼─────────────────┼──────────────────────────┤\n";

    for (size_t i = 0; i < loops_.size(); i++) {
        const auto& loop = loops_[i];
        
        // Format ID
        std::cout << "│ L" << (i + 1);
        if (i + 1 < 10) std::cout << "  │";
        else std::cout << " │";

        // Format line number
        std::cout << " " << std::setw(4) << loop.line_number << " │";

        // Format type
        std::string type = loop.loop_type;
        std::cout << " " << std::setw(9) << std::left << type << " │";

        // Determine status and reasoning
        std::string status, reason;
        if (loop.isParallelizable()) {
            status = "SAFE";
            
            if (loop.bounds.is_simple_pattern && !loop.array_accesses.empty()) {
                reason = "Simple array operations";
            } else if (loop.bounds.is_simple_pattern) {
                reason = "Simple iterator pattern";
            } else {
                reason = "No dependencies";
            }
        } else {
            status = "UNSAFE";
            
            if (loop.hasUnsafeFunctionCalls()) {
                reason = "Function call side effects";
            } else if (loop.has_dependencies) {
                reason = "Loop-carried dependency";
            } else {
                reason = "Complex dependencies";
            }
        }

        // Format status
        std::cout << " " << std::setw(15) << std::left << status << " │";
        
        // Format reason (truncate if too long)
        if (reason.length() > 24) {
            reason = reason.substr(0, 21) + "...";
        }
        std::cout << " " << std::setw(24) << std::left << reason << " │\n";
    }

    // Table footer
    std::cout << "└─────┴──────┴───────────┴─────────────────┴──────────────────────────┘\n";

    // Summary
    std::cout << "\nSummary:\n";
    std::cout << "  Parallelizable: " << parallelizable_count << "/" << loops_.size()
              << " (" << (loops_.size() > 0 ? (parallelizable_count * 100 / loops_.size()) : 0) << "%)\n";
    
    std::cout << "============================\n";
}

} // namespace statik