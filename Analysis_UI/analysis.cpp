#include "analysis.h"

Analysis::Analysis(){
    Initialize();
}

Analysis::Analysis(const QVector<double> x, const QVector<double> y)
{
    Initialize();
    SetData(x,y);
}

Analysis::~Analysis(){

}

void Analysis::Initialize(){
    SSR = 0;
    n = 0;
    p = 0;
    DF = 0;
    fitFlag = -1;
    mean = 0;
    var = 0;

    nIter = 0;
    MaxIter = 300;

    torr = 1e-6;
    delta = 0;
    lambda = 0;

    yIndex = 0;
    yValue = 0;

    zMin = 0;
    zMax = 0;

    startIndex = 0;
    endIndex = 0;

    fitMsg = "";

    fitFuncID = 0;

}

void Analysis::setFunctionType(int fitFuncID)
{
    this->fitFuncID = fitFuncID;
}

void Analysis::setFunctionExpression(QString str)
{
    this->function_str = str;
}

void Analysis::setFunctionGradExpression(QStringList str_list)
{
    this->function_grad_str = str_list;
}

void Analysis::setEngineParameter(QVector<double> par)
{
    for(int i = 0; i < par.size(); i++){
        QString py = "p" + QString::number(i);
        engine.globalObject().setProperty(py, par[i]);

        //qDebug() << engine.globalObject().property(py).toNumber();
    }
}

void Analysis::SetData(const QVector<double> x, const QVector<double> y)
{
    if( x.size() != y.size() ){
        Msg.sprintf("The sizes of input data x and y are not match. Size(x) = %d, Size(y) = %d", x.size(), y.size());
        SendMsg(Msg);
        return;
    }
    this->xdata = x;
    this->zdata = y;

    this->n = this->xdata.size();

    //finding local zMin, zMax;
    zMin = zdata[0];
    zMax = zdata[0];
    zMinIndex = 0;
    zMaxIndex = 0;
    for(int i = 1; i < n; i++ ){
        if( zMin > zdata[i] ) {
            zMin = zdata[i];
            zMinIndex = i;
        }
        if( zMax < zdata[i] ) {
            zMax = zdata[i];
            zMaxIndex = i;
        }
    }

    int x1 = FindXIndex(TIME2);
    QVector<double> mv = MeanAndvariance(0, x1);
    this->mean = mv[0];
    this->var  = mv[1];
}

QVector<double> Analysis::MeanAndvariance(int index_1, int index_2)
{
    QVector<double> output = {0,0};

    if( this->n == 0 ||
        index_1 < 0 ||
        index_2 < 0 ||
        index_1 > this->n ||
        index_1 >= index_2 ||
        index_2 > this->n){
        Msg.sprintf("index Error. n = %d, range = (%d, %d)", n, index_1, index_2);
        emit SendMsg(Msg);
        return output;
    }

    output.clear();

    int size = index_2 - index_1 + 1;
    double mean = 0;
    for( int i = index_1 ; i <= index_2 ; i++){
        mean += (this->zdata)[i];
    }
    mean = mean / size;
    output.push_back(mean);

    double var = 0;
    for( int i = index_1 ; i <= index_2 ; i++){
        var += pow((this->zdata)[i]-mean,2);
    }
    var = var / (size-1);
    output.push_back(var);

    //Msg.sprintf("From index %d to %d (%d data)\nMean = %f, Variance = %f, sigma = %f", index_1, index_2, size, mean, var, sqrt(var));
    //SendMsg(Msg);
    return output;

}

