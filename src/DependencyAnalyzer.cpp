#include "analyzer/DependencyAnalyzer.h"
#include <iostream>

using namespace clang;

namespace paralyze
{

void DependencyAnalyzer::analyzeDependencies(LoopInfo& loop)
{
  manager_->analyzeLoop(loop);
}

bool DependencyAnalyzer::hasDependencies(const LoopInfo& loop) const
{
  return !manager_->isLoopParallelizable(loop);
}

} // namespace paralyze