#pragma once

#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "analyzer/ArrayAccess.h"
#include "analyzer/LoopBounds.h"
#include <vector>

namespace statik {

struct LoopInfo {
    clang::Stmt* stmt;
    clang::SourceLocation location;
    unsigned line_number;
    std::string loop_type;  // "for", "while", "do-while"
    
    // Array access patterns within this loop
    std::vector<ArrayAccess> array_accesses;
    
    // Loop bounds information
    LoopBounds bounds;
    
    LoopInfo(clang::Stmt* s, clang::SourceLocation loc, 
             unsigned line, const std::string& type)
        : stmt(s), location(loc), line_number(line), loop_type(type) {}
        
    void addArrayAccess(const ArrayAccess& access) {
        array_accesses.push_back(access);
    }
};

} // namespace statik