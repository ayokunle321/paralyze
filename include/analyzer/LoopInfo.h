#pragma once

#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include <vector>

namespace statik {

struct LoopInfo {
    clang::Stmt* stmt;
    clang::SourceLocation location;
    unsigned line_number;
    std::string loop_type;  // "for", "while", "do-while"
    
    LoopInfo(clang::Stmt* s, clang::SourceLocation loc, 
             unsigned line, const std::string& type)
        : stmt(s), location(loc), line_number(line), loop_type(type) {}
};

} // namespace statik