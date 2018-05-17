#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qdebug.h>
#include <QMessageBox>
#include <QString>
#include <QFile>
#include <QTextStream>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)

{

    ui->setupUi(this);
    Status_String=new QLabel(" ");
    Progress_Bar=new QProgressBar;
    Status_Check=new QCheckBox;
    E14_140=new LCard(this);
    ui->statusBar->addWidget(Status_String);
    ui->statusBar->addWidget(Progress_Bar);
    ui->statusBar->addWidget(Status_Check);
    Progress_Bar->setMinimum(0);
    Progress_Bar->setMaximum(100);
    E14_140->addChannel();
    E14_140->addChannel();
    connect(E14_140,SIGNAL(Connect(bool)),Status_Check,SLOT(setChecked(bool)));
    connect(E14_140,SIGNAL(Status(QString,int)),Status_String,SLOT(setText(QString)));
    connect(E14_140,SIGNAL(Progress(int)),Progress_Bar,SLOT(setValue(int)));

connect(E14_140->channel(0),SIGNAL(samplesAvailable(Channel::ADCData*,double,QMutex*)),this,SLOT(sl1(Channel::ADCData *ad, double sr, QMutex *m)));
     //  connect(E14_140->channel(1),SIGNAL(samplesAvailable(Channel::ADCData*,double,QMutex*)),this,SLOT(sl2(Channel::ADCData *, double, QMutex *)));

  //  connect(E14_140,SIGNAL(Half_Buffer_Full(int,double)),this,SLOT(Graph_Update(int,double)));
    connect(ui->Sampling_Rate,SIGNAL(valueChanged(int)),E14_140,SLOT(setSamplingRate(int)));
   // connect(ui->Sampling_Rate,SIGNAL(actionTriggered(int)),E14_140,SLOT(Set_Sampling_Rate(int)));
    connect(E14_140,SIGNAL(samplingRateChanged(double)),ui->lcdNumber,SLOT(display(double)));
    connect(ui->Time_Capture_dial,SIGNAL(valueChanged(int)),E14_140,SLOT(setTimeCapture(int)));
    connect(E14_140,SIGNAL(timeCaptureChanged(int)),ui->Time_Capture_lcdNumber,SLOT(display(int)));
    connect(ui->Channel_dial,SIGNAL(valueChanged(int)),ui->Channel_lcdNumber,SLOT(display(int)));
    connect(&DPKW1,SIGNAL(Status(QString)),Status_String,SLOT(setText(QString)));
    connect(&DPKW1,SIGNAL(Calculation_End()),this,SLOT(DPKW_Show()));




    connect(&Tmr,SIGNAL(timeout()),this,SLOT(Init_ADC()));
    Tmr.start(5000);



    // Инициализируем объект полотна для графика ...
    ui->Graph->addGraph(ui->Graph->xAxis, ui->Graph->yAxis);
    ui->Graph->addGraph(ui->Graph->xAxis2, ui->Graph->yAxis2);
    connect(ui->Graph->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));
    connect(ui->Graph->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(yAxisChanged(QCPRange)));
  //  connect(ui->Graph->graph(0), SIGNAL(dataChanged()), this, SLOT(graphDataChanged()));
    ui->Graph->xAxis->setLowerUpperRange(0,20000);
    ui->Graph->xAxis->setRange(0,20000);

    //ui->horizontalScrollBar->setRange(0,20000);
    ui->Graph->graph(0)->setAdaptiveSampling(1);
    ui->Graph->graph(1)->setAdaptiveSampling(1);
    ui->Graph->yAxis->setLowerUpperRange(-100,100);
    ui->Graph->yAxis->setRange(-100,100);
    ui->Graph->xAxis->setLabel("Время, мс");
    ui->Graph->yAxis->setLabel("Напряжение, В");
    ui->Graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->Graph->axisRect()->setRangeDrag(Qt::Horizontal);   // Включаем перетаскивание только по горизонтальной оси
    ui->Graph->axisRect()->setRangeZoom(Qt::Horizontal);   // Включаем удаление/приближение только по горизонтальной оси
    ui->Graph->axisRect()->setupFullAxesBox(true);
    // connect(ui->Graph, SIGNAL(beforeReplot()), this, SLOT(xAxisChang()));
    DPKW1.Set_Time(&T_Sample);
    DPKW1.Set_Data(&Data);

    for (int i=0; i<5;i++)
    {
        ui->Graph1->addGraph(ui->Graph1->xAxis, ui->Graph1->yAxis);
    }
    for (int i=1; i< ui->Graph1->graphCount();i++)
    {
        ui->Graph1->graph(i)->setLineStyle(QCPGraph::lsNone);//убираем линии
        ui->Graph1->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
    }
    ui->Graph1->xAxis->setRange(0,10000);
    ui->Graph1->yAxis->setRange(800,900);
    ui->Graph1->xAxis->setLabel("Время, мс");
    ui->Graph1->yAxis->setLabel("Частота вращения, об/мин");
    ui->Graph1->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->Graph1->axisRect()->setRangeDrag(Qt::Horizontal);   // Включаем перетаскивание только по горизонтальной оси
    ui->Graph1->axisRect()->setRangeZoom(Qt::Horizontal);   // Включаем удаление/приближение только по горизонтальной оси
    ui->Graph1->axisRect()->setupFullAxesBox(true);

    ui->Graph2->xAxis->setRange(0,10000);
    ui->Graph2->yAxis->setRange(-3,3);
    ui->Graph2->xAxis->setLabel("Время, мс");
    ui->Graph2->yAxis->setLabel("Ускорение, об/мин/мсек");
    ui->Graph2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->Graph2->axisRect()->setRangeDrag(Qt::Horizontal);   // Включаем перетаскивание только по горизонтальной оси
    ui->Graph2->axisRect()->setRangeZoom(Qt::Horizontal);   // Включаем удаление/приближение только по горизонтальной оси
    ui->Graph2->axisRect()->setupFullAxesBox(true);
    ui->Graph2->addGraph(ui->Graph2->xAxis, ui->Graph2->yAxis);



