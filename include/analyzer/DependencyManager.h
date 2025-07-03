#pragma once

#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/ArrayDependencyAnalyzer.h"
#include "analyzer/PointerAnalyzer.h"
#include "analyzer/FunctionCallAnalyzer.h"
#include "analyzer/PragmaLocationMapper.h"
#include <memory>
#include <vector>

namespace statik {

// Central manager for all dependency analysis components
class DependencyManager {
public:
  explicit DependencyManager(clang::ASTContext* context);
  
  void analyzeLoop(LoopInfo& loop);
  bool isLoopParallelizable(const LoopInfo& loop) const;
  
  // Pragma location mapping
  void mapPragmaLocations(const std::vector<LoopInfo>& loops);
  
  // Get detailed analysis results
  const std::vector<std::string>& getAnalysisWarnings() const { return warnings_; }
  void clearWarnings() { warnings_.clear(); }
  
private:
  clang::ASTContext* context_;
  
  // Specialized analyzers
  std::unique_ptr<ArrayDependencyAnalyzer> array_analyzer_;
  std::unique_ptr<PointerAnalyzer> pointer_analyzer_;
  std::unique_ptr<FunctionCallAnalyzer> function_analyzer_;
  std::unique_ptr<PragmaLocationMapper> location_mapper_;
  
  // Analysis state
  std::vector<std::string> warnings_;
  
  // Analysis orchestration methods
  void runScalarAnalysis(LoopInfo& loop);
  void runArrayAnalysis(LoopInfo& loop);
  void runPointerAnalysis(LoopInfo& loop);
  void runFunctionAnalysis(LoopInfo& loop);
  
  void recordWarning(const std::string& warning);
  bool hasScalarDependencies(const LoopInfo& loop) const;
};

} // namespace statik