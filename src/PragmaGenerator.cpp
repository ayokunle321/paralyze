#include "analyzer/PragmaGenerator.h"
#include <algorithm>
#include <iostream>

namespace paralyze
{

void PragmaGenerator::generatePragmasForLoops(const std::vector<LoopInfo>& loops)
{
  generated_pragmas_.clear();

  if (verbose_)
  {
    std::cout << "\n=== Generating OpenMP Pragmas ===\n";
  }

  for (const auto& loop : loops)
  {
    PragmaType pragma_type = determinePragmaType(loop);

    if (pragma_type != PragmaType::NO_PRAGMA)
    {
      std::string pragma_text = generatePragmaText(pragma_type, loop);
      std::string reasoning = generateReasoning(pragma_type, loop);

      GeneratedPragma pragma(pragma_type, pragma_text, loop.loop_type, loop.line_number, reasoning);

      // add private variables if needed
      std::vector<std::string> private_vars = identifyPrivateVariables(loop);
      if (!private_vars.empty())
      {
        pragma.requires_private_vars = true;
        pragma.private_variables = private_vars;
        pragma.pragma_text += " private(";
        for (size_t i = 0; i < private_vars.size(); i++)
        {
          if (i > 0)
            pragma.pragma_text += ", ";
          pragma.pragma_text += private_vars[i];
        }
        pragma.pragma_text += ")";
      }

      // calculate confidence score
      if (confidence_scorer_)
      {
        pragma.confidence = confidence_scorer_->calculateConfidence(loop, pragma);
      }
      else
      {
        pragma.confidence.numerical_score = 0.5;
        pragma.confidence.level = ConfidenceLevel::MEDIUM;
        pragma.confidence.reasoning = "Confidence scorer not available";
      }

      generated_pragmas_.push_back(pragma);

      // only show detailed info in verbose mode
      if (verbose_)
      {
        std::cout << "\nGenerated pragma for " << loop.loop_type << " loop at line "
                  << loop.line_number << ":\n";
        std::cout << "  " << pragma.pragma_text << "\n";
        std::cout << "\nReasoning:\n  " << reasoning << "\n";

        if (confidence_scorer_)
        {
          std::cout << "\nConfidence: "
                    << confidence_scorer_->getConfidenceDescription(pragma.confidence.level) << " ("
                    << static_cast<int>(pragma.confidence.numerical_score * 100) << "%)\n";
          std::cout << "  " << pragma.confidence.reasoning << "\n";
        }
      }
    }
    else
    {
      if (verbose_)
      {
        std::cout << "\nNo pragma generated for " << loop.loop_type << " loop at line "
                  << loop.line_number << " (has dependencies)\n";
      }
    }
  }

  if (verbose_)
  {
    std::cout << "======================================================\n";
  }
}

void PragmaGenerator::printCleanSummary() const
{
  if (generated_pragmas_.empty())
  {
    std::cout << "No parallelizable loops found.\n";
    return;
  }

  std::cout << "\nGenerated " << generated_pragmas_.size() << " OpenMP pragma";
  if (generated_pragmas_.size() > 1)
    std::cout << "s";
  std::cout << ":\n";

  for (const auto& pragma : generated_pragmas_)
  {
    std::cout << "  Line " << pragma.line_number << ": " << pragma.pragma_text << "\n";
  }
}

void PragmaGenerator::printPragmaSummary() const
{
  if (!verbose_)
    return;

  std::cout << "\n=== Pragma Generation Summary ===\n";
  std::cout << "Total pragmas generated: " << generated_pragmas_.size() << "\n\n";

  int parallel_for_count = 0;
  int parallel_for_simd_count = 0;
  int simd_count = 0;
  double avg_confidence = 0.0;

  for (const auto& pragma : generated_pragmas_)
  {
    switch (pragma.type)
    {
    case PragmaType::PARALLEL_FOR:
      parallel_for_count++;
      break;
    case PragmaType::PARALLEL_FOR_SIMD:
      parallel_for_simd_count++;
      break;
    case PragmaType::SIMD:
      simd_count++;
      break;
    default:
      break;
    }
    avg_confidence += pragma.confidence.numerical_score;

    std::cout << "Line " << pragma.line_number << ": " << pragma.pragma_text;
    if (pragma.requires_private_vars)
    {
      std::cout << " (with private variables)";
    }

    if (confidence_scorer_)
    {
      std::cout << " [Confidence: "
                << confidence_scorer_->getConfidenceDescription(pragma.confidence.level) << "]\n";
    }
    else
    {
      std::cout << " [Confidence: N/A]\n";
    }
  }

  if (!generated_pragmas_.empty())
  {
    avg_confidence /= generated_pragmas_.size();
  }

  std::cout << "\nBreakdown:\n";
  std::cout << "  #pragma omp parallel for: " << parallel_for_count << "\n";
  std::cout << "  #pragma omp parallel for simd: " << parallel_for_simd_count << "\n";
  std::cout << "  #pragma omp simd: " << simd_count << "\n";
  std::cout << "  Average confidence: " << static_cast<int>(avg_confidence * 100) << "%\n";
}

PragmaType PragmaGenerator::determinePragmaType(const LoopInfo& loop)
{
  if (loop.has_dependencies)
  {
    return PragmaType::NO_PRAGMA;
  }

  // be conservative with nested loops
  if (loop.depth > 0)
  {
    if (shouldUseSimd(loop))
    {
      return PragmaType::SIMD;
    }
    return PragmaType::NO_PRAGMA;
  }

  // for outermost loops, consider SIMD + parallelization
  if (shouldUseSimd(loop))
  {
    return PragmaType::PARALLEL_FOR_SIMD;
  }
  return PragmaType::PARALLEL_FOR;
}

std::string PragmaGenerator::generatePragmaText(PragmaType type, const LoopInfo& loop)
{
  switch (type)
  {
  case PragmaType::PARALLEL_FOR:
    return "#pragma omp parallel for";
  case PragmaType::PARALLEL_FOR_SIMD:
    return "#pragma omp parallel for simd";
  case PragmaType::SIMD:
    return "#pragma omp simd";
  case PragmaType::NO_PRAGMA:
  default:
    return "";
  }
}

std::string PragmaGenerator::generateReasoning(PragmaType type, const LoopInfo& loop)
{
  std::string reason;
  switch (type)
  {
  case PragmaType::PARALLEL_FOR:
    reason = "Loop has no dependencies and good parallelization potential";
    break;
  case PragmaType::PARALLEL_FOR_SIMD:
    reason = "Loop has simple array operations suitable for both parallelization and vectorization";
    break;
  case PragmaType::SIMD:
    reason = "Inner loop with simple operations suitable for vectorization";
    break;
  case PragmaType::NO_PRAGMA:
  default:
    reason = "Loop has dependencies or is not suitable for parallelization";
    break;
  }
  return reason;
}

bool PragmaGenerator::shouldUseSimd(const LoopInfo& loop)
{
  // SIMD works best with simple array access + arithmetic
  if (!hasSimpleArrayAccess(loop))
  {
    return false;
  }

  // prefer arithmetic-heavy loops
  if (loop.metrics.arithmetic_ops > loop.metrics.function_calls * 2)
  {
    return true;
  }

  // inner loops with memory access are good SIMD candidates
  if (isInnerLoop(loop) && loop.metrics.memory_accesses > 0)
  {
    return true;
  }

  return false;
}

bool PragmaGenerator::hasSimpleArrayAccess(const LoopInfo& loop)
{
  // if loop is parallelizable and has array access, assume it's simple enough
  return !loop.array_accesses.empty();
}

bool PragmaGenerator::isInnerLoop(const LoopInfo& loop)
{
  return loop.depth > 0;
}

std::vector<std::string> PragmaGenerator::identifyPrivateVariables(const LoopInfo& loop)
{
  std::vector<std::string> private_vars;

  for (const auto& var_pair : loop.variables)
  {
    const auto& var = var_pair.second;

    // skip induction variables (automatically private in OpenMP)
    if (var.isInductionVariable())
    {
      continue;
    }

    // loop-local variables that are written should be private
    if (var.scope == VariableScope::LOOP_LOCAL && var.hasWrites())
    {
      private_vars.push_back(var.name);
    }
  }

  return private_vars;
}

} // namespace paralyze