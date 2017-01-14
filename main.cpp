#include "gcom_controller.hpp"
#include <QApplication>
#include "modules/goose_census/goose_census.hpp"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GcomController w;
    w.show();
    goose_census t;
    std::vector<goose> mg = t.importCsvData();
    std::vector<matingPair> mps = t.getMatingPairs(mg, 17);
    matingPair mp = t.findClosestPair(mg);
    qDebug() << mp.distance <<mp.femaleGoose.species <<mp.maleGoose.species;
    mp = t.findClosestPair(mg);
    qDebug() << mp.distance<<mp.femaleGoose.species <<mp.maleGoose.species;
    qDebug() << mps.size();
    return a.exec();
}
