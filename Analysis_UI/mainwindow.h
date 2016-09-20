#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QVector>
#include "matrix.h"
#include "analysis.h"
#include "qcustomplot.h"
#include "fileio.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void Plot(int graphID, QVector<double> x, QVector<double> y, double xMin, double xMax, double yMin, double yMax);

    void PlotFitFunc();// also initialize ana

private slots:
    void Write2Log(QString str);

    void on_pushButton_clicked();

    void on_spinBox_y_valueChanged(int arg1);
    void on_spinBox_x_valueChanged(int arg1);

    void on_lineEdit_a_returnPressed();
    void on_lineEdit_Ta_returnPressed();
    void on_lineEdit_b_returnPressed();
    void on_lineEdit_Tb_returnPressed();

    void on_pushButton_Fit_clicked();
    void on_pushButton_reset_clicked();

private:
    Ui::MainWindow *ui;

    QString Msg;

    QCustomPlot *plot;
    FileIO *file;
    Analysis *ana;

};

#endif // MAINWINDOW_H