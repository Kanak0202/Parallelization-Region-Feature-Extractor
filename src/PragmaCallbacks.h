#ifndef PRAGMA_CALLBACKS_H
#define PRAGMA_CALLBACKS_H

#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <clang/Lex/MacroInfo.h>
#include "RegionDetector.h"

class PragmaCallbacks : public clang::PPCallbacks
{
private:

    RegionDetector &detector;

public:

    explicit PragmaCallbacks(RegionDetector &detector);

    void PragmaDirective(
        clang::SourceLocation Loc,
        clang::PragmaIntroducerKind Introducer) override;
    
    void MacroDefined(
        const clang::Token &MacroNameTok,
        const clang::MacroDirective *MD) override;
};

#endif
