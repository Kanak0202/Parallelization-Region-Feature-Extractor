#ifndef PRAGMA_CALLBACKS_H
#define PRAGMA_CALLBACKS_H

#include <clang/Lex/PPCallbacks.h>

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
};

#endif
