#include <iostream>
#include <string>
#include <vector>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <source_file>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -h, --help     Show this help message\n";
    std::cout << "  -v, --version  Show version information\n";
    std::cout << "  --verbose      Enable verbose output\n";
}

void printVersion() {
    std::cout << "Statik v1.0.0\n";
    std::cout << "Static analysis tool for loop parallelization\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string filename;
    bool verbose = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg[0] == '-') {
            std::cout << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            filename = arg;
        }
    }
    
    if (filename.empty()) {
        std::cout << "Error: No input file specified\n";
        printUsage(argv[0]);
        return 1;
    }
    
    if (verbose) {
        std::cout << "Verbose mode enabled\n";
    }
    
    std::cout << "Analyzing: " << filename << "\n";
    
    // TODO: actual analysis with LibTooling
    std::cout << "Analysis complete (placeholder)\n";
    
    return 0;
}