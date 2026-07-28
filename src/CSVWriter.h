// CSVWriter.h
#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <string>
#include "FeatureVector.h"

void appendRegionToCSV(const std::string &csvPath,
                        unsigned regionId,
                        const std::string &fileName,
                        const FeatureVector &f);

#endif