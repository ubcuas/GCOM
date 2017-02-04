#include "goose_census.hpp"
#include <iostream>
#include <fstream>
#include <QDebug>
#include <string>
using namespace std;

goose_census::goose_census()
{

}

void goose_census::matingPairsToCSV(vector<matingPair> matingPairs)
{
    //Todo: Write a vector of mating pairs to a CSV file
}

vector<matingPair> goose_census::getMatingPairsAllGeese(vector<goose> gooseList)
{
    vector<matingPair> matingPairs;
    vector<matingPair> singleSpeciesMatingPairs;
    vector<goose> singleSpeciesGooseList;
    vector<int> speciesList = getAllSpecies(gooseList);
    for(std::vector<int>::iterator speciesIterator = speciesList.begin(); speciesIterator != speciesList.end(); ++speciesIterator)
    {
        singleSpeciesGooseList = getSeparateSpecies(gooseList,*speciesIterator);
        singleSpeciesMatingPairs = getMatingPairs(singleSpeciesGooseList, matingPairDistance);
        matingPairs.insert(matingPairs.end(),singleSpeciesMatingPairs.begin(), singleSpeciesMatingPairs.end());
    }

    return matingPairs;
}

//Calculates the distance between two geese based on their UTM coordinates
double goose_census::gooseDistance(goose goose1, goose goose2)
{
    double xDist = goose1.xcoord - goose2.xcoord;
    double yDist = goose1.ycoord - goose2.ycoord;
    double totalDist = sqrt(xDist*xDist + yDist*yDist);
    return totalDist;
}

// Gets a list of all the species contained in a list of geese
vector<int> goose_census::getAllSpecies(vector<goose> gooseList)
{
    vector<int> differentSpecies;
    goose tempGoose;
    for(std::vector<goose>::iterator    gooseIterator = gooseList.begin();
                                        gooseIterator != gooseList.end();
                                        ++gooseIterator)
    {
        tempGoose = *gooseIterator;

        if(!(std::find(differentSpecies.begin(), differentSpecies.end(),tempGoose.species)!=differentSpecies.end()))
        {
            differentSpecies.push_back(tempGoose.species);
        }
    }
    return differentSpecies;
}


//Creates a seperate list of geese of a certain species
vector<goose> goose_census::getSeparateSpecies(vector<goose> gooseList, int species)
{
    vector<goose> gooseSpecies;
    goose tempGoose;
    for(std::vector<goose>::iterator    gooseIterator = gooseList.begin();
                                        gooseIterator != gooseList.end();
                                        ++gooseIterator)
    {
        tempGoose = *gooseIterator;

        if(tempGoose.species == species)
        {
            gooseSpecies.push_back(tempGoose);
        }
    }
    return gooseSpecies;
}

// Returns all mating pairs in a vector of geese
// Doesn't discriminate based on species
// If interspecies mating is against your values, use getSeparateSpecies to get
// geese of a single species
vector<matingPair> goose_census::getMatingPairs(vector<goose> eligableGeese, double minimumDistance)
{
    vector<matingPair> matingPairs;
    matingPair tempPair;

    while (1) {
        if(eligableGeese.size() >=2 )
        {
            tempPair = findClosestPair(eligableGeese);
            if (tempPair.distance <= minimumDistance)
            {
                matingPairs.push_back(tempPair);
            }
            else
            {
                return matingPairs;
            }
        }
        else
        {
            return matingPairs;
        }

    }
}

// Calculates the centroid(nest) of a nesting pair
nest goose_census::getCentroid(matingPair nestingPair)
{
    nest matingPairNest;
    matingPairNest.xcoord = (nestingPair.maleGoose.xcoord + nestingPair.femaleGoose.xcoord)/2;
    matingPairNest.ycoord = (nestingPair.femaleGoose.ycoord + nestingPair.maleGoose.ycoord)/2;
    return matingPairNest;
}

bool goose_census::sameSpecies(goose Malegoose, goose Femalegoose)
{
    return (Malegoose.species == Femalegoose.species);
}

// Returns the closest mating pair in a vetor of geese
matingPair goose_census::findClosestPair(vector<goose>& eligableGeese)
{
    matingPair closestPair;
    double shortestDistance = -1;
    std::vector<goose>::iterator matingFemale;
    std::vector<goose>::iterator matingMale;


    for(std::vector<goose>::iterator    maleIterator = eligableGeese.begin();
                                        maleIterator != eligableGeese.end();
                                        ++maleIterator)
    {
        for(std::vector<goose>::iterator    femaleIterator = maleIterator + 1;
                                            femaleIterator != eligableGeese.end();
                                            ++femaleIterator)
        {
            if(shortestDistance == -1 || shortestDistance > gooseDistance(*maleIterator,*femaleIterator))
            {
                shortestDistance = gooseDistance(*maleIterator, *femaleIterator);

                closestPair.maleGoose = *maleIterator;
                closestPair.femaleGoose = *femaleIterator;
                closestPair.distance = shortestDistance;

                matingMale = maleIterator;
                matingFemale = femaleIterator;
            }

        }

    }

    //Remove mating pair from the dating pool
    eligableGeese.erase(matingFemale);
    eligableGeese.erase(matingMale);
    closestPair.nestCentroid = getCentroid(closestPair);
    return closestPair;
}

//Used to import my test geese coordinates in the form X, Y, Species, Number
//Coordinates are in UTM
vector<goose> goose_census::importCsvData()
{
    string line;
    vector<goose> gooseData;
    goose goosebuffer;
    int rowcount;
    std::string::size_type sz;

    ifstream myfile ("C:\\Users\\gnico_000\\Documents\\GitHub\\GCOM\\modules\\goose_census\\geeseCSV.csv");
    if (myfile.is_open())
      {
        getline(myfile,line);
        while(getline(myfile,line))
            {
            size_t pos = 0;
            string token;
            string delimiter = ",";
            rowcount = 0;
            while ((pos = line.find(delimiter)) != string::npos) {
                token = line.substr(0, pos);
                if (rowcount == 0)
                    goosebuffer.xcoord = std::stod(token,&sz);
                else if(rowcount == 1)
                    goosebuffer.ycoord = std::stod(token, &sz);
                else if(rowcount == 2)
                {
                    goosebuffer.species = std::stod(token, &sz);
                }
                line.erase(0, pos + delimiter.length());
                rowcount ++;
            }
            gooseData.push_back(goosebuffer);
            }
        myfile.close();
      }

    else qDebug() << "Unable to open file";

    return gooseData;
}
