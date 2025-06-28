#include "analyzer/DependencyAnalyzer.h"
#include <iostream>

using namespace clang;

namespace statik {

void DependencyAnalyzer::analyzeDependencies(LoopInfo& loop) {
  // Delegate to the new DependencyManager
  manager_->analyzeLoop(loop);
}

bool DependencyAnalyzer::hasDependencies(const LoopInfo& loop) const {
  return !manager_->isLoopParallelizable(loop);
}

} // namespace statik