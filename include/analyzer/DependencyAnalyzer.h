#pragma once
#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/DependencyManager.h"
#include <string>
#include <vector>
#include <memory>

namespace statik {

// Main analysis done through DependencyManager
class DependencyAnalyzer {
public:
  explicit DependencyAnalyzer(clang::ASTContext* context) 
      : context_(context), 
        manager_(std::make_unique<DependencyManager>(context)) {}
  
  void analyzeDependencies(LoopInfo& loop);
  bool hasDependencies(const LoopInfo& loop) const;

  void setVerbose(bool verbose) { 
    if (manager_) {
        manager_->setVerbose(verbose);
    }
  }
  
private:
  clang::ASTContext* context_;
  std::unique_ptr<DependencyManager> manager_;
};

} // namespace statik