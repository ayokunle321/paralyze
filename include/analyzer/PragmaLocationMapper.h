#pragma once

#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "analyzer/LoopInfo.h"
#include <string>
#include <vector>

namespace statik {

struct PragmaInsertionPoint {
  clang::SourceLocation location;
  unsigned line_number;
  unsigned column_number;
  std::string loop_type;        // "for", "while", "do-while"
  bool is_nested;              // true if this is a nested loop
  unsigned nesting_depth;
  std::string suggested_pragma; // The actual pragma text to insert
  
  PragmaInsertionPoint(clang::SourceLocation loc, unsigned line, unsigned col,
                      const std::string& type, bool nested, unsigned depth)
      : location(loc), line_number(line), column_number(col), 
        loop_type(type), is_nested(nested), nesting_depth(depth) {}
};

class PragmaLocationMapper {
public:
  explicit PragmaLocationMapper(clang::SourceManager* source_manager)
      : source_manager_(source_manager) {}
  
  void mapLoopToPragmaLocation(const LoopInfo& loop);
  const std::vector<PragmaInsertionPoint>& getInsertionPoints() const { 
    return insertion_points_; 
  }
  
  void clearInsertionPoints() { insertion_points_.clear(); }
  
private:
  clang::SourceManager* source_manager_;
  std::vector<PragmaInsertionPoint> insertion_points_;
  
  clang::SourceLocation findPragmaInsertionLocation(clang::Stmt* loop_stmt);
  unsigned getColumnNumber(clang::SourceLocation loc);
  bool isLocationInMacro(clang::SourceLocation loc);
  std::string getIndentationAtLocation(clang::SourceLocation loc);
  clang::SourceLocation moveToStartOfLine(clang::SourceLocation loc);
};

} // namespace statik