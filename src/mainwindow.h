#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

#include <QVector>
//#include <QTimer>
#include "qcustomplot.h"
#include "LCard.h"
#include "sensor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void Save_Setting();
    void Load_Setting();
    void on_action_3_triggered();
    void Graph_Update(double samplerate, ADCData *sampl, QMutex *mutex, bool append);

    void Init_ADC();

    void on_Start_clicked();
    void on_action_4_triggered();


    void on_action_triggered();
    void Add_Full_Data_To_Gtaph();
    void Set_Time_Capture(QString str);

    void on_pushButton_3_clicked();
    void DPKW_Show();
    void on_Stop_clicked();

    void on_Channel_dial_valueChanged(int value);

    void on_Switch_dial_valueChanged(int value);

    void on_Band_dial_actionTriggered(int action);

    void on_Band_dial_valueChanged(int value);

    void Simulate(double *X, QVector<double> * Y);



    void on_action_5_triggered();

    void on_horizontalScrollBar_valueChanged(int value);

    void xAxisChanged(QCPRange range);

    void on_verticalScrollBar_valueChanged(int value);

    void yAxisChanged(QCPRange range);

    void graphDataChanged();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_checkBox_2_clicked(bool checked);

    void on_checkBox_clicked(bool checked);

    void on_checkBox_3_clicked(bool checked);

    void on_checkBox_4_clicked(bool checked);

    void on_tabWidget_currentChanged(int index);

    void on_pushButton_6_clicked();

private:
    Ui::MainWindow *ui;
    QProgressBar* Progress_Bar;
    QLabel* Status_String;
    QCheckBox* Status_Check;
    LCard *E14_140;
    int a=0;
    QTimer Tmr;


    int X_Begin=0;

    //QVector<double> Time;
    double T_Sample=0;          // Период дескретизации
    QVector<double> Data;
    DPKW DPKW1;
};

#endif // MAINWINDOW_H
