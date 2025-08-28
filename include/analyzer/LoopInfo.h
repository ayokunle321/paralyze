#pragma once

#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "analyzer/ArrayAccess.h"
#include "analyzer/LoopBounds.h"
#include "analyzer/VariableInfo.h"
#include "analyzer/LoopMetrics.h"
#include <vector>
#include <map>
#include <optional>

namespace statik {

struct LoopInfo {
    clang::Stmt* stmt;
    clang::SourceLocation location;
    unsigned line_number;
    std::string loop_type; // "for", "while", "do-while"
    
    // Nesting information - NOW USING INDICES INSTEAD OF POINTERS
    unsigned depth; // 0 = outermost, 1 = nested once, etc.
    std::optional<size_t> parent_loop_index; // Index into loops_ vector, nullopt if outermost
    std::vector<size_t> child_loop_indices; // Indices of child loops
    
    // Array access patterns within this loop
    std::vector<ArrayAccess> array_accesses;
    
    // Loop bounds information
    LoopBounds bounds;
    
    // Variable usage tracking
    std::map<std::string, VariableInfo> variables;
    
    // Performance metrics
    LoopMetrics metrics;
    
    // Function call information
    std::vector<std::string> detected_function_calls;  // Store function names
    std::vector<bool> function_call_safety;           // Store safety flags
    
    // Dependency information - simplified to just a boolean
    bool has_dependencies;
    
    LoopInfo(clang::Stmt* s, clang::SourceLocation loc, unsigned line,
             const std::string& type)
        : stmt(s), location(loc), line_number(line), loop_type(type), depth(0),
          parent_loop_index(std::nullopt), has_dependencies(false) {}
          
    void addArrayAccess(const ArrayAccess& access) {
        array_accesses.push_back(access);
        metrics.memory_accesses++; // Count array access as memory operation
    }
    
    void setParent(size_t parent_index, unsigned parent_depth) {
        parent_loop_index = parent_index;
        depth = parent_depth + 1;
    }
    
    void addChildLoop(size_t child_index) {
        child_loop_indices.push_back(child_index);
    }
    
    void addVariable(const VariableInfo& var_info) {
        auto it = variables.find(var_info.name);
        if (it == variables.end()) {
            variables.emplace(var_info.name, var_info);
        }
    }
    
    void addVariableUsage(const std::string& var_name, const VariableUsage& usage) {
        auto it = variables.find(var_name);
        if (it != variables.end()) {
            it->second.addUsage(usage);
        }
    }
    
    void incrementArithmeticOps() { metrics.arithmetic_ops++; }
    void incrementFunctionCalls() { metrics.function_calls++; }
    void incrementComparisons() { metrics.comparisons++; }
    void incrementAssignments() { metrics.assignments++; }
    
    void finalizeMetrics() {
        metrics.calculateHotness();
    }
    
    void setHasDependencies(bool deps) {
        has_dependencies = deps;
    }
    
    void addDetectedFunctionCall(const std::string& func_name, bool is_safe) {
        detected_function_calls.push_back(func_name);
        function_call_safety.push_back(is_safe);
    }
    
    bool hasUnsafeFunctionCalls() const {
        for (bool is_safe : function_call_safety) {
            if (!is_safe) return true;
        }
        return false;
    }
    
    // Recursively check if this loop or any child loops have unsafe function calls
    bool hasUnsafeCallsRecursive(const std::vector<LoopInfo>& all_loops) const {
        // Check this loop's own function calls
        if (hasUnsafeFunctionCalls()) {
            return true;
        }
        
        // Check all child loops recursively
        for (size_t child_idx : child_loop_indices) {
            if (child_idx < all_loops.size()) {
                if (all_loops[child_idx].hasUnsafeCallsRecursive(all_loops)) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    bool isOutermost() const { return depth == 0; }
    bool isHot() const { return metrics.hotness_score > 10.0; }
    bool isParallelizable() const { return !has_dependencies; }
    
    bool hasParent() const { return parent_loop_index.has_value(); }
};

} // namespace statik