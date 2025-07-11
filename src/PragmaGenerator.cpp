#include "analyzer/PragmaGenerator.h"
#include <iostream>
#include <algorithm>

namespace statik {

void PragmaGenerator::generatePragmasForLoops(const std::vector<LoopInfo>& loops) {
  generated_pragmas_.clear();
  
  std::cout << "\n=== Generating OpenMP Pragmas ===\n";
  
  for (const auto& loop : loops) {
    PragmaType pragma_type = determinePragmaType(loop);
    
    if (pragma_type != PragmaType::NO_PRAGMA) {
      std::string pragma_text = generatePragmaText(pragma_type, loop);
      std::string reasoning = generateReasoning(pragma_type, loop);
      
      GeneratedPragma pragma(pragma_type, pragma_text, loop.loop_type, 
                           loop.line_number, reasoning);
      
      // Check if we need private variable clauses
      std::vector<std::string> private_vars = identifyPrivateVariables(loop);
      if (!private_vars.empty()) {
        pragma.requires_private_vars = true;
        pragma.private_variables = private_vars;
        // Add private clause to pragma text
        pragma.pragma_text += " private(";
        for (size_t i = 0; i < private_vars.size(); i++) {
          if (i > 0) pragma.pragma_text += ", ";
          pragma.pragma_text += private_vars[i];
        }
        pragma.pragma_text += ")";
      }
      
      // Calculate confidence score
      pragma.confidence = confidence_scorer_->calculateConfidence(loop, pragma);
      
      generated_pragmas_.push_back(pragma);
      
      std::cout << "  Generated pragma for " << loop.loop_type 
               << " loop at line " << loop.line_number << ":\n";
      std::cout << "    " << pragma.pragma_text << "\n";
      std::cout << "    Reasoning: " << reasoning << "\n";
      std::cout << "    Confidence: " << confidence_scorer_->getConfidenceDescription(pragma.confidence.level)
               << " (" << static_cast<int>(pragma.confidence.numerical_score * 100) << "%)\n";
      std::cout << "    " << pragma.confidence.reasoning << "\n";
    } else {
      std::cout << "  No pragma generated for " << loop.loop_type 
               << " loop at line " << loop.line_number 
               << " (has dependencies)\n";
    }
  }
  
  std::cout << "===============================\n";
}

PragmaType PragmaGenerator::determinePragmaType(const LoopInfo& loop) {
  // Only generate pragmas for parallelizable loops
  if (loop.has_dependencies) {
    return PragmaType::NO_PRAGMA;
  }
  
  // For nested loops, be more conservative
  if (loop.depth > 0) {
    // Inner loops with simple array access might benefit from SIMD
    if (shouldUseSimd(loop)) {
      return PragmaType::SIMD;
    }
    return PragmaType::NO_PRAGMA;  // Conservative for nested loops
  }
  
  // For outermost loops, check if SIMD is beneficial
  if (shouldUseSimd(loop)) {
    return PragmaType::PARALLEL_FOR_SIMD;
  }
  
  // Default to parallel for
  return PragmaType::PARALLEL_FOR;
}

std::string PragmaGenerator::generatePragmaText(PragmaType type, const LoopInfo& loop) {
  switch (type) {
    case PragmaType::PARALLEL_FOR:
      return "#pragma omp parallel for";
    case PragmaType::PARALLEL_FOR_SIMD:
      return "#pragma omp parallel for simd";
    case PragmaType::SIMD:
      return "#pragma omp simd";
    case PragmaType::NO_PRAGMA:
    default:
      return "";
  }
}

std::string PragmaGenerator::generateReasoning(PragmaType type, const LoopInfo& loop) {
  std::string reason;
  
  switch (type) {
    case PragmaType::PARALLEL_FOR:
      reason = "Loop has no dependencies and good parallelization potential";
      break;
    case PragmaType::PARALLEL_FOR_SIMD:
      reason = "Loop has simple array operations suitable for both parallelization and vectorization";
      break;
    case PragmaType::SIMD:
      reason = "Inner loop with simple operations suitable for vectorization";
      break;
    case PragmaType::NO_PRAGMA:
    default:
      reason = "Loop has dependencies or is not suitable for parallelization";
      break;
  }
  
  // Add specific details about why this loop is good for parallelization
  if (type != PragmaType::NO_PRAGMA) {
    if (loop.bounds.is_simple_pattern) {
      reason += " (simple iterator pattern)";
    }
    if (loop.isHot()) {
      reason += " (high computational intensity)";
    }
  }
  
  return reason;
}

bool PragmaGenerator::shouldUseSimd(const LoopInfo& loop) {
  // SIMD is beneficial for loops with:
  // 1. Simple array access patterns
  // 2. Arithmetic operations
  // 3. No function calls (or only math functions)
  
  if (!hasSimpleArrayAccess(loop)) {
    return false;
  }
  
  // Check if loop has primarily arithmetic operations
  if (loop.metrics.arithmetic_ops > loop.metrics.function_calls * 2) {
    return true;
  }
  
  // For inner loops, be more aggressive with SIMD
  if (isInnerLoop(loop) && loop.metrics.memory_accesses > 0) {
    return true;
  }
  
  return false;
}

bool PragmaGenerator::hasSimpleArrayAccess(const LoopInfo& loop) {
  // Check if array accesses use simple patterns like A[i], B[i]
  for (const auto& access : loop.array_accesses) {
    // For now, assume most array accesses in parallelizable loops are simple
    // A more sophisticated check would analyze the subscript expressions
  }
  
  // If we have array accesses and the loop is parallelizable, 
  // assume they're simple enough
  return !loop.array_accesses.empty();
}

bool PragmaGenerator::isInnerLoop(const LoopInfo& loop) {
  // An inner loop has children but is not at depth 0
  return loop.depth > 0;
}

std::vector<std::string> PragmaGenerator::identifyPrivateVariables(const LoopInfo& loop) {
  std::vector<std::string> private_vars;
  
  for (const auto& var_pair : loop.variables) {
    const auto& var = var_pair.second;
    
    // Skip induction variables - they're automatically private
    if (var.isInductionVariable()) {
      continue;
    }
    
    // Variables that are only written or declared inside the loop
    // should be private
    if (var.scope == VariableScope::LOOP_LOCAL && var.hasWrites()) {
      private_vars.push_back(var.name);
    }
  }
  
  return private_vars;
}

void PragmaGenerator::printPragmaSummary() const {
  std::cout << "\n=== Pragma Generation Summary ===\n";
  std::cout << "Total pragmas generated: " << generated_pragmas_.size() << "\n\n";
  
  int parallel_for_count = 0;
  int parallel_for_simd_count = 0;
  int simd_count = 0;
  double avg_confidence = 0.0;
  
  for (const auto& pragma : generated_pragmas_) {
    switch (pragma.type) {
      case PragmaType::PARALLEL_FOR:
        parallel_for_count++;
        break;
      case PragmaType::PARALLEL_FOR_SIMD:
        parallel_for_simd_count++;
        break;
      case PragmaType::SIMD:
        simd_count++;
        break;
      default:
        break;
    }
    
    avg_confidence += pragma.confidence.numerical_score;
    
    std::cout << "Line " << pragma.line_number << ": " << pragma.pragma_text;
    if (pragma.requires_private_vars) {
      std::cout << " (with private variables)";
    }
    std::cout << " [Confidence: " 
             << confidence_scorer_->getConfidenceDescription(pragma.confidence.level) << "]\n";
  }
  
  if (!generated_pragmas_.empty()) {
    avg_confidence /= generated_pragmas_.size();
  }
  
  std::cout << "\nBreakdown:\n";
  std::cout << "  #pragma omp parallel for: " << parallel_for_count << "\n";
  std::cout << "  #pragma omp parallel for simd: " << parallel_for_simd_count << "\n";
  std::cout << "  #pragma omp simd: " << simd_count << "\n";
  std::cout << "  Average confidence: " << static_cast<int>(avg_confidence * 100) << "%\n";
  std::cout << "================================\n";
}

} // namespace statik