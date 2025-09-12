#pragma once

#include <string>
#include <vector>

namespace paralyze
{

struct LoopInfo;
struct GeneratedPragma;
enum class PragmaType;

// how confident we are that a pragma is useful
enum class ConfidenceLevel
{
  VERY_LOW = 1, // 0-20%
  LOW = 2,      // 21-40%
  MEDIUM = 3,   // 41-60%
  HIGH = 4,     // 61-80%
  VERY_HIGH = 5 // 81-100%
};

// score and reasoning for a single loop/pragma
struct ConfidenceScore
{
  double numerical_score; // 0.0 - 1.0
  ConfidenceLevel level;
  std::string reasoning; // why we think this score
  std::vector<std::string> positive_factors;
  std::vector<std::string> negative_factors;

  ConfidenceScore() : numerical_score(0.0), level(ConfidenceLevel::VERY_LOW) {}
};

// computes confidence for a loop pragma
class ConfidenceScorer
{
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

} // namespace paralyze