int Analysis::Regression(QVector<double> par0)
{
    //Levenberg-Marquardt Algorithm
    fitFlag = 0;
    fitMsg = "";
    int xStart = this->startIndex;
    //int xEnd = this->n - 1;
    int xEnd = this->endIndex;
    int fitSize = xEnd - xStart + 1;

    this->p = par0.size();
    this->DF = fitSize - this->p;
    Matrix par_old(p,1); for(int i = 0; i < p; i++){ par_old(i+1,1) = par0[i];}

    //============================Start regression
    Matrix Y(fitSize,1);
    Matrix f(fitSize,1);
    Matrix F(fitSize,p); // F = grad(f)
    if(fitFuncID == 2) setEngineParameter(par0);
    for(int i = 1; i <= fitSize ; i++) {
        Y(i,1) = zdata[i + xStart - 1];
        double x = xdata[i + xStart - 1];
        f(i,1) = FitFunc(x, par0, fitFuncID);
        QVector<double> gradf = GradFitFunc(x, par0, fitFuncID);
        for(int j = 1; j <= this->p; j++){
            F(i,j) = gradf[j-1];
        }
    }

    Matrix Ft = F.Transpose();
    Matrix FtF = Ft*F;

    if( this->lambda == -1){ // the default initial value is -1
        this->lambda = 0;
        for(int i = 1; i <= fitSize; i++){
            for(int j = 1; j <= p ; j++){
                this->lambda += F(i,j)* F(i,j);
            }
        }

        this->lambda = sqrt(this->lambda/fitSize/p);
        //Msg.sprintf(" ==== cal inital lambda : %f", this->lambda);
        //SendMsg(Msg);
    }
    Matrix DI(p,p);
    for(int i = 1; i <= p ; i++) {
        DI(i,i) = this->lambda;
    }

    try{
        this->CoVar = (FtF + DI).Inverse();
    }catch( Exception err){
        fitFlag = 1;
        fitMsg += "fitFlag += 0001; Fail to calculate covariance. \n";
        return 0; // return 1 when covariance cannot be compute.
    }

    Matrix dY = f-Y;
    this->SSR = (dY.Transpose() * dY)(1,1);

    Matrix FtdY = Ft*dY; // gradient of SSR
    Matrix dpar = CoVar * FtdY;
    Matrix sol = par_old + dpar;

    //========================================== Check the SSR(p+dpar)
    QVector<double> par_new = sol.Matrix2QVec();

    Matrix fn(fitSize, 1);
    Matrix Fn(fitSize, p);
    if(fitFuncID == 2) setEngineParameter(par_new);
    for(int i = 1; i <= fitSize ; i++) {
        double x = xdata[i + xStart - 1];
        fn(i,1) = FitFunc(x, par_new, fitFuncID);
        QVector<double> gradf = GradFitFunc(x, par_new, fitFuncID);
        for(int j = 1; j <= p; j++){
            Fn(i,j) = gradf[j-1];
        }
    }
    Matrix dYn = fn-Y;
    double SSRn = (dYn.Transpose() * dYn)(1,1);

    this->delta = SSRn - this->SSR;

    //========== SSRn > SSR
    if( this->delta >= 0){
        this->lambda = this->lambda * 10;
        sol = par_old;
        this->sol = sol.Matrix2QVec();
        this->dpar = Matrix(p,1).Matrix2QVec(); ///Zero matrix
        this->gradSSR = FtdY.Matrix2QVec();
        return 0;
    }
    //========== SSRn < SSR

    this->lambda = this->lambda / 10;
    this->sol = sol.Matrix2QVec();
    this->dpar = dpar.Matrix2QVec();
    this->SSR = SSRn;
    //new gradient of SSR
    this->gradSSR = (Fn.Transpose()*dYn).Matrix2QVec();

    // replace CoVar
    try{
        this->CoVar = (Fn.Transpose()*Fn).Inverse();
    }catch( Exception err){
        fitFlag = 1;
        fitMsg += "fitFlag += 0001; Fail to calculate covariance. \n";
        return 0; // return 0 when covariance cannot be compute.
    }

    return 1;
}

