#pragma once

#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include <string>

namespace statik {

// captures loop bounds and iteration info
struct LoopBounds {
    std::string iterator_var; 
    clang::Stmt* init_expr;     
    clang::Expr* condition_expr;   
    clang::Expr* increment_expr;   
    bool is_simple_pattern;        
    
    LoopBounds() : init_expr(nullptr), condition_expr(nullptr),
                   increment_expr(nullptr), is_simple_pattern(false) {}
};

} // namespace statik
