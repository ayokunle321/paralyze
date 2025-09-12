#include "analyzer/ConfidenceScorer.h"
#include "analyzer/LoopInfo.h"
#include "analyzer/PragmaGenerator.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace paralyze
{

ConfidenceScore ConfidenceScorer::calculateConfidence(const LoopInfo& loop,
                                                      const GeneratedPragma& pragma)
{
  ConfidenceScore score;

  // Score different aspects of the loop
  double loop_score = scoreLoopCharacteristics(loop);
  double pragma_score = scorePragmaType(pragma.type);
  double complexity_score = scoreComplexity(loop);
  double data_score = scoreDataAccess(loop);
  double dependency_score = scoreDependencyAnalysis(loop);

  // Weighted combination (could tune these weights later)
  double weights[] = {0.25, 0.15, 0.20, 0.20, 0.20};
  double scores[] = {loop_score, pragma_score, complexity_score, data_score, dependency_score};

  score.numerical_score = 0.0;
  for (int i = 0; i < 5; i++)
  {
    score.numerical_score += weights[i] * scores[i];
  }

  score.numerical_score = std::max(0.0, std::min(1.0, score.numerical_score));
  score.level = convertToLevel(score.numerical_score);

  // Build list of factors that influenced the score
  std::vector<std::string> positive_factors;
  std::vector<std::string> negative_factors;

  if (loop.bounds.is_simple_pattern)
  {
    positive_factors.push_back("Simple iterator pattern detected");
  }

  if (loop.isHot())
  {
    positive_factors.push_back("High computational intensity");
  }

  if (!loop.array_accesses.empty())
  {
    positive_factors.push_back("Array access patterns found");
  }

  if (loop.depth == 0)
  {
    positive_factors.push_back("Outermost loop (good for parallelization)");
  }
  else
  {
    negative_factors.push_back("Nested loop (reduced parallelization benefit)");
  }

  if (loop.metrics.function_calls > 0)
  {
    negative_factors.push_back("Contains function calls");
  }

  if (loop.variables.size() > 5)
  {
    negative_factors.push_back("Many variables in scope");
  }

  score.positive_factors = positive_factors;
  score.negative_factors = negative_factors;
  score.reasoning = generateReasoning(loop, pragma, positive_factors, negative_factors);

  return score;
}

double ConfidenceScorer::scoreLoopCharacteristics(const LoopInfo& loop)
{
  double score = 0.5;

  if (loop.bounds.is_simple_pattern)
  {
    score += 0.3;
  }

  // Outermost loops are easier to parallelize
  if (loop.depth == 0)
  {
    score += 0.2;
  }
  else
  {
    score -= 0.1 * loop.depth;
  }

  if (loop.isHot())
  {
    score += 0.1;
  }

  return std::max(0.0, std::min(1.0, score));
}

double ConfidenceScorer::scorePragmaType(PragmaType type)
{
  switch (type)
  {
  case PragmaType::PARALLEL_FOR:
    return 0.8;
  case PragmaType::PARALLEL_FOR_SIMD:
    return 0.7;
  case PragmaType::SIMD:
    return 0.6;
  case PragmaType::NO_PRAGMA:
  default:
    return 0.0;
  }
}

double ConfidenceScorer::scoreComplexity(const LoopInfo& loop)
{
  double score = 1.0;

  // Function calls add complexity
  if (loop.metrics.function_calls > 2)
  {
    score -= 0.3;
  }
  else if (loop.metrics.function_calls > 0)
  {
    score -= 0.1;
  }

  // Too many variables might indicate complexity
  if (loop.variables.size() > 8)
  {
    score -= 0.3;
  }
  else if (loop.variables.size() > 5)
  {
    score -= 0.1;
  }

  // Heavy arithmetic might be complex
  if (loop.metrics.arithmetic_ops > 10)
  {
    score -= 0.1;
  }

  return std::max(0.0, std::min(1.0, score));
}

double ConfidenceScorer::scoreDataAccess(const LoopInfo& loop)
{
  double score = 0.5;

  if (!loop.array_accesses.empty())
  {
    score += 0.3;

    // TODO: Actually analyze access patterns for complexity
    bool has_simple_access = true;
    for (const auto& access : loop.array_accesses)
    {
      // Placeholder - should check index expressions
    }

    if (has_simple_access)
    {
      score += 0.2;
    }
  }

  if (loop.metrics.memory_accesses > 5)
  {
    score += 0.1;
  }

  return std::max(0.0, std::min(1.0, score));
}

double ConfidenceScorer::scoreDependencyAnalysis(const LoopInfo& loop)
{
  if (loop.has_dependencies)
  {
    return 0.0; // Can't be confident if there are dependencies
  }

  double score = 0.8;

  // More thorough analysis = higher confidence
  if (!loop.variables.empty())
  {
    score += 0.1;
  }

  if (!loop.array_accesses.empty())
  {
    score += 0.1;
  }

  return std::max(0.0, std::min(1.0, score));
}

ConfidenceLevel ConfidenceScorer::convertToLevel(double score)
{
  if (score >= 0.81)
    return ConfidenceLevel::VERY_HIGH;
  if (score >= 0.61)
    return ConfidenceLevel::HIGH;
  if (score >= 0.41)
    return ConfidenceLevel::MEDIUM;
  if (score >= 0.21)
    return ConfidenceLevel::LOW;
  return ConfidenceLevel::VERY_LOW;
}

std::string ConfidenceScorer::getConfidenceDescription(ConfidenceLevel level) const
{
  switch (level)
  {
  case ConfidenceLevel::VERY_HIGH:
    return "Very High (81-100%)";
  case ConfidenceLevel::HIGH:
    return "High (61-80%)";
  case ConfidenceLevel::MEDIUM:
    return "Medium (41-60%)";
  case ConfidenceLevel::LOW:
    return "Low (21-40%)";
  case ConfidenceLevel::VERY_LOW:
    return "Very Low (0-20%)";
  default:
    return "Unknown";
  }
}

std::string ConfidenceScorer::generateReasoning(const LoopInfo& loop, const GeneratedPragma& pragma,
                                                const std::vector<std::string>& pos_factors,
                                                const std::vector<std::string>& neg_factors)
{
  std::string reasoning = "\n  Confidence based on:\n";
  if (!pos_factors.empty())
  {
    reasoning += "\n    Positive factors:\n";
    for (const auto& factor : pos_factors)
    {
      reasoning += "      - " + factor + "\n";
    }
  }

  if (!neg_factors.empty())
  {
    reasoning += "\n    Negative factors:\n";
    for (const auto& factor : neg_factors)
    {
      reasoning += "      - " + factor + "\n";
    }
  }
  return reasoning;
}

} // namespace paralyze