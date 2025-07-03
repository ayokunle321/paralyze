#include "analyzer/PragmaLocationMapper.h"
#include "clang/AST/Stmt.h"
#include <iostream>
#include <fstream>

using namespace clang;

namespace statik {

void PragmaLocationMapper::mapLoopToPragmaLocation(const LoopInfo& loop) {
  std::cout << "  Mapping pragma insertion point for " << loop.loop_type 
           << " loop at line " << loop.line_number << "\n";
  
  // Find the best location to insert the pragma (usually before the loop)
  SourceLocation pragma_loc = findPragmaInsertionLocation(loop.stmt);
  
  if (pragma_loc.isInvalid()) {
    std::cout << "  Warning: Could not determine pragma insertion location\n";
    return;
  }
  
  // Skip loops in macro expansions - too risky to modify
  if (isLocationInMacro(pragma_loc)) {
    std::cout << "  Skipping loop in macro expansion\n";
    return;
  }
  
  unsigned line = source_manager_->getSpellingLineNumber(pragma_loc);
  unsigned col = getColumnNumber(pragma_loc);
  
  PragmaInsertionPoint point(pragma_loc, line, col, loop.loop_type,
                           loop.depth > 0, loop.depth);
  
  insertion_points_.push_back(point);
  
  std::cout << "  Pragma insertion point: line " << line << ", column " << col;
  if (loop.depth > 0) {
    std::cout << " (nested depth " << loop.depth << ")";
  }
  std::cout << "\n";
}

SourceLocation PragmaLocationMapper::findPragmaInsertionLocation(Stmt* loop_stmt) {
  if (!loop_stmt) {
    return SourceLocation();
  }
  
  SourceLocation loop_start = loop_stmt->getBeginLoc();
  if (loop_start.isInvalid()) {
    return SourceLocation();
  }
  
  // Try to move to the beginning of the line containing the loop
  SourceLocation line_start = moveToStartOfLine(loop_start);
  
  return line_start;
}

unsigned PragmaLocationMapper::getColumnNumber(SourceLocation loc) {
  if (loc.isInvalid()) {
    return 0;
  }
  
  return source_manager_->getSpellingColumnNumber(loc);
}

bool PragmaLocationMapper::isLocationInMacro(SourceLocation loc) {
  if (loc.isInvalid()) {
    return false;
  }
  
  // Check if this location is from a macro expansion
  return loc.isMacroID();
}

std::string PragmaLocationMapper::getIndentationAtLocation(SourceLocation loc) {
  if (loc.isInvalid()) {
    return "";
  }
  
  // Get the line start and read characters until we hit non-whitespace
  SourceLocation line_start = moveToStartOfLine(loc);
  
  // For now, return a reasonable default indentation
  return "    "; // 4 spaces
}

SourceLocation PragmaLocationMapper::moveToStartOfLine(SourceLocation loc) {
  if (loc.isInvalid()) {
    return loc;
  }
  
  unsigned line = source_manager_->getSpellingLineNumber(loc);
  FileID file_id = source_manager_->getFileID(loc);
  
  // Try to get location at column 1 of the same line
  SourceLocation line_start = source_manager_->translateLineCol(file_id, line, 1);
  
  return line_start.isValid() ? line_start : loc;
}

} // namespace statik