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
    
    // only show function info in verbose analysis mode
    if (verbose_ && !generate_pragmas_) {
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
    // mode 1: analysis-only without verbose - summary table
    if (!generate_pragmas_ && !verbose_) {
        loop_visitor_.printLoopSummary();
        return;
    }

    // mode 2: analysis-only with verbose - detailed analysis + summary
    if (!generate_pragmas_ && verbose_) {
        loop_visitor_.printLoopSummary();
        return;
    }

    // ,ode 3: pragma generation without verbose - clean pragma output
    if (generate_pragmas_ && !pragma_verbose_) {
        // just show the essential pragma generation info
        const auto& detected_loops = loop_visitor_.getLoops();
        if (detected_loops.empty()) {
            std::cout << "No loops detected - no pragma generation needed\n";
            return;
        }

        std::cout << "\n=== OpenMP Pragma Generation ===\n";
        
        // set up the pipeline quietly
        PragmaGenerator pragma_gen;
        PragmaLocationMapper location_mapper(&context_->getSourceManager());
        SourceAnnotator annotator(&context_->getSourceManager());

        // configure for clean output
        pragma_gen.setVerbose(false);
        pragma_gen.generatePragmasForLoops(detected_loops);
        
        // only map insertion points for parallelizable loops
        for (const auto& loop : detected_loops) {
            if (!loop.has_dependencies) {
                location_mapper.mapLoopToPragmaLocation(loop);
            }
        }

        // create the annotated source file (no if-check, since it’s void)
        annotator.annotateSourceWithPragmas(input_filename_,
                                            pragma_gen.getGeneratedPragmas(),
                                            location_mapper.getInsertionPoints());

        if (annotator.writeAnnotatedFile(output_filename_)) {
            std::cout << "Successfully created: " << output_filename_ << "\n";
            std::cout << "Compile with: gcc -fopenmp " << output_filename_ << "\n";
            
            // show clean pragma summary
            pragma_gen.printCleanSummary();
        } else {
            std::cerr << "Error: Failed to create output file\n";
        }

        std::cout << "===============================\n";
        return;
    }

    // mode 4: pragma generation with verbose - detailed pragma reasoning
    if (generate_pragmas_ && pragma_verbose_) {
        const auto& detected_loops = loop_visitor_.getLoops();
        if (detected_loops.empty()) {
            std::cout << "No loops detected - no pragma generation needed\n";
            return;
        }

        std::cout << "\n=== Detailed Pragma Generation ===\n";
        std::cout << "Creating OpenMP annotated file: " << output_filename_ << "\n";

        try {
            PragmaGenerator pragma_gen;
            PragmaLocationMapper location_mapper(&context_->getSourceManager());
            SourceAnnotator annotator(&context_->getSourceManager());

            // enable verbose mode for detailed output
            pragma_gen.setVerbose(true);
            pragma_gen.generatePragmasForLoops(detected_loops);

            // map insertion points
            for (const auto& loop : detected_loops) {
                if (!loop.has_dependencies) {
                    location_mapper.mapLoopToPragmaLocation(loop);
                }
            }

            // create annotated file
            annotator.annotateSourceWithPragmas(input_filename_,
                                                pragma_gen.getGeneratedPragmas(),
                                                location_mapper.getInsertionPoints());

            if (annotator.writeAnnotatedFile(output_filename_)) {
                std::cout << "\nSuccessfully created: " << output_filename_ << "\n";
                std::cout << "Compile with: gcc -fopenmp " << output_filename_ << "\n";
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