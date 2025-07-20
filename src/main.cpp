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

class AnalyzerAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler,
                                                   StringRef file) override {
        return std::make_unique<statik::AnalyzerConsumer>(&compiler.getASTContext());
    }
};

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <source_file>\n";
    std::cout << "\nOptions:\n";
    std::cout << " -h, --help     Show this help message\n";
    std::cout << " -v, --version  Show version information\n";
    std::cout << " --verbose      Enable verbose output\n";
}

void printVersion() {
    std::cout << "Statik v1.0.0\n";
    std::cout << "Static analysis tool for loop parallelization\n";
}

int main(int argc, char** argv) {
    // Handle help/version before tooling
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        }
    }

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    // Take the last argument as filename
    std::string filename = argv[argc - 1];
    std::cout << "Analyzing: " << filename << "\n";

    // Create a simple tool without FixedCompilationDatabase
    std::vector<std::string> sources = {filename};
    
    // Use runToolOnCode instead of ClangTool for simpler setup
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }
    
    std::string source_code((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();
    
    auto tool = newFrontendActionFactory<AnalyzerAction>();
    
    // Run directly on the source code
    bool result = runToolOnCode(tool->create(), source_code, filename);
    
    std::cout << "Analysis complete\n";
    return result ? 0 : 1;
}