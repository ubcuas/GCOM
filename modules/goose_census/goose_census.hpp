#ifndef GOOSE_CENSUS_H
#define GOOSE_CENSUS_H
#include <QDebug>

struct goose {
  int species;
  double xcoord;
  double ycoord;
} ;

struct matingPair{
    goose maleGoose;
    goose femaleGoose;
    double distance;
};

class goose_census
{
public:
    goose_census();
    void countMatingPairs(std::vector<goose>& gooseList);
    std::vector<matingPair> getMatingPairs(std::vector<goose> eligableGeese, double minimumDistance);
    std::vector<goose> importCsvData();
    matingPair findClosestPair(std::vector<goose>& eligableGeese);
    double gooseDistance(goose goose1, goose goose2);
};

#endif // GOOSE_CENSUS_H
