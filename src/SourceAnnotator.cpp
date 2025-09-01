#include "analyzer/SourceAnnotator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

namespace statik {

void SourceAnnotator::annotateSourceWithPragmas(const std::string& input_filename,
                                               const std::vector<GeneratedPragma>& pragmas,
                                               const std::vector<PragmaInsertionPoint>& insertion_points) {
  std::cout << "\n=== Annotating Source with OpenMP Pragmas ===\n";
  std::cout << "Input file: " << input_filename << "\n";
  
  if (!readSourceFile(input_filename)) {
    std::cout << "Error: Could not read source file\n";
    return;
  }
  
  insertPragmaAnnotations(pragmas, insertion_points);
  
  std::cout << "Annotation complete. Found " << pragmas.size() 
           << " pragmas to insert.\n";
  std::cout << "============================================\n";
}

bool SourceAnnotator::writeAnnotatedFile(const std::string& output_filename) {
  std::ofstream outfile(output_filename);
  if (!outfile.is_open()) {
    std::cout << "Error: Could not create output file " << output_filename << "\n";
    return false;
  }
  
  std::cout << "\nWriting annotated source to: " << output_filename << "\n";
  
  for (const auto& line : annotated_lines_) {
    // Write pragma annotation first (if any)
    if (line.has_pragma) {
      outfile << line.pragma_annotation << "\n";
    }
    
    // Write original line
    outfile << line.original_content << "\n";
  }
  
  outfile.close();
  std::cout << "Successfully wrote " << annotated_lines_.size() 
           << " lines to output file\n";
  
  return true;
}

void SourceAnnotator::printAnnotationSummary() const {
  // std::cout << "\n=== Annotation Summary ===\n";
  
  // int pragma_count = 0;
  // for (const auto& line : annotated_lines_) {
  //   if (line.has_pragma) {
  //     pragma_count++;
  //     std::cout << "Line " << line.line_number << ": " 
  //              << line.pragma_annotation << "\n";
  //   }
  // }
  
  // std::cout << "Total pragmas inserted: " << pragma_count << "\n";
  // std::cout << "=========================\n";
}

bool SourceAnnotator::readSourceFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }
  
  annotated_lines_.clear();
  input_file_ = filename;
  
  std::string line;
  unsigned line_number = 1;
  
  while (std::getline(file, line)) {
    annotated_lines_.emplace_back(line_number, line);
    line_number++;
  }
  
  file.close();
  
  std::cout << "Read " << annotated_lines_.size() << " lines from " << filename << "\n";
  return true;
}

void SourceAnnotator::insertPragmaAnnotations(const std::vector<GeneratedPragma>& pragmas,
                                             const std::vector<PragmaInsertionPoint>& insertion_points) {
  // create a map of line numbers to pragmas for quick lookup
  std::map<unsigned, std::string> pragma_map;
  
  for (const auto& pragma : pragmas) {
    // find the corresponding insertion point for this pragma
    auto insertion_it = std::find_if(insertion_points.begin(), insertion_points.end(),
        [&pragma](const PragmaInsertionPoint& point) {
            return point.line_number == pragma.line_number;
        });
    
    if (insertion_it != insertion_points.end()) {
      std::string indentation = getIndentationForLine(pragma.line_number);
      std::string full_pragma = indentation + pragma.pragma_text;
      pragma_map[pragma.line_number] = full_pragma;
    }
  }
  
  // apply pragmas to the appropriate lines
  for (auto& line : annotated_lines_) {
    auto pragma_it = pragma_map.find(line.line_number);
    if (pragma_it != pragma_map.end()) {
      line.has_pragma = true;
      line.pragma_annotation = pragma_it->second;
      
      std::cout << "  Inserting pragma at line " << line.line_number 
               << ": " << pragma_it->second << "\n";
    }
  }
}

std::string SourceAnnotator::getIndentationForLine(unsigned line_number) {
  // find the line and extract its indentation
  for (const auto& line : annotated_lines_) {
    if (line.line_number == line_number) {
      std::string content = line.original_content;
      size_t first_non_space = content.find_first_not_of(" \t");
      
      if (first_non_space == std::string::npos) {
        // empty line or only whitespace
        return "    "; // 
      }
      
      return content.substr(0, first_non_space);
    }
  }
  
  return "    "; // 
}

std::string SourceAnnotator::generateOutputFilename(const std::string& input_filename) {
  // generate output filename by adding "_openmp" before the extension
  size_t dot_pos = input_filename.find_last_of('.');
  
  if (dot_pos != std::string::npos) {
    std::string base = input_filename.substr(0, dot_pos);
    std::string extension = input_filename.substr(dot_pos);
    return base + "_openmp" + extension;
  } else {
    return input_filename + "_openmp";
  }
}

} // namespace statik