Load_Setting();

    emit ui->Sampling_Rate->valueChanged(ui->Sampling_Rate->value());
    emit ui->Time_Capture_dial->valueChanged(ui->Time_Capture_dial->value());
    emit ui->Channel_dial->valueChanged(ui->Channel_dial->value());
    emit ui->Switch_dial->valueChanged(ui->Switch_dial->value());
    emit ui->Band_dial->valueChanged(ui->Band_dial->value());
}

MainWindow::~MainWindow()
{

Save_Setting();
    delete ui;
    delete Progress_Bar;
    delete Status_String;
    delete Status_Check;
    delete E14_140;

}

void MainWindow::Save_Setting()
{
    QSettings settings("settings.ini",QSettings::IniFormat);
    settings.setValue("settings/Sampling_Rate",ui->Sampling_Rate->value());
    settings.setValue("settings/Time_Capture",ui->Time_Capture_dial->value());
    settings.setValue("settings/Channel_dial",ui->Channel_dial->value());
    settings.setValue("settings/Switch_dial",ui->Switch_dial->value());
    settings.setValue("settings/Band_dial",ui->Band_dial->value());
}

void MainWindow::Load_Setting()
{
    QSettings settings("settings.ini",QSettings::IniFormat);
    ui->Sampling_Rate->setValue(settings.value("settings/Sampling_Rate","1").toInt());
    ui->Time_Capture_dial->setValue(settings.value("settings/Time_Capture","10").toInt());
    ui->Channel_dial->setValue(settings.value("settings/Channel_dial","1").toInt());
    ui->Switch_dial->setValue(settings.value("settings/Switch_dial","1").toInt());
    ui->Band_dial->setValue(settings.value("settings/Band_dial","1").toInt());
}

void MainWindow::Init_ADC()
{
    if(E14_140->Init(0))
    {
        Tmr.stop();
    }
}

void MainWindow::Graph_Update(int X_Count, double t)
{
    T_Sample=t;
    QTime tt;
    tt.start();
  //  QVector<double> X(100);
  //  QVector<double> Y(100);
    double Average=0;
    for(int i=X_Begin;i<X_Count;i++)
    {
       // X.push_back(i*t);
        double D=E14_140->channel(0)->getData(i);
       // Y.push_back(D);
         ui->Graph->graph(0)->addData(i*t,D);
     //   Data.push_back(D);
        Average+=D;
    }
    Average=Average/(X_Count-X_Begin);
    ui->lcdNumber_2->display(Average);
    X_Begin=X_Count;
    qDebug("Graph_Time1: %d ms", tt.elapsed());
    ui->Graph->yAxis->setRange(ui->Band_lcdNumber->value()*-1-1,ui->Band_lcdNumber->value()+1);

  //  ui->Graph->graph(0)->addData(X,Y);
    //
    // создаем график и добавляем данные:

    ui->Graph->xAxis->setRange((X_Count-50000)/100,X_Count/100);
    qDebug("Graph_Time2: %d ms", tt.elapsed());
    ui->Graph->replot();
    qDebug("Graph_Time3: %d ms", tt.elapsed());
}

