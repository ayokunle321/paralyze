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
    
    if (verbose_) {
        SourceLocation loc = func->getLocation();
        std::cout << "\nFound function: " << funcName;
        if (loc.isValid()) {
            SourceManager& sm = context_->getSourceManager();
            unsigned line = sm.getSpellingLineNumber(loc);
            std::cout << " at line " << line;
        }
        std::cout << "\n";
    }

    loop_visitor_.TraverseStmt(func->getBody());
    return true;
}

void AnalyzerVisitor::runAnalysis() {
    // Always show the analysis results first
    loop_visitor_.printLoopSummary();
    
    // Pragma generation is optional
    if (generate_pragmas_) {
        const auto& detected_loops = loop_visitor_.getLoops();
        
        if (detected_loops.empty()) {
            if (verbose_) {
                std::cout << "\nNo loops detected - no pragma generation needed\n";
            }
            return;
        }
        
        if (verbose_) {
            std::cout << "\n=== Pragma Generation Pipeline ===\n";
            std::cout << "Creating OpenMP annotated file: " << output_filename_ << "\n";
        }
        
        try {
            // Set up the pragma generation pipeline
            PragmaGenerator pragma_gen;
            PragmaLocationMapper location_mapper(&context_->getSourceManager());
            SourceAnnotator annotator(&context_->getSourceManager());
            
            // Generate pragmas for all loops (safe and unsafe)
            pragma_gen.generatePragmasForLoops(detected_loops);
            
            // Only map insertion points for parallelizable loops
            for (const auto& loop : detected_loops) {
                if (!loop.has_dependencies) {
                    location_mapper.mapLoopToPragmaLocation(loop);
                }
            }
            
            std::string input_filename = input_filename_;
            
            if (input_filename.empty()) {
                std::cerr << "Error: Could not determine input filename\n";
                return;
            }
            
            // Create the annotated source file
            annotator.annotateSourceWithPragmas(input_filename,
                                               pragma_gen.getGeneratedPragmas(),
                                               location_mapper.getInsertionPoints());
            
            if (annotator.writeAnnotatedFile(output_filename_)) {
                if (verbose_) {
                    std::cout << "Successfully created: " << output_filename_ << "\n";
                    std::cout << "Note: Compile with 'gcc -fopenmp " << output_filename_ 
                             << "' for OpenMP support\n";
                    
                    pragma_gen.printPragmaSummary();
                    annotator.printAnnotationSummary();
                }
            } else {
                std::cerr << "Error: Failed to create output file\n";
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in pragma generation: " << e.what() << "\n";
        }
        
        if (verbose_) {
            std::cout << "=================================\n";
        }
    }
}

} // namespace statik