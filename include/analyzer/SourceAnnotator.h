#pragma once

#include "clang/Basic/SourceManager.h"
#include "analyzer/PragmaGenerator.h"
#include "analyzer/PragmaLocationMapper.h"
#include <string>
#include <vector>
#include <fstream>

namespace statik {

struct AnnotatedLine {
    unsigned line_number;
    std::string original_content;
    std::string pragma_annotation;
    bool has_pragma;

    AnnotatedLine(unsigned line, const std::string& content)
        : line_number(line), original_content(content), has_pragma(false) {}
};

class SourceAnnotator {
public:
    explicit SourceAnnotator(clang::SourceManager* source_manager)
        : source_manager_(source_manager) {}

    void annotateSourceWithPragmas(const std::string& input_filename,
                                   const std::vector<GeneratedPragma>& pragmas,
                                   const std::vector<PragmaInsertionPoint>& insertion_points);

    bool writeAnnotatedFile(const std::string& output_filename);
    void printAnnotationSummary() const;

private:
    clang::SourceManager* source_manager_;
    std::vector<AnnotatedLine> annotated_lines_;
    std::string input_file_;

    bool readSourceFile(const std::string& filename);
    void insertPragmaAnnotations(const std::vector<GeneratedPragma>& pragmas,
                                 const std::vector<PragmaInsertionPoint>& insertion_points);
    std::string getIndentationForLine(unsigned line_number);
    std::string generateOutputFilename(const std::string& input_filename);
};

} // namespace statik
