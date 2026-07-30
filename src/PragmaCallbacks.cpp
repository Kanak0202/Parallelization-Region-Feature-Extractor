#include "PragmaCallbacks.h"

PragmaCallbacks::PragmaCallbacks(
    RegionDetector &detector)
    : detector(detector)
{
}

void PragmaCallbacks::PragmaDirective(
    clang::SourceLocation Loc,
    clang::PragmaIntroducerKind Introducer)
{
    detector.handlePragma(Loc);
}

void PragmaCallbacks::MacroDefined(
    const clang::Token &MacroNameTok,
    const clang::MacroDirective *MD)
{
    detector.recordMacro(MacroNameTok.getLocation());
}