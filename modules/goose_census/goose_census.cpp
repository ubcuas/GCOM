#include "goose_census.hpp"
#include <iostream>
#include <fstream>
#include <QDebug>
#include <string>
using namespace std;

goose_census::goose_census()
{

}

//Calculates the distance between two geese based on their UTM coordinates
double goose_census::gooseDistance(goose goose1, goose goose2)
{
    double xDist = goose1.xcoord - goose2.xcoord;
    double yDist = goose1.ycoord - goose2.ycoord;
    double totalDist = sqrt(xDist*xDist + yDist*yDist);
    return totalDist;
}

//Returns all mating pairs in a vector of geese
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

//Returns the closest mating pair in a vetor of geese
matingPair goose_census::findClosestPair(vector<goose>& eligableGeese)
{
    matingPair closestPair;
    double shortestDistance = 100;
    std::vector<goose>::iterator matingFemale;
    std::vector<goose>::iterator matingMale;


    for(std::vector<goose>::iterator maleIterator = eligableGeese.begin(); maleIterator != eligableGeese.end(); ++maleIterator)
    {
        for(std::vector<goose>::iterator femaleIterator = maleIterator + 1;femaleIterator != eligableGeese.end(); ++femaleIterator)
        {
            if(shortestDistance > gooseDistance(*maleIterator,*femaleIterator))
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

    eligableGeese.erase(matingFemale);
    eligableGeese.erase(matingMale);
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
