#ifndef GOOSE_CENSUS_H
#define GOOSE_CENSUS_H
//===================================================================
// Includes
//===================================================================
#include <string>
#include <vector>
using namespace std;

//===================================================================
// Goose Structs
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
/*!
 * \brief The goose_census class is designed to take in coordinate (lat/long) and species
 * information of geese and calculate the locations of the nest.  It also converts the lat/long
 * coordinates to UTM so that a Ripley K script can be run.
 * \author Grant Nicol
 */
class goose_census
{
public:
    goose_census();


    /*!
     * \brief geeseToNestCSV uses getMatingPairsAllGeese and MatingPairToCSV to take a list
     * of geese and create a csv file containing the locations and filepaths of the image
     * containing the geese.
     *
     * \param gooseList
     */
    void geeseToNestCSV(std::vector<goose> gooseList);

    /*!
     * \brief getMatingPairsAllGeese finds all mating pairs of geese (CONOPPS defines a
     * mating pair as two geese of the same species within 3 meters of eachother) and
     * calculates the loaction of their nest.
     * \param gooselist, a list of the geese to be processed. This is to be generated from
     *  imagery analysis
     * \return a vector of the mating pairs
     */
    std::vector<matingPair> getMatingPairsAllGeese(std::vector<goose> gooselist);

    /*!
     * \brief matingPairsToCSV saves the list of mating pairs to a CSV which can be used in
     * the report
     * \param matingPairs
     */
    void matingPairsToCSV(std::vector<matingPair> matingPairs);

    /*!
     * \brief setup
     * \param nestingDistance is the distance that defines a mating pair. CONOPS specifies this
     * distance to be 3m
     * \param csvFilename is the name of the file you want to save your mating pairs to
     */
    void setup(double nestingDistance, std::string csvFilename);

    /*!
     * \brief importCsvData is used for testing purposes only. It reads a CSV file of
     * simulated goose information and returns a vector of geese
     * \return vector<goose>, a vector of geese. This is what is expected to be given to
     * the goose census in operating conditions.
     */
    std::vector<goose> importCsvData();

private:
    //Alma conversion values
    //Todo: Allow users to change values, add vancouver value
    double latitudeToMeters = 111200.98;
    double longitudeToMeters = 73827.95;
    double matingPairDistance = 17;
    std::string csvFilename = "matingPairs.csv";

    bool sameSpecies(goose Malegoose, goose Femalegoose);
    double gooseDistance(goose goose1, goose goose2);
    nest getCentroid(matingPair nestingPair);
    matingPair findClosestPair(std::vector<goose>& eligableGeese);
    std::vector<matingPair> getMatingPairs(std::vector<goose> eligableGeese, double minimumDistance);
    std::vector<goose> getSeparateSpecies(std::vector<goose> gooseList, int species);
    std::vector<int> getAllSpecies(std::vector<goose> gooseList);
    std::vector<goose> LatLongtoUTM(std::vector<goose> LatLongGeese);
};

#endif // GOOSE_CENSUS_H
