#include "waveletanalysis.h"

//WaveletAnalysis::WaveletAnalysis(QObject *parent) : QObject(parent)
WaveletAnalysis::WaveletAnalysis(QVector<double> a)
{
    size = a.size();
    M = qFloor( qLn(size)/qLn(2.) )+1;

    V = new QVector<double> [M];
    W = new QVector<double> [M];

    for( int i = 0; i < size; i++){
        origin.push_back(a[i]);
        V[0].push_back(a[i]);
        W[0].push_back(0.);
    }

    msg.sprintf("Array size = %d; Max scale = %d", size, M);

    //Should calculate G0, G1, H1, H2 and R for semi-orthonormal wavelet;

}

WaveletAnalysis::~WaveletAnalysis(){
    if( V != NULL) delete [] V;
    if( W != NULL) delete [] W;
}

void WaveletAnalysis::Decompose(){
    int s;
    WAbsMax = 0;
    for( s = 0; s < M-1 ; s++){

        int sizeV = V[s].size();
        if( sizeV == 0) return;

        V[s+1].clear();
        W[s+1].clear();

        for(int k = 1; k < sizeV/2.+1; k++){
            double sum = 0;
            for(int l = 1; l <= sizeV; l++){
                sum += H0(2*k-l)*V[s][l-1];
            }
            V[s+1].push_back(sum);

            sum = 0;
            for(int l = 1; l <= sizeV; l++){
                sum += H1(2*k-l)*V[s][l-1];
            }
            W[s+1].push_back(sum);
            if( WAbsMax < sum) WAbsMax = sum;
        }

    }

    msg.sprintf("Decomposed scale = %d", s );

    //for(int s = 1; s < M; s++){
    //    PrintV(s);
    //    PrintW(s);
    //}

}

void WaveletAnalysis::Recontruct(){

    for( int s = M-1; s > 0; s--){
        //int sizeV = (V[s].size()-1) * 2;

        V[s-1].clear();

        for(int l = 1; l <= V[s].size() * 2; l++){
            double sum = 0;
            for(int k = 1; k <= V[s].size(); k++){
                sum += G0(l+1-2*k)*V[s][k-1];
                sum -= G1(l+1-2*k)*W[s][k-1];
            }
            V[s-1].push_back(sum);
        }

    }

    msg.sprintf("Reconstructed.");
}

void WaveletAnalysis::HardThresholding(double threshold)
{
    for( int s = 1;  s < M ; s++){
        for( int k = 0; k <= W[s].size(); k++){
            if( qAbs(W[s][k]) < threshold ) {
                //qDebug() << s << "," << k << "," << W[s][k];
                W[s][k] = 0.;
            }
        }
    }
    msg.sprintf("Applied Hard Thresholding, level = %2.1f", threshold);
}

void WaveletAnalysis::PrintV(int s)
{
    qDebug() << "V("<<s << ")" << V[s];
}

void WaveletAnalysis::PrintW(int s)
{
    qDebug() << "W("<<s << ")" << W[s];
}