void MainWindow::Add_Full_Data_To_Gtaph()
{
    QTime tt;
    tt.start();
    Progress_Bar->setValue(0);
    ui->Graph->graph(0)->clearData();

    int Size_Data=Data.size();
    QVector<double>Time;
    Time.reserve(Size_Data);
    for (int i=0;i<Size_Data;i++)
    {
        Time.push_back(i*T_Sample);
    }
    qDebug("Load_File: %d ms", tt.elapsed());
    ui->Graph->graph(0)->setData(Time,Data);
    qDebug("Load_File: %d ms", tt.elapsed());
    qDebug("Load_File: %d ms", tt.elapsed());
   // xAxisChanged(ui->Graph->xAxis->range());
  //  yAxisChanged(ui->Graph->yAxis->range());
    ui->Graph->replot();
//on_horizontalScrollBar_setRange1();
}

void MainWindow::on_Start_clicked()
{
    Data.clear();
    ui->Graph->graph(0)->clearData();
    X_Begin=0;
    E14_140->Start();
}

void MainWindow::on_Stop_clicked()
{
   E14_140->Stop();

}

void MainWindow::on_action_4_triggered()
{
    QString File_Name=QFileDialog::getSaveFileName(0,"Сохранить в файл","","*.osc");
    if(File_Name=="") {return;}
    QFile Save_File(File_Name);// Создаем файл с данными
    Save_File.open(QIODevice::WriteOnly);              // Открываем файл на запись
    QDataStream stream(&Save_File);
    stream. setVersion(QDataStream::Qt_5_5);
    stream<<T_Sample<<Data;
    Save_File.close();
}

void MainWindow::on_action_triggered()
{

    QString File_Name=QFileDialog::getOpenFileName(0,"Открыть файл","","*.osc");
    QTime tt;
    tt.start();
    if(File_Name=="") {return;}
    QFile Open_File(File_Name);
    Open_File.open(QIODevice::ReadOnly);              // Открываем файл на запись
    QDataStream stream(&Open_File);
    stream. setVersion(QDataStream::Qt_5_5);
    Data.clear();
    stream>>T_Sample;
    stream>>Data;
    Open_File.close();
    qDebug("Load_File: %d ms", tt.elapsed());
    Add_Full_Data_To_Gtaph();

}

void MainWindow::Set_Time_Capture(QString str)
{
    E14_140->setTimeCapture(str.toInt());
}

void MainWindow::on_pushButton_3_clicked()
{
    connect(&DPKW1,SIGNAL(Progress(int)),Progress_Bar,SLOT(setValue(int)));
    QtConcurrent::run(&DPKW1,&DPKW::Calculation );
    //  DPKW1.Calculation();
}

void MainWindow::DPKW_Show()
{

//    ui->Graph->graph(1)->clearData();
//    int Size_Data=DPKW1.Differential.size();
//    for (int i=0;i<Size_Data;i++)
//    {
//        ui->Graph->graph(1)->addData(i*T_Sample,DPKW1.Differential.at(i));
//    }
//    ui->Graph->replot();

   for (int i=0; i< ui->Graph1->graphCount();i++)
   {
   ui->Graph1->graph(i)->clearData();
   }
    bool a=true;
    bool b=true;
    for(int i=0;i<DPKW1.RPM.size();i++)
    {
        if (DPKW1.Reference_Number.at(i)>21&&DPKW1.Reference_Number.at(i)<=51)
        {
            if (a)
            {
                ui->Graph1->graph(1)->addData(DPKW1.Zero_Crossing.at(i),DPKW1.RPM.at(i));
                b=false;
            }

            else
            {
                ui->Graph1->graph(4)->addData(DPKW1.Zero_Crossing.at(i),DPKW1.RPM.at(i));
                b=true;
            }
        }
        else
        {
            if (!b)
            {
                ui->Graph1->graph(3)->addData(DPKW1.Zero_Crossing.at(i),DPKW1.RPM.at(i));
                a=false;

            }
            else
            {
                ui->Graph1->graph(2)->addData(DPKW1.Zero_Crossing.at(i),DPKW1.RPM.at(i));
                a=true;
            }
        }
    }
    int RPMSamplingSize=DPKW1.RPM_Resampling.size();
    double MIN=10000;
    double MAX=0;
    for (int i=0;i<RPMSamplingSize;i++)
    {
      ui->Graph1->graph(0)->addData(i*0.1+DPKW1.Zero_Crossing.first(),DPKW1.RPM_Resampling.at(i));
      if (DPKW1.RPM_Resampling.at(i)>MAX) {MAX=DPKW1.RPM_Resampling.at(i);}
      if (DPKW1.RPM_Resampling.at(i)<MIN) {MIN=DPKW1.RPM_Resampling.at(i);}
    }
    ui->Graph1->yAxis->setRange(MIN,MAX);
    ui->Graph1->replot();
    for (int i=0;i<DPKW1.RPM_Differential.size();i++)
    {
      ui->Graph2->graph(0)->addData(i*0.1+DPKW1.Zero_Crossing.first(),DPKW1.RPM_Differential.at(i));

    }
ui->Graph2->replot();
}

