#pragma once

#include "analyzer/LoopInfo.h"
#include "analyzer/PragmaLocationMapper.h"
#include "analyzer/ConfidenceScorer.h"
#include <string>
#include <vector>
#include <memory>

namespace statik {

enum class PragmaType {
  PARALLEL_FOR,       // #pragma omp parallel for
  PARALLEL_FOR_SIMD,  // #pragma omp parallel for simd
  SIMD,              // #pragma omp simd
  NO_PRAGMA          // Loop not suitable for parallelization
};

struct GeneratedPragma {
  PragmaType type;
  std::string pragma_text;
  std::string loop_type;
  unsigned line_number;
  std::string reasoning;
  bool requires_private_vars;
  std::vector<std::string> private_variables;
  ConfidenceScore confidence;
  
  GeneratedPragma(PragmaType pragma_type, const std::string& text, 
                 const std::string& loop, unsigned line, const std::string& reason)
      : type(pragma_type), pragma_text(text), loop_type(loop), 
        line_number(line), reasoning(reason), requires_private_vars(false) {}
};

class PragmaGenerator {
public:
  PragmaGenerator() : confidence_scorer_(std::make_unique<ConfidenceScorer>()) {}
  
  void generatePragmasForLoops(const std::vector<LoopInfo>& loops);
  const std::vector<GeneratedPragma>& getGeneratedPragmas() const { 
    return generated_pragmas_; 
  }
  
  void clearPragmas() { generated_pragmas_.clear(); }
  void printPragmaSummary() const;
  
private:
  std::vector<GeneratedPragma> generated_pragmas_;
  std::unique_ptr<ConfidenceScorer> confidence_scorer_;
  
  PragmaType determinePragmaType(const LoopInfo& loop);
  std::string generatePragmaText(PragmaType type, const LoopInfo& loop);
  std::string generateReasoning(PragmaType type, const LoopInfo& loop);
  
  bool shouldUseSimd(const LoopInfo& loop);
  bool hasSimpleArrayAccess(const LoopInfo& loop);
  bool isInnerLoop(const LoopInfo& loop);
  std::vector<std::string> identifyPrivateVariables(const LoopInfo& loop);
};

} // namespace statik