#pragma once

#include "clang/AST/ASTContext.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/ArrayDependencyAnalyzer.h"
#include "analyzer/PointerAnalyzer.h"
#include "analyzer/FunctionCallAnalyzer.h"
#include "analyzer/PragmaLocationMapper.h"
#include "analyzer/PragmaGenerator.h"
#include "analyzer/SourceAnnotator.h"
#include <string>
#include <vector>
#include <memory>

namespace statik {

// Central manager for all dependency analysis components
class DependencyManager {
public:
    explicit DependencyManager(clang::ASTContext* context);
    
    void analyzeLoop(LoopInfo& loop);
    bool isLoopParallelizable(const LoopInfo& loop) const;
    
    // Pragma location mapping
    void mapPragmaLocations(const std::vector<LoopInfo>& loops);
    
    // Pragma generation
    void generatePragmas(const std::vector<LoopInfo>& loops);
    
    // Source annotation with pragmas
    void annotateSourceFile(const std::string& input_filename,
                           const std::string& output_filename);
    
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
    std::unique_ptr<PragmaGenerator> pragma_generator_;
    std::unique_ptr<SourceAnnotator> source_annotator_;
    
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