int Analysis::LMA( QVector<double> par0, double lambda0){

    this->lambda = lambda0;

    QString tmp;
    PrintVector(par0, "ini. par:");

    tmp.sprintf("Fit Torr: %7.1e, Max Iteration: %d", torr, MaxIter);
    SendMsg(tmp);
    tmp.sprintf("Fit X-index (%d, %d) = (%f, %f) us ; DF = %d", startIndex, endIndex, xdata[startIndex], xdata[endIndex], DF);
    SendMsg(tmp);

    nIter = 0;
    QVector<double> par = par0;
    bool contFlag = true;
    Msg.sprintf(" === Start fit using Levenberg-Marquardt Algorithm: ");
    while(contFlag){
        Regression(par);
        par = this->sol;
        nIter ++;
        if( nIter >= MaxIter ) {
            fitFlag = 2; // fitFlag = 2 when iteration too many
            fitMsg += "fitFlag += 0010; Fail to converge. \n";
            break;
        }
        bool converge = 0;
        //since this is 4-parameter fit
        converge = std::abs(this->delta) <  torr * SSR;
        for(int i = 0; i < p; i++){
            converge &= std::abs(this->gradSSR[i]) < TORRGRAD;
        }
        // if lambda to small or too big, reset
        //if( this->lambda < 1e-5) this->lambda = 1e+5;
        //if( this->lambda > 1e+10) this->lambda = 1e-4;
        contFlag = fitFlag == 0 && ( !converge );
        if( fitFuncID == 2){
            QEventLoop eventloop;
            QTimer::singleShot(10, &eventloop, SLOT(quit()));
            SendMsg("Number of Iteration : " + QString::number(nIter));
            eventloop.exec();
        }
        //qDebug() << "Number of Iteration : " << QString::number(nIter);
        qDebug() << par;
        qDebug() << this->delta;
        qDebug() << this->gradSSR;
    };

    tmp.sprintf(" %d", nIter);
    Msg += tmp;
    if( fitFlag == 0) {
        Msg += "| End Normally.";
        fitMsg += "fitFlag += 0000; Fit ended. Covariance OK. Converged.\n";
    }else if(fitFlag == 1){
        Msg += "| Terminated. Covariance fail to cal.";
    }else if(fitFlag == 2){
        tmp.sprintf("| Terminated. Fail to converge in %d trials.", MaxIter);
        Msg += tmp;
    }
    SendMsg(Msg);

    //===== cal error
    double fitVar = this->SSR / this->DF;
    Matrix error(p,1);
    for( int i = 1; i <= p ; i++){
        error(i,1)  = sqrt(fitVar * CoVar(i,i));
    }
    Matrix pValue(p,1);
    for( int i = 1; i <= p ; i++){
        double tDis = sol[i-1]/error(i,1);
        pValue(i,1) = cum_tDis30(- std::abs(tDis));
    }

    this->error = error.Matrix2QVec();
    this->pValue = pValue.Matrix2QVec();

    return 0;
}

