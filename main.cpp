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
    std::vector<matingPair> mp = t.getMatingPairsAllGeese(mg);
    qDebug() << mp.size();
    //qDebug() << mps;
    //matingPair mp = t.findClosestPair(mp);
    //qDebug() << mp.distance <<mp.femaleGoose.species <<mp.maleGoose.species;
    return a.exec();
}
