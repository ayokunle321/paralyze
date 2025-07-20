#pragma once

#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include <string>

namespace statik {

struct LoopBounds {
    std::string iterator_var; // "i", "j"
    clang::Stmt* init_expr; // initialization statement (can be DeclStmt or Expr)
    clang::Expr* condition_expr; // loop condition (i < N)
    clang::Expr* increment_expr; // increment (i++, ++i, i+=1)
    bool is_simple_pattern; // true if this is a simple i=0; i<N; i++ pattern
    
    LoopBounds() : init_expr(nullptr), condition_expr(nullptr),
                   increment_expr(nullptr), is_simple_pattern(false) {}
};

} // namespace stati