int Analysis::GnuFit(QVector<double> par)
{   // using gnuplot to fit and read the gnufit.log
    // need the "text.dat", which is condensed from *.cvs

    PrintVector(par, "ini. par:");
    Msg.sprintf(" === Start fit using gnuplot, also Levenberg-Marquardt Algorithm. ");
    SendMsg(Msg);

    QString cmd;
    cmd.sprintf("gnuplot -e \"yIndex=%d;a=%f;Ta=%f;b=%f;Tb=%f;startX=%d\" gnuFit.gp",
                yIndex, par[0], par[1], par[2], par[3], startIndex);
    qDebug() << cmd.toStdString().c_str();
    QDir::setCurrent(DATA_PATH);
    qDebug() << QDir::currentPath();
    system(cmd.toStdString().c_str());

    this->p = 4;
    this->sol.clear();
    this->error.clear();
    this->pValue.clear();
    this->gradSSR.clear();

    QString gnuFitLogPath = DESKTOP_PATH;
    gnuFitLogPath += "/gnufit.log";
    QFile fitlog(gnuFitLogPath);
    fitlog.open( QIODevice::ReadOnly);
    QTextStream stream(&fitlog);
    QString line, findstr;
    int pos;
    //Get fit parameter
    while(stream.readLineInto(&line)){
        //get SSR;
        findstr = "residuals :";
        pos = line.indexOf(findstr) +  findstr.length();
        if( pos > findstr.length()) {
            SSR = line.mid(pos,10).toDouble();
        }

        //get NDF;
        findstr = "(FIT_NDF)";
        pos = line.indexOf(findstr) + findstr.length();
        if( pos > findstr.length()) {
            pos = line.indexOf(":");
            DF = line.mid(pos+1, 10).toDouble();
        }

        //get delta;
        findstr = "iteration";
        pos = line.indexOf(findstr) + findstr.length();
        if( pos > findstr.length()) {
            pos = line.indexOf(":");
            delta = line.mid(pos+1, 20).toDouble();
        }

        //get a and error a;
        double a, ea;
        findstr = "=======================";
        pos = line.indexOf(findstr);
        if( pos > -1) {

            for( int j = 0; j < this->p; j++){
                stream.readLineInto(&line);

                pos = line.indexOf("=");
                a = line.mid(pos+1, 12).toDouble();

                pos = line.indexOf("+/-");
                ea = line.mid(pos+3, 8).toDouble();
                this->sol.push_back(a);
                this->error.push_back(ea);
                this->dpar.push_back(0);
                this->pValue.push_back(cum_tDis30(- std::abs(a/ea)));
            }

        }

    }

    if( this->sol.isEmpty() ) return 1;
    int xStart = this->startIndex;
    //int xEnd = this->n - 1;
    int xEnd = this->endIndex;
    int fitSize = xEnd - xStart + 1;
    Matrix Y(fitSize,1);
    for(int i = 1; i <= fitSize ; i++) {
        Y(i,1) = zdata[i + xStart - 1];
    }

    Matrix fn(fitSize,1);
    if(fitFuncID == 2) setEngineParameter(this->sol);
    for(int i = 1; i <= fitSize ; i++) {
        double x = xdata[i + xStart - 1];
        fn(i,1) = FitFunc(x, this->sol, fitFuncID);
    }
    Matrix dYn = fn-Y;

    Matrix F(fitSize,p); // F = grad(f)
    for(int i = 1; i <= fitSize ; i++) {
        double x = xdata[i - 1 + xStart];
        QVector<double> gradf = GradFitFunc(x, this->sol, fitFuncID);
        for(int j = 1; j <= p; j++){
            F(i,j) = gradf[j-1];
        }
    }
    //new gradient of SSR
    Matrix FtdY = F.Transpose()*dYn;
    this->gradSSR = FtdY.Matrix2QVec();

    return 0;
}

int Analysis::NonLinearFit(QVector<double> par0, bool gnufit)
{
    if( !gnufit){
        LMA(par0, this->lambda);
    }else{
        GnuFit(par0);
    }
    //Print();

    bool redFlag = 0; // reset redFlag
    for(int i = 0 ; i < gradSSR.size(); i++){
        redFlag |= std::abs(gradSSR[i]) > 0.01;
    }
    if( redFlag ) {
        fitFlag += 4;
        fitMsg += "fitFlag += 0100; Grad(SSR) > 0.02. \n";
    }
    redFlag = 0; // reset redFlag
    for(int i = 0 ; i < gradSSR.size(); i++){
        redFlag |= std::abs(pValue[i]) > 0.05;
    }
    if( redFlag ) {
        fitFlag += 8;
        fitMsg += "fitFlag += 1000; p-Value(s) > 5%. \n";
    }

    return 0;
}

void Analysis::CalFitData(QVector<double> par){
    fydata.clear();
    if(fitFuncID == 2) setEngineParameter(par);
    for ( int i = 0; i < this->n; i++){
        double x = xdata[i];
        fydata.push_back( FitFunc(x, par, fitFuncID) );
    }
}

