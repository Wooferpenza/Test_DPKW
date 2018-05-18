#include "sensor.h"

Sensor::Sensor(QObject *parent) : QObject(parent)
{

}
int Sensor:: DiffPt(QVector<double> *Result,const QVector<double> *Dat, double T, int N)
{

    int dataSize=Dat->size();
    if (T==0||N<2||dataSize<2){return -1;}
    int A=((N/2)*2)+1;
    if (A>dataSize) {A=dataSize/2*2+1;}
    int B=3;
    Result->clear();
    Result->reserve(dataSize);
    Result->push_back((Dat->at(1)-Dat->at(0))/T);
    int oldProgress=0;
    for (int i = B/2; i < dataSize-B/2; i++)
    {
        double D=0;
        for (int j = 1; j <= B; j++)
        {
            D+=(2*j-1-B)*Dat->at(i-1+j-B/2);
        }
        D=(D*6)/((B-1)*B*(B+1));
        Result->push_back(D/T);
        if ((i<dataSize-B/2-1)&&B<A) {B+=2;}
        if ((i>=dataSize-B/2-1)&&B>3) {B-=2;}
        int progress=i*100/dataSize;
        if ((progress-oldProgress)>=5) {emit OperationProgress(progress);oldProgress=progress;}

    }
    if (dataSize>=2) Result->push_back((Dat->at(dataSize-1)-Dat->at(dataSize-2))/T);
    emit OperationProgress(100);
    if (Result->size()!=Dat->size()) {return -1;}
    return 0;
}

int Sensor::Resampling(QVector<double> *Result, QVector<double> *Dat, QVector<double> *Time, double T)
{
    Result->clear();
    int N=(Time->last()-Time->first())/T;
    int n=Time->size();
    double offset=Time->first();
    int old_a=0;
    int j=0;
    int oldProgress=0;
    for (int i=0;i<N;i++)
    {
        int a=0;
        for (j;j<n-1;j++)
        {
            if ((i*T+offset>=Time->at(j))&&(i*T+offset<Time->at(j+1))){a=j; break;}

        }
        if (old_a<a){j=old_a;}
        old_a=a;
        double k,b;
        k=(Dat->at(a+1)-Dat->at(a))/(Time->at(a+1)-Time->at(a));
        b=Dat->at(a)-k*Time->at(a);
        Result->push_back((i*T+offset)*k+b);
        int progress=i*100/(N-1);
        if ((progress-oldProgress)>=5) {emit OperationProgress(progress);oldProgress=progress;}
    }
    return 0;
}

void Sensor::Set_Time(double *Tm)
{
    Time=Tm;
}

void Sensor::Set_Data(QVector<double> *Dt)
{
    Data=Dt;
}

DPKW::DPKW(QObject *parent): Sensor(parent)
{
}

void DPKW::Calculation()
{

    {
        Zero_Crossing.clear();
        Zero_Crossing_Y.clear();
        Reference_Number.clear();
        RPM.clear();
        emit Status("Расчет производной");
        connect(this,SIGNAL(OperationProgress(int)),this,SLOT(ProgessCalc(int)));
        if (DiffPt(&Differential,Data,(*Time),7))
        {
            emit Status("Ошибка расчета производной");return;
        }
        emit Status("Поиск вершины зубьев");
        int Data_Size=Data->size();
        double T0;
        bool flag1=true;
        int oldOpProgress=0;
        for (int i = 0; i < Data_Size-1; i++)
        {
            if (Data->at(i)>=0&&Data->at(i+1)<0&&Differential.at(i)<0&&flag1)
            {
                T0=(fabs(Data->at(i))*(*Time*(i+1))+fabs(Data->at(i+1))*(*Time*i))/(fabs(Data->at(i))+fabs(Data->at(i+1)));
                Zero_Crossing.push_back(T0);
                Zero_Crossing_Y.push_back(0);
                flag1=false;
            }
            if (Data->at(i)<=-1&&!flag1)
            {
                flag1=true;
            }
            int opProgress=i*100/(Data_Size-2);
            if ((opProgress-oldOpProgress)>=10){emit Progress(opProgress); oldOpProgress=opProgress;}

        }

    }
   //==========================================================================
    {
        emit Status("Расчет номера зуба");
        int j=0;
        int Zero_Crossing_size=Zero_Crossing.size();
        for (int i = 2; i < Zero_Crossing_size; i++)
        {
            if ( (Zero_Crossing[i]-Zero_Crossing[i-1])/(Zero_Crossing[i-1]-Zero_Crossing[i-2])>2 )
            {
                j=i;
                break;
            }
        }
        for (int i = 0; i<j; i++)
        {
            Reference_Number.push_back(58+i+1-j);
        }
        int jj=1;
        int oldOpProgress=0;
        for (int i = j; i < Zero_Crossing_size; i++)
        {

            Reference_Number.push_back(jj);
            if (++jj>58)	jj=1;
            int opProgress=i*100/(Zero_Crossing_size-1);
            if ((opProgress-oldOpProgress)>=50){emit Progress(opProgress); oldOpProgress=opProgress;}

        }
    }
   //========================================================
    {
    emit Status("Востановление недостающих зубов");
        for (int i = 0; i < Reference_Number.size(); i++)
        {

            if (Reference_Number[i]==57)
            {

                double  T0=(Zero_Crossing[i+3]-Zero_Crossing[i])/5;
                Zero_Crossing[i+1]=Zero_Crossing[i]+T0;
                Zero_Crossing.insert(i+2,Zero_Crossing[i]+2*T0);
                Zero_Crossing.insert(i+3,Zero_Crossing[i]+3*T0);
                Zero_Crossing[i+4]=Zero_Crossing[i]+4*T0;

                Reference_Number.insert(i+2,59);
                Reference_Number.insert(i+3,60);


            }
        }
    }
   //==============================================================================
    {
        emit Status("Расчет частоты вращения");
        int N=9;
        int A=((N/2)*2)+1;
        int B=3;
        int zerroCrossingSize=Zero_Crossing.size();
        RPM.clear();
        RPM.reserve(zerroCrossingSize);
        if (zerroCrossingSize<2){ emit Calculation_End(); return;}
        RPM.push_back(1000/(Zero_Crossing[1]-Zero_Crossing[0]));
        int oldOpProgress=0;
        for (int i = B/2; i <zerroCrossingSize-B/2; i++)
        {
            double T=0;
            for (int j = 1; j <= B; j++)
            {
                T+=(2*j-1-B)*Zero_Crossing[i-1+j-B/2];
            }
            T=(T*6)/((B-1)*B*(B+1));
            T=1000/T;
            RPM.push_back(T);
            if ((i<zerroCrossingSize-B/2-1)&&B<A) {B+=2;}
            if ((i>=zerroCrossingSize-B/2-1)&&B>3) {B-=2;}
            int opProgress=i*100/zerroCrossingSize;
            if ((opProgress-oldOpProgress)>=5) {emit Progress(opProgress);oldOpProgress=opProgress;}
        }
        if (zerroCrossingSize>=2)  RPM.push_back(1000/(Zero_Crossing[zerroCrossingSize-1]-Zero_Crossing[zerroCrossingSize-2]));
        emit Progress(100);
    }
     //========================================================
    emit Status("Передискретизация");
    Resampling(&RPM_Resampling,&RPM,&Zero_Crossing,0.1);
    //===========================================================
   DiffPt(&RPM_Differential,&RPM_Resampling,0.1,15);
    emit Status("Расчет завершен");

    emit Calculation_End();
}

void DPKW::ProgessCalc(int operationProgress)
{
    emit Progress(operationProgress);
}

