#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <source_file>\n";
        return 1;
    }
    
    std::string filename = argv[1];
    std::cout << "Analyzing: " << filename << "\n";
    
    // TODO: actual analysis
    std::cout << "Analysis complete (placeholder)\n";
    
    return 0;
}