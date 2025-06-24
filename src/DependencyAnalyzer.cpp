#include "analyzer/DependencyAnalyzer.h"
#include <iostream>

using namespace clang;

namespace statik {

void DependencyAnalyzer::analyzeDependencies(LoopInfo& loop) {
  std::cout << "  Analyzing dependencies for loop at line "
           << loop.line_number << "\n";
  
  bool has_scalar_deps = false;
  bool has_array_deps = false;
  bool has_pointer_risk = false;
  bool has_unsafe_calls = false;
  
  try {
    // Analyze scalar variable dependencies
    analyzeScalarDependencies(loop);
    
    // Analyze array access dependencies
    array_analyzer_->analyzeArrayDependencies(loop);
    has_array_deps = array_analyzer_->hasArrayDependencies(loop);
  } catch (...) {
    std::cout << "  Warning: Array dependency analysis failed - assuming unsafe\n";
    has_array_deps = true;
  }
  
  try {
    // Analyze pointer usage and aliasing
    pointer_analyzer_->analyzePointerUsage(loop);
    has_pointer_risk = (pointer_analyzer_->getPointerRisk(loop) != PointerRisk::SAFE);
  } catch (...) {
    std::cout << "  Warning: Pointer analysis failed - assuming unsafe\n";
    has_pointer_risk = true;
  }
  
  try {
    // Analyze function calls for side effects
    function_analyzer_->analyzeFunctionCalls(loop);
    has_unsafe_calls = (function_analyzer_->getFunctionCallSafety(loop) == FunctionCallSafety::UNSAFE);
  } catch (...) {
    std::cout << "  Warning: Function call analysis failed - assuming unsafe\n";
    has_unsafe_calls = true;
  }
  
  // Check scalar dependencies
  for (const auto& var_pair : loop.variables) {
    const auto& var = var_pair.second;
    if (var.isInductionVariable()) {
      continue;
    }
    if (var.hasReads() && var.hasWrites()) {
      has_scalar_deps = true;
      break;
    }
  }
  
  if (has_scalar_deps || has_array_deps || has_pointer_risk || has_unsafe_calls) {
    std::cout << "  Dependencies found - not safe for parallelization\n";
    loop.setHasDependencies(true);
  } else {
    std::cout << "  No dependencies detected - safe for parallelization\n";
    loop.setHasDependencies(false);
  }
}

bool DependencyAnalyzer::hasDependencies(const LoopInfo& loop) const {
  // Check scalar dependencies
  for (const auto& var_pair : loop.variables) {
    const auto& var = var_pair.second;
    // Skip induction variables
    if (var.isInductionVariable()) {
      continue;
    }
    // Check for read-after-write patterns that could be loop-carried
    if (var.hasReads() && var.hasWrites()) {
      return true; // Conservative: assume any RW pattern is problematic
    }
  }
  
  // Check array dependencies
  return array_analyzer_->hasArrayDependencies(loop) || 
         (pointer_analyzer_->getPointerRisk(loop) != PointerRisk::SAFE) ||
         (function_analyzer_->getFunctionCallSafety(loop) == FunctionCallSafety::UNSAFE);
}

void DependencyAnalyzer::analyzeScalarDependencies(LoopInfo& loop) {
  for (auto& var_pair : loop.variables) {
    const std::string& var_name = var_pair.first;
    const VariableInfo& var_info = var_pair.second;
    
    // Skip induction variables
    if (var_info.isInductionVariable()) {
      continue;
    }
    
    checkVariableForDependency(var_name, var_info, loop);
  }
}

void DependencyAnalyzer::checkVariableForDependency(const std::string& var_name,
                                                   const VariableInfo& var_info,
                                                   LoopInfo& loop) {
  // Simple analysis: look for variables that are both read and written
  if (!var_info.hasReads() || !var_info.hasWrites()) {
    return; // No dependency if only reading or only writing
  }
  
  // Find all write operations
  std::vector<VariableUsage> writes;
  std::vector<VariableUsage> reads;
  
  for (const auto& usage : var_info.usages) {
    if (usage.is_write) {
      writes.push_back(usage);
    } else if (usage.is_read) {
      reads.push_back(usage);
    }
  }
  
  // Check for potential loop-carried dependencies
  for (const auto& write : writes) {
    for (const auto& read : reads) {
      if (isLoopCarriedDependency(write, read)) {
        std::string desc = "Scalar variable " + var_name +
                          " written at line " + std::to_string(write.line_number) +
                          " and read at line " + std::to_string(read.line_number);
        
        Dependency dep(var_name, DependencyType::LOOP_CARRIED,
                      write.line_number, read.line_number, desc);
        
        // For now, just print the dependency
        std::cout << "  Found scalar dependency: " << desc << "\n";
      }
    }
  }
}

bool DependencyAnalyzer::isLoopCarriedDependency(const VariableUsage& write_usage,
                                                const VariableUsage& read_usage) const {
  // Simple heuristic for now: if a variable is both read and written in the loop,
  // assume it's a potential loop-carried dependency
  // This is conservative but safe
  
  // More sophisticated analysis would check if the read could access
  // a value written in a previous iteration
  return true; // Conservative assumption
}

} // namespace statik