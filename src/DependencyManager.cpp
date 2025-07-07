#include "analyzer/DependencyManager.h"
#include <iostream>

using namespace clang;

namespace statik {

DependencyManager::DependencyManager(ASTContext* context) 
    : context_(context),
      array_analyzer_(std::make_unique<ArrayDependencyAnalyzer>(context)),
      pointer_analyzer_(std::make_unique<PointerAnalyzer>(context)),
      function_analyzer_(std::make_unique<FunctionCallAnalyzer>(context)),
      location_mapper_(std::make_unique<PragmaLocationMapper>(&context->getSourceManager())),
      pragma_generator_(std::make_unique<PragmaGenerator>()),
      source_annotator_(std::make_unique<SourceAnnotator>(&context->getSourceManager())) {
}

void DependencyManager::analyzeLoop(LoopInfo& loop) {
  warnings_.clear();
  
  std::cout << "=== Dependency Analysis for Loop at Line " << loop.line_number << " ===\n";
  
  try {
    runScalarAnalysis(loop);
    runArrayAnalysis(loop);
    runPointerAnalysis(loop);
    runFunctionAnalysis(loop);
    
    // Set final parallelization decision
    bool is_safe = isLoopParallelizable(loop);
    loop.setHasDependencies(!is_safe);
    
    std::cout << "\n--- Final Decision ---\n";
    if (is_safe) {
      std::cout << "Loop is SAFE for parallelization\n";
    } else {
      std::cout << "Loop is UNSAFE for parallelization\n";
      if (!warnings_.empty()) {
        std::cout << "Reasons:\n";
        for (const auto& warning : warnings_) {
          std::cout << "  - " << warning << "\n";
        }
      }
    }
    std::cout << "========================\n\n";
    
  } catch (const std::exception& e) {
    recordWarning("Analysis failed with exception: " + std::string(e.what()));
    loop.setHasDependencies(true);  // Conservative fallback
  }
}

bool DependencyManager::isLoopParallelizable(const LoopInfo& loop) const {
  // A loop is parallelizable if it has no dependencies from any analyzer
  return !hasScalarDependencies(loop) &&
         !array_analyzer_->hasArrayDependencies(loop) &&
         (pointer_analyzer_->getPointerRisk(loop) == PointerRisk::SAFE) &&
         (function_analyzer_->getFunctionCallSafety(loop) != FunctionCallSafety::UNSAFE);
}

void DependencyManager::runScalarAnalysis(LoopInfo& loop) {
  std::cout << "\n--- Scalar Variable Analysis ---\n";
  
  bool found_scalar_deps = false;
  
  for (const auto& var_pair : loop.variables) {
    const auto& var = var_pair.second;
    
    if (var.isInductionVariable()) {
      std::cout << "  " << var.name << ": INDUCTION VARIABLE (safe)\n";
      continue;
    }
    
    if (var.hasReads() && var.hasWrites()) {
      std::cout << "  " << var.name << ": READ+WRITE dependency detected\n";
      recordWarning("Scalar variable '" + var.name + "' has read-after-write dependency");
      found_scalar_deps = true;
    } else if (var.hasWrites()) {
      std::cout << "  " << var.name << ": WRITE-ONLY (safe)\n";
    } else if (var.hasReads()) {
      std::cout << "  " << var.name << ": READ-ONLY (safe)\n";
    }
  }
  
  if (!found_scalar_deps) {
    std::cout << "  No scalar dependencies detected\n";
  }
}

void DependencyManager::runArrayAnalysis(LoopInfo& loop) {
  std::cout << "\n--- Array Dependency Analysis ---\n";
  
  try {
    array_analyzer_->analyzeArrayDependencies(loop);
    
    if (array_analyzer_->hasArrayDependencies(loop)) {
      recordWarning("Array access conflicts detected");
    }
  } catch (...) {
    recordWarning("Array analysis failed - assuming unsafe");
  }
}

void DependencyManager::runPointerAnalysis(LoopInfo& loop) {
  std::cout << "\n--- Pointer Analysis ---\n";
  
  try {
    pointer_analyzer_->analyzePointerUsage(loop);
    
    PointerRisk risk = pointer_analyzer_->getPointerRisk(loop);
    switch (risk) {
      case PointerRisk::POTENTIAL_ALIAS:
        recordWarning("Potential pointer aliasing detected");
        break;
      case PointerRisk::UNSAFE:
        recordWarning("Complex pointer operations detected");
        break;
      case PointerRisk::SAFE:
        // No warning needed
        break;
    }
  } catch (...) {
    recordWarning("Pointer analysis failed - assuming unsafe");
  }
}

void DependencyManager::runFunctionAnalysis(LoopInfo& loop) {
  std::cout << "\n--- Function Call Analysis ---\n";
  
  try {
    function_analyzer_->analyzeFunctionCalls(loop);
    
    FunctionCallSafety safety = function_analyzer_->getFunctionCallSafety(loop);
    switch (safety) {
      case FunctionCallSafety::UNSAFE:
        recordWarning("Function calls with side effects detected");
        break;
      case FunctionCallSafety::POTENTIALLY_SAFE:
        std::cout << "  Math functions detected (potentially safe)\n";
        break;
      case FunctionCallSafety::SAFE:
        // No warning needed
        break;
    }
  } catch (...) {
    recordWarning("Function analysis failed - assuming unsafe");
  }
}

void DependencyManager::recordWarning(const std::string& warning) {
  warnings_.push_back(warning);
}

bool DependencyManager::hasScalarDependencies(const LoopInfo& loop) const {
  for (const auto& var_pair : loop.variables) {
    const auto& var = var_pair.second;
    
    if (var.isInductionVariable()) {
      continue;
    }
    
    if (var.hasReads() && var.hasWrites()) {
      return true;
    }
  }
  
  return false;
}

void DependencyManager::mapPragmaLocations(const std::vector<LoopInfo>& loops) {
  location_mapper_->clearInsertionPoints();
  
  std::cout << "\n=== Mapping Pragma Insertion Points ===\n";
  
  for (const auto& loop : loops) {
    // Only map locations for parallelizable loops
    if (isLoopParallelizable(loop)) {
      location_mapper_->mapLoopToPragmaLocation(loop);
    } else {
      std::cout << "  Skipping unsafe loop at line " << loop.line_number << "\n";
    }
  }
  
  const auto& points = location_mapper_->getInsertionPoints();
  std::cout << "  Total pragma insertion points identified: " << points.size() << "\n";
  std::cout << "==============================\n";
}

void DependencyManager::generatePragmas(const std::vector<LoopInfo>& loops) {
  pragma_generator_->generatePragmasForLoops(loops);
  pragma_generator_->printPragmaSummary();
}

void DependencyManager::annotateSourceFile(const std::string& input_filename, 
                                          const std::string& output_filename) {
  const auto& pragmas = pragma_generator_->getGeneratedPragmas();
  const auto& insertion_points = location_mapper_->getInsertionPoints();
  
  source_annotator_->annotateSourceWithPragmas(input_filename, pragmas, insertion_points);
  
  if (source_annotator_->writeAnnotatedFile(output_filename)) {
    source_annotator_->printAnnotationSummary();
  }
}

} // namespace statik