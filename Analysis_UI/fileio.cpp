#include "fileio.h"

FileIO::FileIO()
{
    myfile = NULL;
    zData = NULL;
    colwise = 0;
    xMax = 0;
    xMin = 0;
    yMax = 0;
    yMin = 0;
    zMax = 0;
    zMin = 0;

    xSize = 0;
    ySize = 0;
}

FileIO::FileIO(QString filePath){
    this->filePath = filePath;
    myfile = new QFile(filePath);
    openState = myfile->open(QIODevice::ReadOnly);
}

FileIO::~FileIO(){
    delete myfile;
    //if( zData != NULL ) delete zData;
}

void FileIO::OpenCSVData(){

    this->colwise = 1;

    myfile->seek(0);

    QTextStream stream(myfile);
    QString line;


    // get number of rows;
    xSize = 0;
    while(stream.readLineInto(&line)){
        xSize ++;
        QStringList lineList = line.split(",");

        if( xSize == 1){ // get yDatax
            ySize = lineList.size()/2;
        }
    }
    xSize --;

    zData = new QVector<double> [xSize];
    double ** z ;
    z = new double *[ySize];
    for(int i = 0; i < ySize; i++){
        z[i] = new double [xSize];
    }

    //get data
    myfile->seek(0);
    int rows = 0;
    while(stream.readLineInto(&line)){
        rows ++;
        QStringList lineList = line.split(",");

        if( rows == 1){ // get yDatax
            for( int i = 1 ; i < lineList.size() ; i++ ){
                if( i % 2 == 1) {
                    double temp = GetYValue(lineList[i]);
                    yData.push_back(temp) ; // get data from string.
                }
            }
        }else{
            xData.push_back( (lineList[0]).toDouble() * 1e6);
            int yCount = 0;
            for( int i = 1 ; i < lineList.size() ; i++ ){
                if( i % 2 == 1){
                    z[yCount][rows-2] = (lineList[i]).toDouble() * 1000;
                    yCount ++;
                }
            }
        }
    }

    //transpose the zData
    zMin = z[0][0];
    zMax = z[0][0];
    for( int j = 0; j < ySize ; j ++){
        for(int i = 0; i < xSize ; i++){
            zData[j].push_back(z[j][i]);
            if( z[j][i] > zMax) zMax = z[j][i];
            if( z[j][i] < zMin) zMin = z[j][i];
        }
    }

    delete z;

    //check
    qDebug("X: %d , %d", xData.size(), xSize);
    qDebug("Y: %d , %d", yData.size(), ySize);

    xMin = FindMin(xData);
    xMax = FindMax(xData);

    yMin = FindMin(yData);
    yMax = FindMax(yData);

}

void FileIO::OpenTxtData_row(){

}

double FileIO::GetYValue(QString str){
    int pos = str.lastIndexOf("_") ;
    QString strY = str.mid(pos+1, 5);
    //qDebug() << str << ", " << pos << ", " << strY;
    return strY.toDouble();
}

double FileIO::FindMax(QVector<double> vec)
{
    double max = vec[0];
    for( int i = 1; i < vec.size(); i++){
        if( vec[i] > max) max = vec[i];
    }
    return max;
}

double FileIO::FindMin(QVector<double> vec)
{
    double min = vec[0];
    for( int i = 1; i < vec.size(); i++){
        if( vec[i] < min) min = vec[i];
    }
    return min;
}
