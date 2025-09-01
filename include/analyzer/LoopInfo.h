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

// holds info about a single loop
struct LoopInfo {
    clang::Stmt* stmt;
    clang::SourceLocation location;
    unsigned line_number;
    std::string loop_type; // for, while, or do-while

    // nesting
    unsigned depth = 0;
    std::optional<size_t> parent_loop_index;
    std::vector<size_t> child_loop_indices;

    std::vector<ArrayAccess> array_accesses;
    LoopBounds bounds;
    std::map<std::string, VariableInfo> variables;
    
    LoopMetrics metrics; // performance metrics

    // function calls
    std::vector<std::string> detected_function_calls;
    std::vector<bool> function_call_safety;
    bool has_dependencies = false;

    LoopInfo(clang::Stmt* s, clang::SourceLocation loc, unsigned line,
             const std::string& type)
        : stmt(s), location(loc), line_number(line), loop_type(type) {}

    // helpers
    void addArrayAccess(const ArrayAccess& access) {
        array_accesses.push_back(access);
        metrics.memory_accesses++;
    }

    void setParent(size_t parent_index, unsigned parent_depth) {
        parent_loop_index = parent_index;
        depth = parent_depth + 1;
    }

    void addChildLoop(size_t child_index) {
        child_loop_indices.push_back(child_index);
    }

    void addVariable(const VariableInfo& var_info) {
        if (variables.find(var_info.name) == variables.end()) {
            variables.emplace(var_info.name, var_info);
        }
    }

    void addVariableUsage(const std::string& var_name, const VariableUsage& usage) {
        auto it = variables.find(var_name);
        if (it != variables.end()) it->second.addUsage(usage);
    }

    void incrementArithmeticOps() { metrics.arithmetic_ops++; }
    void incrementFunctionCalls() { metrics.function_calls++; }
    void incrementComparisons() { metrics.comparisons++; }
    void incrementAssignments() { metrics.assignments++; }

    void finalizeMetrics() { metrics.calculateHotness(); }
    void setHasDependencies(bool deps) { has_dependencies = deps; }

    void addDetectedFunctionCall(const std::string& func_name, bool is_safe) {
        detected_function_calls.push_back(func_name);
        function_call_safety.push_back(is_safe);
    }

    bool hasUnsafeFunctionCalls() const {
        for (bool safe : function_call_safety) if (!safe) return true;
        return false;
    }

    bool hasUnsafeCallsRecursive(const std::vector<LoopInfo>& all_loops) const {
        if (hasUnsafeFunctionCalls()) return true;
        for (size_t child_idx : child_loop_indices) {
            if (child_idx < all_loops.size() &&
                all_loops[child_idx].hasUnsafeCallsRecursive(all_loops)) {
                return true;
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