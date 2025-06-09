#pragma once

namespace statik {

struct LoopMetrics {
  unsigned arithmetic_ops;     // +, -, *, /, %
  unsigned memory_accesses;    // array accesses, pointer dereferences
  unsigned function_calls;     // function call expressions
  unsigned comparisons;        // <, >, ==, etc.
  unsigned assignments;        // = and compound assignments
  
  double hotness_score;        // Computed overall score
  
  LoopMetrics() : arithmetic_ops(0), memory_accesses(0), function_calls(0),
                  comparisons(0), assignments(0), hotness_score(0.0) {}
                  
  void calculateHotness() {
    // Simple scoring heuristic - can be tuned later
    hotness_score = (arithmetic_ops * 1.0) + 
                   (memory_accesses * 2.0) + 
                   (function_calls * 5.0) + 
                   (comparisons * 0.5) + 
                   (assignments * 1.5);
  }
  
  unsigned getTotalOps() const {
    return arithmetic_ops + memory_accesses + function_calls + 
           comparisons + assignments;
  }
};

} // namespace statik