#include "analyzer/CrossIterationAnalyzer.h"
#include "clang/AST/Expr.h"
#include <iostream>
#include <algorithm>
#include <map>

using namespace clang;

namespace statik {

void CrossIterationAnalyzer::analyzeCrossIterationConflicts(LoopInfo& loop) {
  conflicts_.clear();
  
  if (verbose_) {
    std::cout << "  Analyzing cross-iteration conflicts for loop at line " 
             << loop.line_number << "\n";
  }
  
  // group array accesses by array name
  std::map<std::string, std::vector<ArrayAccess>> arrays_map;
  for (const auto& access : loop.array_accesses) {
    arrays_map[access.array_name].push_back(access);
  }
  
  // analyze each array separately
  for (const auto& array_pair : arrays_map) {
    const std::string& array_name = array_pair.first;
    const std::vector<ArrayAccess>& accesses = array_pair.second;
    
    if (accesses.size() > 1) {
      analyzeArrayAccessPattern(array_name, accesses, loop.bounds.iterator_var);
    }
  }
  
  if (verbose_) {
    if (conflicts_.empty()) {
      std::cout << "  No cross-iteration conflicts detected\n";
    } else {
      std::cout << "  Found " << conflicts_.size() << " potential cross-iteration conflicts\n";
    }
  }
}

bool CrossIterationAnalyzer::hasCrossIterationConflicts(const LoopInfo& loop) const {
  return !conflicts_.empty();
}

void CrossIterationAnalyzer::analyzeArrayAccessPattern(const std::string& array_name, 
                                                     const std::vector<ArrayAccess>& accesses,
                                                     const std::string& induction_var) {
  
  if (verbose_) {
    std::cout << "  Analyzing " << accesses.size() << " accesses to array " << array_name << "\n";
  }
  
  // check every pair of accesses for potential conflicts
  for (size_t i = 0; i < accesses.size(); i++) {
    for (size_t j = i + 1; j < accesses.size(); j++) {
      const ArrayAccess& access1 = accesses[i];
      const ArrayAccess& access2 = accesses[j];
      
      // skip if both are reads - no conflict
      if (!access1.is_write && !access2.is_write) {
        continue;
      }
      
      // analyze the index expressions to detect stride patterns
      int offset1 = 0, offset2 = 0;
      bool has_offset1 = hasOffsetFromInduction(access1.subscript, induction_var, offset1);
      bool has_offset2 = hasOffsetFromInduction(access2.subscript, induction_var, offset2);
      
      if (has_offset1 && has_offset2) {
        // both use induction variable with offsets - check for conflicts
        int stride = 1; // assume unit stride for now
        IterationConflictType conflict_type = classifyConflict(access1, access2, 
                                                             offset1, offset2, stride);
        
        if (conflict_type != IterationConflictType::NO_CONFLICT) {
          // format the pattern correctly
          std::string pattern1 = induction_var;
          std::string pattern2 = induction_var;
          
          if (offset1 > 0) {
            pattern1 += "+" + std::to_string(offset1);
          } else if (offset1 < 0) {
            pattern1 += std::to_string(offset1);
          }
          
          if (offset2 > 0) {
            pattern2 += "+" + std::to_string(offset2);
          } else if (offset2 < 0) {
            pattern2 += std::to_string(offset2);
          }
          
          std::string pattern = pattern1 + " vs " + pattern2;
          std::string desc = describeConflict(conflict_type, array_name, pattern);
          
          CrossIterationConflict conflict(array_name, conflict_type, pattern,
                                        access1.line_number, access2.line_number, desc);
          conflicts_.push_back(conflict);
          
          if (verbose_) {
            std::cout << "  Cross-iteration conflict: " << desc << "\n";
          }
        }
      } else if (!has_offset1 || !has_offset2) {
        // one or both indices are complex - conservative
        std::string pattern = "complex_indices";
        std::string desc = describeConflict(IterationConflictType::STRIDE_CONFLICT, 
                                          array_name, pattern);
        
        CrossIterationConflict conflict(array_name, IterationConflictType::STRIDE_CONFLICT,
                                      pattern, access1.line_number, access2.line_number, desc);
        conflicts_.push_back(conflict);
        
        if (verbose_) {
          std::cout << "  Complex index pattern - assuming unsafe: " << desc << "\n";
        }
      }
    }
  }
}

bool CrossIterationAnalyzer::hasOffsetFromInduction(Expr* index, 
                                                   const std::string& induction_var, 
                                                   int& offset) {
  if (!index || induction_var.empty()) {
    return false;
  }
  
  index = index->IgnoreParenImpCasts();
  
  // case 1: simple induction variable
  if (auto* declRef = dyn_cast<DeclRefExpr>(index)) {
    if (declRef->getDecl()->getNameAsString() == induction_var) {
      offset = 0;
      return true;
    }
  }
  
  // case 2: induction variable with constant offset
  if (auto* binOp = dyn_cast<BinaryOperator>(index)) {
    if (binOp->getOpcode() == BO_Add || binOp->getOpcode() == BO_Sub) {
      Expr* lhs = binOp->getLHS()->IgnoreParenImpCasts();
      Expr* rhs = binOp->getRHS()->IgnoreParenImpCasts();
      
      // check if LHS is induction variable and RHS is constant
      if (auto* lhsRef = dyn_cast<DeclRefExpr>(lhs)) {
        if (lhsRef->getDecl()->getNameAsString() == induction_var) {
          if (auto* rhsLit = dyn_cast<IntegerLiteral>(rhs)) {
            int constant = rhsLit->getValue().getSExtValue();
            offset = (binOp->getOpcode() == BO_Add) ? constant : -constant;
            return true;
          }
        }
      }
      
      // check if RHS is induction variable and LHS is constant (rare but possible)
      if (auto* rhsRef = dyn_cast<DeclRefExpr>(rhs)) {
        if (rhsRef->getDecl()->getNameAsString() == induction_var && 
            binOp->getOpcode() == BO_Add) {
          if (auto* lhsLit = dyn_cast<IntegerLiteral>(lhs)) {
            offset = lhsLit->getValue().getSExtValue();
            return true;
          }
        }
      }
    }
  }
  
  return false;
}

IterationConflictType CrossIterationAnalyzer::classifyConflict(const ArrayAccess& access1, 
                                                             const ArrayAccess& access2,
                                                             int offset1, int offset2, 
                                                             int stride) {
  // if offsets are the same, different iterations access same element
  if (offset1 == offset2) {
    if (access1.is_write && access2.is_write) {
      return IterationConflictType::WRITE_AFTER_WRITE;
    } else if (access1.is_write || access2.is_write) {
      // one write, one read
      if (access1.line_number < access2.line_number) {
        return access1.is_write ? IterationConflictType::READ_AFTER_WRITE 
                               : IterationConflictType::WRITE_AFTER_READ;
      } else {
        return access2.is_write ? IterationConflictType::READ_AFTER_WRITE 
                               : IterationConflictType::WRITE_AFTER_READ;
      }
    }
  }
  
  // check if offset difference matches stride
  int offset_diff = std::abs(offset1 - offset2);
  if (offset_diff == stride) {
    // adjacent iterations accessing related elements
    if (access1.is_write || access2.is_write) {
      return IterationConflictType::WRITE_AFTER_READ;  // Conservative
    }
  }
  
  return IterationConflictType::NO_CONFLICT;
}

std::string CrossIterationAnalyzer::describeConflict(IterationConflictType type, 
                                                    const std::string& array_name,
                                                    const std::string& pattern) {
  std::string desc = array_name + "[" + pattern + "] - ";
  
  switch (type) {
    case IterationConflictType::WRITE_AFTER_READ:
      desc += "write after read conflict";
      break;
    case IterationConflictType::READ_AFTER_WRITE:
      desc += "read after write conflict"; 
      break;
    case IterationConflictType::WRITE_AFTER_WRITE:
      desc += "write after write conflict";
      break;
    case IterationConflictType::STRIDE_CONFLICT:
      desc += "stride/indexing conflict";
      break;
    default:
      desc += "unknown conflict";
      break;
  }
  
  return desc;
}

bool CrossIterationAnalyzer::detectsStridePattern(Expr* index, const std::string& induction_var, int& stride) {
  // assume unit stride for now
  stride = 1;
  return true;
}

} // namespace statik