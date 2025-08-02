#pragma once

#include "clang/AST/Expr.h"
#include "clang/Basic/SourceLocation.h"
#include <string>

namespace statik {

struct ArrayAccess {
    std::string array_name;
    clang::Expr* subscript;
    clang::SourceLocation location;
    unsigned line_number;
    bool is_write;
    
    ArrayAccess(const std::string& name, clang::Expr* sub, 
                clang::SourceLocation loc, unsigned line, bool write)
        : array_name(name), subscript(sub), location(loc), 
          line_number(line), is_write(write) {}
};

} // namespace stati