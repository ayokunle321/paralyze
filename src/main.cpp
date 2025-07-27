#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Frontend/FrontendActions.h"
#include "analyzer/ASTVisitor.h"

using namespace clang;
using namespace clang::tooling;

// Global flags
bool generate_pragmas = false;
bool verbose_mode = false;
std::string output_filename;

std::string generateOutputFilename(const std::string& input_file) {
    // Find the last dot to get the extension
    size_t dot_pos = input_file.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string base = input_file.substr(0, dot_pos);
        std::string extension = input_file.substr(dot_pos);
        return base + "_openmp" + extension;
    } else {
        return input_file + "_openmp";
    }
}

class AnalyzerAction : public ASTFrontendAction {
private:
    bool generate_pragmas_;
    std::string output_filename_;
    std::string input_filename_;
    bool verbose_;
    
public:
    AnalyzerAction(bool gen_pragmas = false, const std::string& output = "", 
                   const std::string& input = "", bool verbose = false)
        : generate_pragmas_(gen_pragmas), output_filename_(output), 
          input_filename_(input), verbose_(verbose) {}
        
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler,
                                                   StringRef file) override {
        auto consumer = std::make_unique<statik::AnalyzerConsumer>(&compiler.getASTContext());
        
        if (generate_pragmas_) {
            consumer->enablePragmaGeneration(output_filename_, input_filename_);
        }
        
        consumer->setVerbose(verbose_);
        
        return consumer;
    }
};

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <source_file>\n";
    std::cout << "\nModes:\n";
    std::cout << "  Default: Analysis only (safe, no file changes)\n";
    std::cout << "  --generate-pragmas: Create <filename>_openmp.c with OpenMP pragmas\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -h, --help     Show this help message\n";
    std::cout << "  -v, --version  Show version information\n";
    std::cout << "  --verbose      Show detailed analysis information\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << progName << " code.c                    # Analysis only\n";
    std::cout << "  " << progName << " --verbose code.c          # Detailed analysis\n";
    std::cout << "  " << progName << " --generate-pragmas code.c # Creates code_openmp.c\n";
}

void printVersion() {
    std::cout << "Statik v1.0.0\n";
    std::cout << "Static analysis tool for loop parallelization\n";
}

bool parseArgs(int argc, char** argv, std::string& input_file) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return false;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return false;
        } else if (arg == "--generate-pragmas") {
            generate_pragmas = true;
        } else if (arg == "--verbose") {
            verbose_mode = true;
        } else if (!arg.empty() && arg[0] != '-') {
            // This is the input file
            input_file = arg;
        }
    }
    
    if (input_file.empty()) {
        std::cerr << "Error: No input file specified\n";
        return false;
    }
    
    return true;
}

int main(int argc, char** argv) {
    std::string input_file;
    
    if (argc < 2 || !parseArgs(argc, argv, input_file)) {
        if (argc >= 2) return 0; // Help/version was shown
        printUsage(argv[0]);
        return 1;
    }

    if (generate_pragmas) {
        output_filename = generateOutputFilename(input_file);
        std::cout << "Mode: Pragma generation\n";
        std::cout << "Input: " << input_file << "\n";
        std::cout << "Output: " << output_filename << "\n";
    } else {
        std::cout << "Mode: Analysis only\n";
    }
    
    std::cout << "Analyzing: " << input_file << "\n";

    // Read source file
    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << input_file << std::endl;
        return 1;
    }
    
    std::string source_code((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();
    
    auto tool = newFrontendActionFactory<AnalyzerAction>();
    
    // Create action with pragma generation settings
    std::unique_ptr<FrontendAction> action;
    if (generate_pragmas) {
        action = std::make_unique<AnalyzerAction>(true, output_filename, input_file, verbose_mode);
    } else {
        action = std::make_unique<AnalyzerAction>(false, "", "", verbose_mode);
    }
    
    // Run analysis (and pragma generation if enabled)
    bool result = runToolOnCode(std::move(action), source_code, input_file);
    
    if (!result) {
        std::cerr << "Analysis failed\n";
        return 1;
    }
    
    std::cout << "Analysis complete\n";
    return 0;
}