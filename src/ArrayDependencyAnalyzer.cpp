#include "analyzer/ArrayDependencyAnalyzer.h"
#include "clang/AST/Expr.h"
#include <iostream>

using namespace clang;

namespace paralyze
{

void ArrayDependencyAnalyzer::analyzeArrayDependencies(LoopInfo& loop)
{
  detected_dependencies_.clear();

  if (verbose_)
  {
    std::cout << "  Analyzing array dependencies for " << loop.array_accesses.size()
              << " array accesses\n";
  }

  // check all pairs of array accesses for conflicts
  for (size_t i = 0; i < loop.array_accesses.size(); i++)
  {
    for (size_t j = i + 1; j < loop.array_accesses.size(); j++)
    {
      const ArrayAccess& access1 = loop.array_accesses[i];
      const ArrayAccess& access2 = loop.array_accesses[j];

      if (access1.array_name == access2.array_name)
      {
        checkArrayAccessPair(access1, access2, loop.bounds.iterator_var);
      }
    }
  }

  // run cross-iteration analysis
  cross_iteration_analyzer_->setVerbose(verbose_);
  cross_iteration_analyzer_->analyzeCrossIterationConflicts(loop);

  if (verbose_)
  {
    std::cout << "  Found " << detected_dependencies_.size() << " basic array dependencies\n";
  }
}

bool ArrayDependencyAnalyzer::hasArrayDependencies(const LoopInfo& loop) const
{
  for (const auto& dep : detected_dependencies_)
  {
    if (dep.type != ArrayDependencyType::NO_DEPENDENCY)
    {
      return true;
    }
  }

  if (cross_iteration_analyzer_->hasCrossIterationConflicts(loop))
  {
    return true;
  }

  return false;
}

void ArrayDependencyAnalyzer::checkArrayAccessPair(const ArrayAccess& access1,
                                                   const ArrayAccess& access2,
                                                   const std::string& induction_var)
{
  // skip read-only pairs
  if (!access1.is_write && !access2.is_write)
  {
    return;
  }

  ArrayDependencyType dep_type =
      compareArrayIndices(access1.subscript, access2.subscript, induction_var);

  if (dep_type != ArrayDependencyType::NO_DEPENDENCY)
  {
    std::string idx1_str = exprToString(access1.subscript);
    std::string idx2_str = exprToString(access2.subscript);

    ArrayDependency dependency(access1.array_name, dep_type, access1.line_number,
                               access2.line_number, idx1_str, idx2_str);

    detected_dependencies_.push_back(dependency);

    if (verbose_)
    {
      std::cout << "  Array dependency: " << access1.array_name << "[" << idx1_str << "] vs ["
                << idx2_str << "] - ";

      switch (dep_type)
      {
      case ArrayDependencyType::SAME_INDEX:
        std::cout << "SAME INDEX (write conflict)";
        break;
      case ArrayDependencyType::CONSTANT_OFFSET:
        std::cout << "CONSTANT OFFSET (loop-carried)";
        break;
      case ArrayDependencyType::UNKNOWN_RELATION:
        std::cout << "UNKNOWN (assume unsafe)";
        break;
      default:
        break;
      }
      std::cout << "\n";
    }
  }
}

ArrayDependencyType ArrayDependencyAnalyzer::compareArrayIndices(Expr* index1, Expr* index2,
                                                                 const std::string& induction_var)
{
  if (!index1 || !index2)
  {
    return ArrayDependencyType::UNKNOWN_RELATION;
  }

  try
  {
    std::string idx1_str = exprToString(index1);
    std::string idx2_str = exprToString(index2);

    if (idx1_str == "error_expr" || idx2_str == "error_expr")
    {
      return ArrayDependencyType::UNKNOWN_RELATION;
    }

    // check for A[i] vs A[i] pattern
    if (isSimpleInductionAccess(index1, induction_var) &&
        isSimpleInductionAccess(index2, induction_var))
    {
      return ArrayDependencyType::SAME_INDEX;
    }

    // check for offset patterns like A[i] vs A[i+1]
    if (hasConstantOffset(index1, index2))
    {
      return ArrayDependencyType::CONSTANT_OFFSET;
    }

    if (idx1_str != idx2_str)
    {
      return ArrayDependencyType::UNKNOWN_RELATION;
    }

    return ArrayDependencyType::NO_DEPENDENCY;
  }
  catch (...)
  {
    return ArrayDependencyType::UNKNOWN_RELATION;
  }
}

bool ArrayDependencyAnalyzer::isSimpleInductionAccess(Expr* index, const std::string& induction_var)
{
  if (!index || induction_var.empty())
  {
    return false;
  }

  index = index->IgnoreParenImpCasts();
  if (auto* declRef = dyn_cast<DeclRefExpr>(index))
  {
    return declRef->getDecl()->getNameAsString() == induction_var;
  }

  return false;
}

bool ArrayDependencyAnalyzer::hasConstantOffset(Expr* index1, Expr* index2)
{
  if (!index1 || !index2)
  {
    return false;
  }

  // TODO: Improve this - current implementation is pretty basic
  std::string str1 = exprToString(index1);
  std::string str2 = exprToString(index2);

  // look for arithmetic patterns
  if ((str1.find("+") != std::string::npos || str1.find("-") != std::string::npos) &&
      (str2.find("+") != std::string::npos || str2.find("-") != std::string::npos))
  {
    return true;
  }

  return false;
}

std::string ArrayDependencyAnalyzer::exprToString(Expr* expr)
{
  if (!expr)
  {
    return "null";
  }

  try
  {
    expr = expr->IgnoreParenImpCasts();

    if (auto* declRef = dyn_cast<DeclRefExpr>(expr))
    {
      if (declRef->getDecl())
      {
        return declRef->getDecl()->getNameAsString();
      }
      return "unknown_var";
    }

    if (auto* binOp = dyn_cast<BinaryOperator>(expr))
    {
      std::string lhs = exprToString(binOp->getLHS());
      std::string rhs = exprToString(binOp->getRHS());

      // avoid infinite recursion
      if (lhs == "complex_expr" || rhs == "complex_expr")
      {
        return "complex_expr";
      }

      std::string op;
      switch (binOp->getOpcode())
      {
      case BO_Add:
        op = " + ";
        break;
      case BO_Sub:
        op = " - ";
        break;
      case BO_Mul:
        op = " * ";
        break;
      case BO_Div:
        op = " / ";
        break;
      default:
        op = " ? ";
        break;
      }

      return lhs + op + rhs;
    }

    if (auto* intLit = dyn_cast<IntegerLiteral>(expr))
    {
      return std::to_string(intLit->getValue().getSExtValue());
    }

    if (auto* unaryOp = dyn_cast<UnaryOperator>(expr))
    {
      std::string sub = exprToString(unaryOp->getSubExpr());
      switch (unaryOp->getOpcode())
      {
      case UO_Minus:
        return "-" + sub;
      case UO_Plus:
        return "+" + sub;
      default:
        return "unary_op(" + sub + ")";
      }
    }

    return "complex_expr";
  }
  catch (...)
  {
    return "error_expr";
  }
}

} // namespace paralyze