void MainWindow::on_action_3_triggered()
{
    //if(!ADC1) return;
    //ADC_Setting w;
  //  w.show();
  //  w.exec();

}

void MainWindow::on_Channel_dial_valueChanged(int value)
{
    E14_140->channel(0)->setInput(value);
}

void MainWindow::on_Switch_dial_valueChanged(int value)
{
    E14_140->channel(0)->setDiv(value);
    if (value<4)
    {
        ui->Switch_lcdNumber->display(qPow(10,value));
        ui->Band_lcdNumber->display(ui->Switch_lcdNumber->value()/qPow(4,ui->Band_dial->value()));

    }
    else
    {
        ui->Switch_lcdNumber->display("OFF");
       // ui->Band_lcdNumber->display(ui->Switch_lcdNumber->value()/qPow(4,value));
    }
}

void MainWindow::on_Band_dial_actionTriggered(int action)
{
    E14_140->channel(0)->setBand(action);
    ui->Band_lcdNumber->display(ui->Switch_lcdNumber->value()/qPow(4,action));
}

void MainWindow::on_Band_dial_valueChanged(int value)
{
    E14_140->channel(0)->setBand(value);
    ui->Band_lcdNumber->display(ui->Switch_lcdNumber->value()/qPow(4,value));

}

void MainWindow::Simulate(double *X, QVector<double> *Y)
{
      *X=0.01;
      double A1=10;
      double A2=20;

int N=6;
int F=1000;
int T=5;
int n=T*1000/(*X);

double f=360*F*N*(*X)/(60*1000);
int count=1;
bool count_en=false;
for (int i=0;i<n;i++)
{
    if ((i*f/(count*360))>(360*(N-3))&&(i*f/(count*360))<(360*(N-3)+90))
    {Y->push_back(A2*sin(qDegreesToRadians((double) i*f)));count_en=true;}
else{Y->push_back(A1*sin(qDegreesToRadians((double) i*f)));}
 if ((i*f/(count*360))>=(360*(N-3)+90)&&count_en)
 {count++;count_en=false;}
}
}

void MainWindow::on_action_5_triggered()
{
     Data.clear();
     Simulate(&T_Sample,&Data);
     Add_Full_Data_To_Gtaph();
}

void MainWindow::on_horizontalScrollBar_valueChanged(int value)
{
    if (fabs(ui->Graph->xAxis->range().lower-(double)value/100)>0.1)
    {
        ui->Graph->xAxis->setRange((double)value/100, ui->Graph->xAxis->range().size(), Qt::AlignLeft);
        ui->Graph->replot();
    }
}

void MainWindow::xAxisChanged(QCPRange range)
{
    ui->horizontalScrollBar->setValue(range.lower*100); // adjust position of scroll bar slider
    ui->horizontalScrollBar->setPageStep(range.size()*100); // adjust size of scroll bar slider
    ui->horizontalScrollBar->setRange(ui->Graph->xAxis->lowerRange*100,(ui->Graph->xAxis->upperRange-range.size())*100);
}

void MainWindow::on_verticalScrollBar_valueChanged(int value)
{
    if (fabs(ui->Graph->yAxis->range().center()-value)>0.1)
    {
        ui->Graph->yAxis->setRange(value, ui->Graph->yAxis->range().size(), Qt::AlignCenter);
        ui->Graph->replot();
    }
}

