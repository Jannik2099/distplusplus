#pragma once

#include <array>

namespace distplusplus::common {

enum class Language {
    NONE,
    C,
    CXX,
};

[[maybe_unused]] static constexpr const char *language_to_string(Language language) {
    switch (language) {
    case Language::NONE:
        return "none";
    case Language::C:
        return "C";
    case Language::CXX:
        return "CXX";
    }
}

// This is for flags that ONLY affect the preprocessor
// gcc flags from https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html

static constexpr std::array singleArgsCPP [[maybe_unused]] = {
    "-C",
    "-CC",
    "-dD",
    "-dI",
    "-dM",
    "-dN",
    "-dU",
    "-fdebug-cpp",
    "-fdirectives-only",
    "-fdollars-in-identifiers",
    "-fextended-identifiers",
    "-flarge-source-files",
    "-fmax-include-depth",
    "-fno-canonical-system-headers",
    "-fpch-deps",
    "-fpch-preprocess",
    "-fpreprocessed",
    "-ftrack-macro-expansion",
    "-fworking-directory",
    "-H",
    "-M",
    "-MD",
    "-MG",
    "-MM",
    "-MMD",
    "-MP",
    "-Mno-modules",
    "-no-integrated-cpp",
    "-P",
    "-pthread",
    "-remap",
    "-traditional",
    "-traditional-cpp",
    "-trigraphs",
    "-undef",
};

static constexpr std::array multiArgsCPP [[maybe_unused]] = {
    "-imacros", "-include", "-Xpreprocessor", "-MT",      "-MF",        "-MQ", "-I",
    "-D",       "-U",       "-iquote",        "-isystem", "-idirafter",
};

static constexpr std::array singleArgsCPPStartsWith [[maybe_unused]] = {
    "-I",
    "-D",
    "-U",
    "-Wp,",
};

static constexpr std::array multiArgsNoDistribute [[maybe_unused]] = {
    "-dumpbase",
    "-dumpbase-ext",
    "-dumpdir",
};

static constexpr std::array singleArgsNoDistribute [[maybe_unused]] = {
    "-M",
    "-MM",
    "-E",
    "-dr",
    "-Wa,-a",
    "-Wa,--MD",
    "-fbranch-probabilities",
    "--coverage",
    "-fprofile-arcs",
    "-ftest-coverage",
    "-include-pch",
};

static constexpr std::array singleArgsNoDistributeStartsWith [[maybe_unused]] = {
    "-fplugin=",
    "-specs=",
    "-ffile-prefix-map=",
    "-fprofile-use",
    "-fauto-profile",
    "-finstrument-functions-exclude",
    "-save-temps",
    "-time",
    "-fdump",
    "-fdebug-prefix-map=",
    "-fprofile-prefix-map=",
    "-fmacro-prefix-map=",
};

static constexpr std::array inputFileExtensionC [[maybe_unused]] = {
    ".c",
    ".i",
};

static constexpr std::array inputFileExtensionCXX [[maybe_unused]] = {
    ".C", ".cc", ".cxx", ".cpp", ".c++", ".cp", ".CPP", ".ii",
};

// this could be computed via constexpr, but that's a LOT of boilerplate
static constexpr std::array inputFileExtension
    [[maybe_unused]] = {".c", ".C", ".cc", ".cxx", ".cpp", ".c++", ".cp", ".CPP", ".i", ".ii"};

static constexpr std::array xArgsC [[maybe_unused]] = {
    "c",
    "cpp-output",
};

static constexpr std::array xArgsCXX [[maybe_unused]] = {
    "c++",
    "c++-cpp-output",
};

} // namespace distplusplus::common
