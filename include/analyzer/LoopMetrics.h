#pragma once

namespace paralyze
{

// simple metrics for a loop
struct LoopMetrics
{
  unsigned arithmetic_ops = 0;  // +, -, *, /, %
  unsigned memory_accesses = 0; // array accesses, pointer derefs
  unsigned function_calls = 0;  // function call expressions
  unsigned comparisons = 0;     // <, >, ==
  unsigned assignments = 0;     // = and compound assignments

  double hotness_score = 0.0;

  void calculateHotness()
  {
    // basic scoring heuristic
    hotness_score = (arithmetic_ops * 1.0) + (memory_accesses * 2.0) + (function_calls * 5.0) +
                    (comparisons * 0.5) + (assignments * 1.5);
  }

  unsigned getTotalOps() const
  {
    return arithmetic_ops + memory_accesses + function_calls + comparisons + assignments;
  }
};

} // namespace paralyze
