#include "analyzer/ASTVisitor.h"
#include "analyzer/PragmaGenerator.h"
#include "analyzer/PragmaLocationMapper.h"
#include "analyzer/SourceAnnotator.h"
#include "clang/AST/ASTContext.h"
#include <iostream>

using namespace clang;

namespace statik {

bool AnalyzerVisitor::VisitFunctionDecl(FunctionDecl* func) {
    if (!func || !func->hasBody()) {
        return true;
    }

    const std::string funcName = func->getNameAsString();
    SourceLocation loc = func->getLocation();
    std::cout << "Found function: " << funcName;
    if (loc.isValid()) {
        SourceManager& sm = context_->getSourceManager();
        unsigned line = sm.getSpellingLineNumber(loc);
        std::cout << " at line " << line;
    }
    std::cout << "\n";

    // Let the loop visitor handle loop detection
    loop_visitor_.TraverseStmt(func->getBody());
    return true;
}

void AnalyzerVisitor::runAnalysis() {
    // Always run the loop analysis and print summary
    loop_visitor_.printLoopSummary();
    
    // If pragma generation is enabled, run the full pipeline
    if (generate_pragmas_) {
        const auto& detected_loops = loop_visitor_.getLoops();
        
        if (detected_loops.empty()) {
            std::cout << "\nNo loops detected - no pragma generation needed\n";
            return;
        }
        
        std::cout << "\n=== Pragma Generation Pipeline ===\n";
        std::cout << "Creating OpenMP annotated file: " << output_filename_ << "\n";
        
        try {
            // Initialize the pragma generation components
            PragmaGenerator pragma_gen;
            PragmaLocationMapper location_mapper(&context_->getSourceManager());
            SourceAnnotator annotator(&context_->getSourceManager());
            
            // Generate pragmas for parallelizable loops
            pragma_gen.generatePragmasForLoops(detected_loops);
            
            // Map pragma insertion locations
            for (const auto& loop : detected_loops) {
                if (!loop.has_dependencies) {
                    location_mapper.mapLoopToPragmaLocation(loop);
                }
            }
            
            // Use the input filename passed from main
            std::string input_filename = input_filename_;
            
            if (input_filename.empty()) {
                std::cerr << "Error: Could not determine input filename\n";
                return;
            }
            
            // Create annotated source file
            annotator.annotateSourceWithPragmas(input_filename,
                                               pragma_gen.getGeneratedPragmas(),
                                               location_mapper.getInsertionPoints());
            
            // Write the output file with OpenMP header
            if (annotator.writeAnnotatedFile(output_filename_)) {
                std::cout << "Successfully created: " << output_filename_ << "\n";
                std::cout << "Note: Compile with 'gcc -fopenmp " << output_filename_ 
                         << "' for OpenMP support\n";
                
                // Show summary
                pragma_gen.printPragmaSummary();
                annotator.printAnnotationSummary();
            } else {
                std::cerr << "Error: Failed to create output file\n";
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in pragma generation: " << e.what() << "\n";
        }
        
        std::cout << "=================================\n";
    }
}

} // namespace statik