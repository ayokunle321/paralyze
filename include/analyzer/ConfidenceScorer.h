#pragma once

#include <string>
#include <vector>

namespace statik {

// Forward declarations to avoid circular dependencies
struct LoopInfo;
struct GeneratedPragma;
enum class PragmaType;

enum class ConfidenceLevel {
  VERY_LOW = 1,    // 0-20% confident
  LOW = 2,         // 21-40% confident  
  MEDIUM = 3,      // 41-60% confident
  HIGH = 4,        // 61-80% confident
  VERY_HIGH = 5    // 81-100% confident
};

struct ConfidenceScore {
  double numerical_score;  // 0.0 - 1.0
  ConfidenceLevel level;
  std::string reasoning;
  std::vector<std::string> positive_factors;
  std::vector<std::string> negative_factors;
  
  ConfidenceScore() : numerical_score(0.0), level(ConfidenceLevel::VERY_LOW) {}
};

class ConfidenceScorer {
public:
  ConfidenceScorer() = default;
  
  ConfidenceScore calculateConfidence(const LoopInfo& loop, const GeneratedPragma& pragma);
  std::string getConfidenceDescription(ConfidenceLevel level) const;
  
private:
  double scoreLoopCharacteristics(const LoopInfo& loop);
  double scorePragmaType(PragmaType type);
  double scoreComplexity(const LoopInfo& loop);
  double scoreDataAccess(const LoopInfo& loop);
  double scoreDependencyAnalysis(const LoopInfo& loop);
  
  ConfidenceLevel convertToLevel(double score);
  std::string generateReasoning(const LoopInfo& loop, const GeneratedPragma& pragma, 
                              const std::vector<std::string>& pos_factors,
                              const std::vector<std::string>& neg_factors);
};

} // namespace statik