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

// central manager coordinating all dependency analysis components
class DependencyManager {
public:
    explicit DependencyManager(clang::ASTContext* context);
    
    void analyzeLoop(LoopInfo& loop);            
    bool isLoopParallelizable(const LoopInfo& loop) const;
    void setVerbose(bool verbose) { verbose_ = verbose; }
    void mapPragmaLocations(const std::vector<LoopInfo>& loops);
    void generatePragmas(const std::vector<LoopInfo>& loops);
    void annotateSourceFile(const std::string& input_filename,
                           const std::string& output_filename);
    
    // access analysis warnings
    const std::vector<std::string>& getAnalysisWarnings() const { return warnings_; }
    void clearWarnings() { warnings_.clear(); }
    
private:
    clang::ASTContext* context_;
    bool verbose_ = false;  
    
    // specialized analyzers
    std::unique_ptr<ArrayDependencyAnalyzer> array_analyzer_;
    std::unique_ptr<PointerAnalyzer> pointer_analyzer_;
    std::unique_ptr<FunctionCallAnalyzer> function_analyzer_;
    std::unique_ptr<PragmaLocationMapper> location_mapper_;
    std::unique_ptr<PragmaGenerator> pragma_generator_;
    std::unique_ptr<SourceAnnotator> source_annotator_;
    
    // keeps warnings during analysis
    std::vector<std::string> warnings_;
    
    // internal analysis
    void runScalarAnalysis(LoopInfo& loop);
    void runArrayAnalysis(LoopInfo& loop);
    void runPointerAnalysis(LoopInfo& loop);
    void runFunctionAnalysis(LoopInfo& loop);
    void recordWarning(const std::string& warning);
    bool hasScalarDependencies(const LoopInfo& loop) const;
};

} // namespace statik