void Analysis::Print()
{
    qDebug() << "======= Ana ==========";
    qDebug("         yIndex : %d, yValue : %f", yIndex, yValue);
    qDebug("      Data size : %d", n);
    qDebug("       par size : %d", p);
    qDebug("Start Fit Index : %d", startIndex);
    qDebug("  End Fit Index : %d", endIndex);
    qDebug("             DF : %d", DF);
    qDebug("    Sample mean : %f", mean);
    qDebug("Sample variance : %f", var);
    qDebug("            SSR : %f", SSR);
    qDebug("        fitFlag : %d", fitFlag);
    qDebug("         lambda : %f, delta : %f", lambda, delta);

    PrintVector(sol, "sol:");
    PrintVector(dpar, "dpar:");
    PrintVector(error, "error:");
    PrintVector(pValue, "p-Value:");
    PrintVector(gradSSR, "gradSSR:");

    qDebug()<< "======== End of Ana =========";
}

void Analysis::PrintVector(QVector<double> vec, QString str)
{
    QString tmp;
    tmp.sprintf("%*s(%d) : [ ", 15, str.toStdString().c_str(), vec.size());
    Msg = tmp;
    for(int i = 0; i < vec.size() - 1; i++){
        tmp.sprintf(" %7.3f,", vec[i]);
        Msg += tmp;
    }
    tmp.sprintf(" %7.3f]", vec[vec.size()-1]);
    Msg += tmp;

    SendMsg(Msg);

}

void Analysis::PrintMatrix(Matrix mat, QString str)
{

    int r = mat.GetRows();
    int c = mat.GetCols();

    QString tmp, msg;
    msg.sprintf("%s(%d,%d) = ", str.toStdString().c_str(), r, c);
    SendMsg(msg);

    msg = "[" ;
    for (int i = 1 ; i <= r ; i++){
        if ( i > 1) msg += " " ;
        for (int j = 1 ; j <= c-1; j++){
            tmp.sprintf("%7.3f, ",mat(i,j));
            msg += tmp;
        }
        if( i <= r -1) {
            tmp.sprintf("%7.3f;", mat(i,c));
        }else{
            tmp.sprintf("%7.3f]", mat(i,c));
        }
        msg += tmp;
        SendMsg(msg);
        msg.clear();
    }

}

void Analysis::PrintCoVarMatrix(){

    int r = CoVar.GetRows();
    int c = CoVar.GetCols();
    Matrix mat(r, c);

    for(int i = 1; i <= r; i++){
        for( int j = 1; j <= c; j++){
            double xy = CoVar(i,j);
            double x  = CoVar(i,i);
            double y  = CoVar(j,j);
            mat(i,j) =  xy / sqrt(x) / sqrt(y);
        }
    }

    PrintMatrix(mat, "Correlation Matrix");
}

int Analysis::FindXIndex(double goal){
    int xIndex = 0;
    for(int i = 0; i < xdata.size() ; i++){
        if( xdata[i] >= goal){
            xIndex = i;
            break;
        }
    }
    return xIndex;
}

double Analysis::FindXFromYAfterTZero(double y)
{
    int xIndex = 0;

    int zIndex = 0;
    double maxValue = 0;
    if( qAbs(zMax) > qAbs(zMin)) {
        zIndex = zMaxIndex;
        maxValue = zMax;
    }else{
        zIndex = zMinIndex;
        maxValue = qAbs(zMin);
    }

    //qDebug("zMinIndex: %d, zMaxIndex: %d, zIndex: %d", zMinIndex, zMaxIndex, zIndex);
    //qDebug("zMin: %f, zMax: %f, var: %f", zMin, zMax, var);

    if( maxValue < var ) return 20;

    for( int i = zIndex ; i < xdata.size()-1; i++){
        if( (zdata[i]-y) * (zdata[i+1]-y) < 0 ){
            //qDebug() << i << "," << xdata[i] << ", " << y << "," << zdata[i] << ", " << zdata[i+1];
            xIndex = i;
            break;
        }
    }

    if( xIndex == xdata.size()-2) return 20; // defalut value for not find

    return xdata[xIndex];
}
