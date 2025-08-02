#include "analyzer/PragmaLocationMapper.h"
#include "clang/AST/Stmt.h"
#include <iostream>
#include <fstream>

using namespace clang;

namespace statik {

void PragmaLocationMapper::mapLoopToPragmaLocation(const LoopInfo& loop) {
  std::cout << "  Mapping pragma insertion point for " << loop.loop_type 
           << " loop at line " << loop.line_number << "\n";
  
  SourceLocation pragma_loc = findPragmaInsertionLocation(loop.stmt);
  
  if (pragma_loc.isInvalid()) {
    std::cout << "  Warning: Could not determine pragma insertion location\n";
    return;
  }
  
  // Skip macro expansions
  if (isLocationInMacro(pragma_loc)) {
    std::cout << "  Skipping loop in macro expansion\n";
    return;
  }
  
  unsigned spelling_line = source_manager_->getSpellingLineNumber(pragma_loc);
  unsigned expansion_line = source_manager_->getExpansionLineNumber(pragma_loc);
  
  if (spelling_line != expansion_line) {
    std::cout << "  Note: Line number mismatch due to preprocessor (spelling: " 
             << spelling_line << ", expansion: " << expansion_line << ")\n";
  }
  
  unsigned line = spelling_line;
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
  
  // Handle macro expansions
  if (loop_start.isMacroID()) {
    loop_start = source_manager_->getSpellingLoc(loop_start);
    
    if (loop_start.isInvalid()) {
      std::cout << "  Warning: Could not resolve macro expansion location\n";
      return SourceLocation();
    }
  }
  
  SourceLocation line_start = moveToStartOfLine(loop_start);
  
  // TODO: Check for preprocessor directives on previous line
  unsigned line_num = source_manager_->getSpellingLineNumber(line_start);
  if (line_num > 1) {
    SourceLocation prev_line = source_manager_->translateLineCol(
        source_manager_->getFileID(line_start), line_num - 1, 1);
    
    if (prev_line.isValid()) {
      // For now just use current line - could improve this
      return line_start;
    }
  }
  
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
  
  return loc.isMacroID();
}

std::string PragmaLocationMapper::getIndentationAtLocation(SourceLocation loc) {
  if (loc.isInvalid()) {
    return "";
  }
  
  SourceLocation line_start = moveToStartOfLine(loc);
  
  // TODO: Actually read the source file to get real indentation
  // For now just return reasonable default
  return "    ";
}

SourceLocation PragmaLocationMapper::moveToStartOfLine(SourceLocation loc) {
  if (loc.isInvalid()) {
    return loc;
  }
  
  // Could use Clang's Lexer class for better line handling
  unsigned line = source_manager_->getSpellingLineNumber(loc);
  FileID file_id = source_manager_->getFileID(loc);
  
  SourceLocation line_start = source_manager_->translateLineCol(file_id, line, 1);
  
  return line_start.isValid() ? line_start : loc;
}

} // namespace statik