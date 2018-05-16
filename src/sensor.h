#ifndef SENSOR_H
#define SENSOR_H

#include <QObject>
#include <QVector>

class Sensor : public QObject
{
    Q_OBJECT
private:
public:
protected:
    double *Time;
    QVector<double> *Data;
    int DiffPt(QVector<double> *Result,const QVector<double> *Dat,double T,int N);
    int Resampling(QVector<double> *Result,QVector<double> *Dat,QVector<double> *Time,double T);
public:
    explicit Sensor(QObject *parent = 0);
    void Set_Time(double *Tm);
    void Set_Data(QVector<double> *Dt);

signals:
    void OperationProgress(int);
public slots:
};
class DPKW : public Sensor
{
    Q_OBJECT
private:

public:
    QVector<double> Differential;           // Производная входного сигнала
    QVector<double> Zero_Crossing;          // Время перехода сигнала через 0
    QVector<double> Zero_Crossing_Y;
    QVector<double> Reference_Number;       // Номер зуба
    QVector<double> RPM;
    QVector<double> RPM_Resampling;
    QVector<double> RPM_Differential;
    QVector<double> Cilinder;
    int progress=0;
    explicit DPKW(QObject *parent = 0) ;
    void Calculation();
    void Zero_Crossing_Calc();
    void Rference_Number_Calc();
signals:
   void Status(QString);    // Текстовые сообщения
   void Calculation_End(); // Расчет завершен
  void  Progress(int);    //
    public slots:
    void ProgessCalc(int);
};

#endif // SENSOR_H
