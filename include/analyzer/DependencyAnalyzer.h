#pragma once
#include "analyzer/DependencyManager.h"
#include "analyzer/LoopInfo.h"
#include "clang/AST/ASTContext.h"
#include <memory>
#include <string>
#include <vector>

namespace paralyze
{

// wrapper around DependencyManager to analyze a loop
class DependencyAnalyzer
{
public:
  explicit DependencyAnalyzer(clang::ASTContext* context)
      : context_(context), manager_(std::make_unique<DependencyManager>(context))
  {
  }

  void analyzeDependencies(LoopInfo& loop); // run analysis for a loop
  bool hasDependencies(const LoopInfo& loop) const;

  void setVerbose(bool verbose)
  {
    if (manager_)
    {
      manager_->setVerbose(verbose); // forward verbosity to manager
    }
  }

private:
  clang::ASTContext* context_;
  std::unique_ptr<DependencyManager> manager_;
};

} // namespace paralyze