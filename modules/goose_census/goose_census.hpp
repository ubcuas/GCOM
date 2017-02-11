#ifndef GOOSE_CENSUS_H
#define GOOSE_CENSUS_H
//===================================================================
// Includes
//===================================================================
#include <string>
#include <vector>
using namespace std;

//===================================================================
// Structs
//===================================================================
struct goose {
  int species;
  double xcoord;
  double ycoord;
  std::string filepath;
} ;

struct nest {
    double xcoord;
    double ycoord;
};

struct matingPair{
    goose maleGoose;
    goose femaleGoose;
    double distance;
    nest nestCentroid;
};

//===================================================================
// Class Declarations
//===================================================================
//Todo: Write fancy descriptions
class goose_census
{
public:
    goose_census();
    std::vector<goose> importCsvData();
    void matingPairsToCSV(std::vector<matingPair> matingPairs);
    std::vector<matingPair> getMatingPairsAllGeese(std::vector<goose> gooselist);
    void setup(double nestingDistance, std::string csvFilename);
    void geeseToNestCSV(std::vector<goose> gooseList);
private:
    double matingPairDistance = 17;
    std::string csvFilename = "matingPairs.csv";

    bool sameSpecies(goose Malegoose, goose Femalegoose);
    double gooseDistance(goose goose1, goose goose2);
    nest getCentroid(matingPair nestingPair);
    matingPair findClosestPair(std::vector<goose>& eligableGeese);
    std::vector<matingPair> getMatingPairs(std::vector<goose> eligableGeese, double minimumDistance);
    std::vector<goose> getSeparateSpecies(std::vector<goose> gooseList, int species);
    std::vector<int> getAllSpecies(std::vector<goose> gooseList);
};

#endif // GOOSE_CENSUS_H
