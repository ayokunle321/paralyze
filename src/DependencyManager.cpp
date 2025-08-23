#include "analyzer/DependencyManager.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/ArrayDependencyAnalyzer.h"
#include "analyzer/PointerAnalyzer.h"
#include "analyzer/FunctionCallAnalyzer.h"
#include "analyzer/PragmaLocationMapper.h"
#include "analyzer/PragmaGenerator.h"
#include "analyzer/SourceAnnotator.h"
#include <iostream>
#include <stdexcept>

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
    
    if (verbose_) {
        std::cout << "\n=== Dependency Analysis for Loop at Line " << loop.line_number << " ===\n";
    }

    try {
        runScalarAnalysis(loop);
        runArrayAnalysis(loop);
        runPointerAnalysis(loop);
        runFunctionAnalysis(loop);

        // Set final parallelization decision
        bool is_safe = isLoopParallelizable(loop);
        loop.setHasDependencies(!is_safe);

        if (verbose_) {
            std::cout << "\n--- Final Decision ---\n";
            if (is_safe) {
                std::cout << "Loop is SAFE for parallelization\n";
            } else {
                std::cout << "Loop is UNSAFE for parallelization\n";
                if (!warnings_.empty()) {
                    std::cout << "Blocking factors:\n";
                    for (const auto& warning : warnings_) {
                        std::cout << "  • " << warning << "\n";
                    }
                }
            }
            std::cout << "======================================================\n";
        }
    } catch (const std::exception& e) {
        recordWarning("Analysis failed with exception: " + std::string(e.what()));
        loop.setHasDependencies(true); // Conservative fallback
        
        if (verbose_) {
            std::cout << "Analysis failed: " << e.what() << "\n";
        }
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
    if (verbose_) {
        std::cout << "\n--- Scalar Variable Analysis ---\n";
    }
    
    bool found_scalar_deps = false;
    
    for (const auto& var_pair : loop.variables) {
        const auto& var = var_pair.second;

        // Skip induction variables as OpenMP handles them automatically
        if (var.isInductionVariable()) {
            if (verbose_) {
                std::cout << "  " << var.name << ": INDUCTION VARIABLE (safe)\n";
            }
            continue;
        }

        // Check for read-after-write dependencies
        if (var.hasReads() && var.hasWrites()) {
            if (var.scope == VariableScope::LOOP_LOCAL) {
                if (verbose_) {
                    std::cout << "  " << var.name << ": LOCAL VARIABLE (safe)\n";
                }
            } else {
                if (verbose_) {
                    std::cout << "  " << var.name << ": READ+WRITE dependency (unsafe)\n";
                }
                recordWarning("Scalar variable '" + var.name + "' has read-after-write dependency");
                found_scalar_deps = true;
            }
        } else if (var.hasWrites()) {
            if (verbose_) {
                std::cout << "  " << var.name << ": WRITE-ONLY (safe)\n";
            }
        } else if (var.hasReads()) {
            if (verbose_) {
                std::cout << "  " << var.name << ": READ-ONLY (safe)\n";
            }
        }
    }

    if (verbose_ && !found_scalar_deps) {
        std::cout << "  No scalar dependencies detected\n";
    }
}

void DependencyManager::runArrayAnalysis(LoopInfo& loop) {
    if (verbose_) {
        std::cout << "\n--- Array Dependency Analysis ---\n";
    }

    try {
        array_analyzer_->setVerbose(verbose_);
        array_analyzer_->analyzeArrayDependencies(loop);
        
        if (array_analyzer_->hasArrayDependencies(loop)) {
            recordWarning("Array access conflicts detected");
            if (verbose_) {
                std::cout << "  Array dependencies found\n";
            }
        } else if (verbose_) {
            std::cout << "  No array dependencies detected\n";
        }
    } catch (...) {
        recordWarning("Array analysis failed - assuming unsafe");
        if (verbose_) {
            std::cout << "  Array analysis failed\n";
        }
    }
}
void DependencyManager::runPointerAnalysis(LoopInfo& loop) {
    if (verbose_) {
        std::cout << "\n--- Pointer Analysis ---\n";
    }

    try {
        pointer_analyzer_->setVerbose(verbose_);
        pointer_analyzer_->analyzePointerUsage(loop);
        PointerRisk risk = pointer_analyzer_->getPointerRisk(loop);
        
        switch (risk) {
            case PointerRisk::POTENTIAL_ALIAS:
                recordWarning("Potential pointer aliasing detected");
                if (verbose_) {
                    std::cout << "  Potential pointer aliasing\n";
                }
                break;
            case PointerRisk::UNSAFE:
                recordWarning("Complex pointer operations detected");
                if (verbose_) {
                    std::cout << "  Complex pointer operations\n";
                }
                break;
            case PointerRisk::SAFE:
                if (verbose_) {
                    std::cout << "  No risky pointer operations\n";
                }
                break;
        }
    } catch (...) {
        recordWarning("Pointer analysis failed - assuming unsafe");
        if (verbose_) {
            std::cout << "  Pointer analysis failed\n";
        }
    }
}

void DependencyManager::runFunctionAnalysis(LoopInfo& loop) {
    if (verbose_) {
        std::cout << "\n--- Function Call Analysis ---\n";
    }

    try {
        function_analyzer_->setVerbose(verbose_);
        function_analyzer_->analyzeFunctionCalls(loop);
        FunctionCallSafety safety = function_analyzer_->getFunctionCallSafety(loop);
        
        switch (safety) {
            case FunctionCallSafety::UNSAFE:
                recordWarning("Function calls with side effects detected");
                if (verbose_) {
                    std::cout << "  Functions with side effects found\n";
                }
                break;
            case FunctionCallSafety::POTENTIALLY_SAFE:
                if (verbose_) {
                    std::cout << "  Math functions detected (potentially safe)\n";
                }
                break;
            case FunctionCallSafety::SAFE:
                if (verbose_) {
                    std::cout << "  No problematic function calls\n";
                }
                break;
        }
    } catch (...) {
        recordWarning("Function analysis failed - assuming unsafe");
        if (verbose_) {
            std::cout << "  Function analysis failed\n";
        }
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