void MainWindow::yAxisChanged(QCPRange range)
{
    ui->verticalScrollBar->setValue(range.center()); // adjust position of scroll bar slider
    ui->verticalScrollBar->setPageStep(range.size()); // adjust size of scroll bar slider
    ui->verticalScrollBar->setRange(ui->Graph->yAxis->lowerRange/2,ui->Graph->yAxis->upperRange/2);

    // ui->verticalScrollBar->setRange(ui->Graph->graph(0)->data()->vvalue(,ui->Graph->graph(0)->data()->lastKey()-range.size());
 //  ui->verticalScrollBar->setRange(-50,50);
  //

}

void MainWindow::graphDataChanged()
{
    if (ui->Graph->graph(0)->data()->empty()){return;}
    ui->Graph->xAxis->setLowerUpperRange(ui->Graph->graph(0)->data()->firstKey(),ui->Graph->graph(0)->data()->lastKey());
    //ui->Graph->yAxis->setLowerUpperRange(ui->Graph->graph(0)->data()->,ui->Graph->graph(0)->data()->last());
    double MIN=10000;
    double MAX=-10000;
    QCPDataMap::Iterator it=ui->Graph->graph(0)->data()->begin();
    for (;it!=ui->Graph->graph(0)->data()->end();++it)
    {
        if (it.value().value<MIN) {MIN=it.value().value;}
        if (it.value().value>MAX) {MAX=it.value().value;}
    }
    ui->Graph->yAxis->setLowerUpperRange(MIN-10,MAX+10);
    ui->Graph->replot();
}

void MainWindow::on_pushButton_clicked()
{
    ui->Graph->yAxis->setRange(ui->Graph->yAxis->range().lower/1.5,ui->Graph->yAxis->range().upper/1.5);
ui->Graph->replot();
}

void MainWindow::on_pushButton_2_clicked()
{
    ui->Graph->yAxis->setRange(ui->Graph->yAxis->range().lower*1.5,ui->Graph->yAxis->range().upper*1.5);
ui->Graph->replot();
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    ui->Graph1->graph(1)->setVisible(checked);
    ui->Graph1->replot();
}

void MainWindow::on_checkBox_2_clicked(bool checked)
{
    ui->Graph1->graph(2)->setVisible(checked);
    ui->Graph1->replot();
}

void MainWindow::on_checkBox_3_clicked(bool checked)
{
    ui->Graph1->graph(3)->setVisible(checked);
    ui->Graph1->replot();
}

void MainWindow::on_checkBox_4_clicked(bool checked)
{
    ui->Graph1->graph(4)->setVisible(checked);
    ui->Graph1->replot();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
     if (index==1)
     {
         QPen graphPen;
         graphPen.setColor(QColor(243,242,242));
         ui->Graph1->graph(0)->setPen(graphPen);
         graphPen.setColor( ui->checkBox->palette().color(ui->checkBox->backgroundRole()));
         ui->Graph1->graph(1)->setPen(graphPen);
         graphPen.setColor( ui->checkBox_2->palette().color(ui->checkBox_2->backgroundRole()));
         ui->Graph1->graph(2)->setPen(graphPen);
         graphPen.setColor( ui->checkBox_3->palette().color(ui->checkBox_3->backgroundRole()));
         ui->Graph1->graph(3)->setPen(graphPen);
         graphPen.setColor( ui->checkBox_4->palette().color(ui->checkBox_4->backgroundRole()));
         ui->Graph1->graph(4)->setPen(graphPen);
     }
}

void MainWindow::on_pushButton_6_clicked()
{
  qDebug("ADC test: %d ",  E14_140->channelCount());
//    E14_140->addChannel();
//    qDebug("ADC test: %d ",  E14_140->channelCount());
//    E14_140->channel(0)->setBand(40);
//    qDebug("ADC test: %d ",  E14_140->channel(0)->getBand());
//    E14_140->removeChannel(E14_140->channel(0));
  //    qDebug("ADC test: %d ",  E14_140->channelCount());
}

void MainWindow::sl1(Channel::ADCData *ad, double sr, QMutex *m)
{
    qDebug("1");
  //  qDebug("1: %f",sr);
}

void MainWindow::sl2(Channel::ADCData *ad, double sr, QMutex *m)
{
     qDebug("2");
   //   qDebug("2: %f",sr);
}


