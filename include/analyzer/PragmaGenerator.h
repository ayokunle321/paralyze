#pragma once

#include "analyzer/LoopInfo.h"
#include "analyzer/ConfidenceScorer.h"
#include <vector>
#include <string>
#include <memory>

namespace statik {

// types of OpenMP pragmas
enum class PragmaType {
    NO_PRAGMA,
    PARALLEL_FOR,
    PARALLEL_FOR_SIMD,
    SIMD
};

// representation of a pragma for a loop
struct GeneratedPragma {
    PragmaType type;
    std::string pragma_text;
    std::string loop_type;
    unsigned line_number;
    std::string reasoning;
    bool requires_private_vars = false;
    std::vector<std::string> private_variables;
    ConfidenceScore confidence;

    GeneratedPragma(PragmaType t, const std::string& text, const std::string& ltype,
                    unsigned line, const std::string& reason)
        : type(t), pragma_text(text), loop_type(ltype), line_number(line), reasoning(reason) {}
};

class PragmaGenerator {
public:
    PragmaGenerator() : confidence_scorer_(std::make_unique<ConfidenceScorer>()) {}

    void generatePragmasForLoops(const std::vector<LoopInfo>& loops);
    void printCleanSummary() const;
    void printPragmaSummary() const;

    const std::vector<GeneratedPragma>& getGeneratedPragmas() const { 
        return generated_pragmas_; 
    }

    void setVerbose(bool verbose) { verbose_ = verbose; }

private:
    std::vector<GeneratedPragma> generated_pragmas_;
    std::unique_ptr<ConfidenceScorer> confidence_scorer_;
    bool verbose_ = false;

    PragmaType determinePragmaType(const LoopInfo& loop);
    std::string generatePragmaText(PragmaType type, const LoopInfo& loop);
    std::string generateReasoning(PragmaType type, const LoopInfo& loop);
    bool shouldUseSimd(const LoopInfo& loop);
    bool hasSimpleArrayAccess(const LoopInfo& loop);
    bool isInnerLoop(const LoopInfo& loop);
    std::vector<std::string> identifyPrivateVariables(const LoopInfo& loop);
};

} // namespace statik
