#pragma once

#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include <string>
#include <vector>
#include <set>

namespace statik {

enum class PointerRisk {
    SAFE,            // no pointer operations
    POTENTIAL_ALIAS, // pointer dereferences that might alias
    UNSAFE           // complex arithmetic
};

// record of a single pointer operation
struct PointerOperation {
    std::string pointer_name;
    clang::SourceLocation location;
    unsigned line_number;
    bool is_dereference; // *ptr
    bool is_address_of;  // &var
    bool is_arithmetic;  // ptr + 1, ptr++

    PointerOperation(const std::string& name, clang::SourceLocation loc, 
                     unsigned line, bool deref, bool addr, bool arith)
        : pointer_name(name), location(loc), line_number(line),
          is_dereference(deref), is_address_of(addr), is_arithmetic(arith) {}
};

class PointerAnalyzer {
public:
    explicit PointerAnalyzer(clang::ASTContext* context) 
        : context_(context), verbose_(false) {}

    void analyzePointerUsage(LoopInfo& loop);
    PointerRisk getPointerRisk(const LoopInfo& loop) const;
    void setVerbose(bool verbose) { verbose_ = verbose; }

    // visit pointer-related expressions
    void visitUnaryOperator(clang::UnaryOperator* unaryOp, LoopInfo& loop);
    void visitBinaryOperator(clang::BinaryOperator* binOp, LoopInfo& loop);
    void visitMemberExpr(clang::MemberExpr* memberExpr, LoopInfo& loop);

private:
    clang::ASTContext* context_;
    bool verbose_;
    std::vector<PointerOperation> pointer_ops_;
    std::set<std::string> detected_pointers_;

    bool isPointerType(clang::QualType type);
    std::string extractPointerName(clang::Expr* expr);
    std::string extractPointerNameRecursive(clang::Expr* expr, int depth);
    void recordPointerOperation(const std::string& name, clang::SourceLocation loc,
                                bool deref, bool addr, bool arith);
    bool hasComplexPointerArithmetic() const;
    bool hasMultiplePointerDereferences() const;
};

} // namespace statik
