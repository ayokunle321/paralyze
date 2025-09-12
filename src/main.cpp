#include "analyzer/ASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

using namespace clang;
using namespace clang::tooling;

// global flags for mode
bool generate_pragmas = false;
bool verbose_mode = false;
std::string output_filename;

// generate output filename by adding "_openmp" before extension
std::string generateOutputFilename(const std::string& input_file)
{
  size_t dot_pos = input_file.find_last_of('.');
  if (dot_pos != std::string::npos)
  {
    std::string base = input_file.substr(0, dot_pos);
    std::string extension = input_file.substr(dot_pos);
    return base + "_openmp" + extension;
  }
  else
  {
    return input_file + "_openmp";
  }
}

// clang frontend action that sets up our analyzer
class AnalyzerAction : public ASTFrontendAction
{
private:
  bool generate_pragmas_;
  std::string output_filename_;
  std::string input_filename_;
  bool verbose_;

public:
  AnalyzerAction(bool gen_pragmas = false, const std::string& output = "",
                 const std::string& input = "", bool verbose = false)
      : generate_pragmas_(gen_pragmas), output_filename_(output), input_filename_(input),
        verbose_(verbose)
  {
  }

  bool BeginSourceFileAction(CompilerInstance& compiler) override
  {
    // suppress clang diagnostics
    DiagnosticsEngine& diags = compiler.getDiagnostics();
    diags.setSuppressAllDiagnostics(true);
    return ASTFrontendAction::BeginSourceFileAction(compiler);
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler,
                                                 StringRef file) override
  {
    auto consumer = std::make_unique<paralyze::AnalyzerConsumer>(&compiler.getASTContext());

    if (generate_pragmas_)
    {
      consumer->enablePragmaGeneration(output_filename_, input_filename_);
      consumer->setVerbose(false);
      consumer->setPragmaVerbose(verbose_);
    }
    else
    {
      consumer->setVerbose(verbose_);
      consumer->setPragmaVerbose(false);
    }

    return consumer;
  }
};

// print help message
void printUsage(const char* progName)
{
  std::cout << "PARALYZE - Static Analysis Tool for Loop Parallelization\n\n";
  std::cout << "Usage: " << progName << " [OPTIONS] <source_file>\n\n";
  std::cout << "MODES:\n";
  std::cout << "  Analysis Only (default)\n";
  std::cout << "    " << progName << " code.c\n";
  std::cout << "    └─ Shows summary table of loop parallelization safety\n\n";
  std::cout << "  Pragma Generation\n";
  std::cout << "    " << progName << " --generate-pragmas code.c\n";
  std::cout << "    └─ Creates code_openmp.c with OpenMP pragmas inserted\n\n";
  std::cout << "OPTIONS:\n";
  std::cout << "  --generate-pragmas    Generate OpenMP pragma annotations\n";
  std::cout << "  --verbose            Show detailed analysis information\n";
  std::cout << "  -h, --help           Show this help message\n";
  std::cout << "  -v, --version        Show version information\n\n";
}

// print version info
void printVersion()
{
  std::cout << "PARALYZE v1.0.0\n";
  std::cout << "Static analysis tool for automatic OpenMP parallelization\n";
  std::cout << "Built with Clang/LLVM\n";
}

// show which mode we’re running in and relevant files
void printModeInfo(const std::string& input_file)
{
  if (generate_pragmas)
  {
    if (verbose_mode)
    {
      std::cout << "Mode: Pragma Generation (Verbose)\n";
      std::cout << "      ├─ Input:  " << input_file << "\n";
      std::cout << "      ├─ Output: " << output_filename << "\n";
      std::cout << "      └─ Shows:  Detailed pragma reasoning & confidence scores\n\n";
    }
    else
    {
      std::cout << "Mode: Pragma Generation (Clean)\n";
      std::cout << "      ├─ Input:  " << input_file << "\n";
      std::cout << "      ├─ Output: " << output_filename << "\n";
      std::cout << "      └─ Shows:  Essential pragma generation info only\n\n";
    }
  }
  else
  {
    if (verbose_mode)
    {
      std::cout << "Mode: Analysis Only (Verbose)\n";
      std::cout << "      ├─ Input:  " << input_file << "\n";
      std::cout << "      └─ Shows:  Summary table + detailed dependency analysis\n\n";
    }
    else
    {
      std::cout << "Mode: Analysis Only (Clean)\n";
      std::cout << "      ├─ Input:  " << input_file << "\n";
      std::cout << "      └─ Shows:  Summary table only\n\n";
    }
  }
}

// parse CLI args and set global flags
bool parseArgs(int argc, char** argv, std::string& input_file)
{
  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help")
    {
      printUsage(argv[0]);
      return false;
    }
    else if (arg == "-v" || arg == "--version")
    {
      printVersion();
      return false;
    }
    else if (arg == "--generate-pragmas")
    {
      generate_pragmas = true;
    }
    else if (arg == "--verbose")
    {
      verbose_mode = true;
    }
    else if (!arg.empty() && arg[0] != '-')
    {
      input_file = arg;
    }
    else
    {
      std::cerr << "Error: Unknown option '" << arg << "'\n";
      std::cerr << "Use --help for usage information.\n";
      return false;
    }
  }

  if (input_file.empty())
  {
    std::cerr << "Error: No input file specified\n";
    std::cerr << "Use --help for usage information.\n";
    return false;
  }

  return true;
}

int main(int argc, char** argv)
{
  std::string input_file;

  // parse args, show help/version if needed
  if (argc < 2 || !parseArgs(argc, argv, input_file))
  {
    if (argc >= 2)
      return 0;
    printUsage(argv[0]);
    return 1;
  }

  // generate output filename if pragmas are enabled
  if (generate_pragmas)
  {
    output_filename = generateOutputFilename(input_file);
  }

  // show current mode
  printModeInfo(input_file);

  // check if file exists
  std::ifstream file(input_file);
  if (!file.is_open())
  {
    std::cerr << "Error: Could not open file '" << input_file << "'\n";
    return 1;
  }

  // read source code into string
  std::string source_code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  // create analyzer frontend action
  std::unique_ptr<FrontendAction> action =
      std::make_unique<AnalyzerAction>(generate_pragmas, output_filename, input_file, verbose_mode);

  // run clang tooling
  bool result = runToolOnCode(std::move(action), source_code, input_file);

  if (!result)
  {
    std::cerr << "\nAnalysis failed. Check your input file for syntax errors.\n";
    return 1;
  }

  std::cout << "\nAnalysis completed successfully.\n";
  return 0;
}
