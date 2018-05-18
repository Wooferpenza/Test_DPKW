//#include <QObject>
//#include <windows.h>
//#include <QString>
//#include <QVector>
//#include <QMutex>

//#include <QtMath>
//#include "..\include\ioctl.h"
//#include "..\include\ifc_ldev.h"
//#include "..\include\create.h"
#define INITGUID
#include "LCard.h"
//#include <QMessageBox>
#include <QFile>
#include <QTime>
void *handle;
LCard::LCard(QObject *parent): QObject(parent)
{

}
//=================================================================================
LCard::~LCard()
{
    if(mStatusAdc)
    {
        mpI->CloseLDevice();
        mpI->Release();
    }
    clearChannel();
    setStatus(0);
}

Channel *LCard::addChannel()
{
    Channel *logic_Channel=new Channel();
    mChannel.push_back(logic_Channel);
    return logic_Channel;
}

bool LCard::removeChannel(Channel *logic_Channel)
{
    mChannel.removeOne(logic_Channel);
    delete logic_Channel;
    return true;
}

bool LCard::removeChannel(const int &index)
{
    if (index >= 0 && index < mChannel.size())
        return removeChannel(mChannel.at(index));
    else
    {
        qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
        return false;
    }
}

int LCard::clearChannel()
{
    int c = mChannel.size();
    for (int i=c-1; i >= 0; --i)
        removeChannel(mChannel.at(i));
    return c;
}

Channel *LCard::channel(const int &index) const
{
    if (index >= 0 && index < mChannel.size())
    {
        return mChannel.at(index);
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "index out of bounds:" << index;
        return 0;
    }
}

Channel *LCard::channel() const
{
    if (! mChannel.isEmpty())
    {
        return mChannel.last();
    } else
        return 0;
}

int LCard::channelCount() const
{
    return mChannel.size();
}
//=================================================================================
bool LCard::Init(ULONG Slot)
{

    setStatus(0);
   //
   // void *handle;
   #ifdef Q_OS_LINUX
    handle = dlopen("../BIN/liblcomp.so",RTLD_LAZY);

     if(!handle)
     {
        //cout << "error open dll!! " << dlerror() << endl;
     }
     CreateInstance =(CREATEFUNCPTR) dlsym(handle,"CreateInstance");
//       if((error = dlerror())!=NULL)
//       {
//          cout << error << endl;
//       }
#endif
    #ifdef Q_OS_WIN
    CallCreateInstance("lcomp.dll");
#endif
    LUnknown* pLUnknown = CreateInstance(Slot);
    if(pLUnknown == NULL) { return 0; }
    HRESULT hr = pLUnknown->QueryInterface(IID_ILDEV,(void**)&mpI);  //получение указателя на интерфейс устройства
    if(hr!=S_OK) {return 0; }
    pLUnknown->Release();
    mpI->OpenLDevice();          // начало работы с платой
    mpI->GetSlotParam(&msl);      // получение параметров виртуального слота
    mpI->ReadPlataDescr(&mpd);    // чтение пользовательского флеш
    adcParDefault();
    mBufferSize=10000000;
    setStatus(1);
    return 1;
}
//================================================================================
void LCard::setStatus(int St)
{
    static int Status_ADC_Old=10;
    mStatusAdc=St;
    if (mStatusAdc==Status_ADC_Old) {return;}
    Status_ADC_Old=mStatusAdc;
    emit Connect(mStatusAdc!=0);
    switch (mStatusAdc)
    {
    case 0: {emit Status("Нет связи с АЦП",mStatusAdc); break;}
    case 1: {emit Status("АЦП подключен",mStatusAdc); break;}
    case 2: {emit Status("Идет сбор данных",mStatusAdc); break;}
    case 3: {emit Status("Сбор данных завершен",mStatusAdc); break;}
    }
}
//================================================================================
void LCard::adcParDefault()
{
    mAdcPar.t1.s_Type = L_ADC_PARAM;
    mAdcPar.t1.AutoInit = 1;
    mAdcPar.t1.dRate = 100.0;
    mAdcPar.t1.dKadr = 0;
    mAdcPar.t1.dScale = 0;
    mAdcPar.t1.SynchroType = 0;//0
    mAdcPar.t1.SynchroSensitivity = 0;
    mAdcPar.t1.SynchroMode = 0;
    mAdcPar.t1.AdChannel = 0;
    mAdcPar.t1.AdPorog = 0;
    mAdcPar.t1.NCh = 1;
    mAdcPar.t1.Chn[0] = 0x0;
    //adcPar.t1.Chn[1] = 0x1;
    //adcPar.t1.Chn[2] = 0x2;
    //adcPar.t1.Chn[3] = 0x3;
    mAdcPar.t1.FIFO = 4096;
    mAdcPar.t1.IrqStep = 4096;
    mAdcPar.t1.Pages = 32;
    mAdcPar.t1.IrqEna = 1;
    mAdcPar.t1.AdcEna = 1;
}
//================================================================================
bool LCard::writeAdcPar()
{
    updateAdcPar();
    if (mpI->RequestBufferStream(&mBufferSize,L_STREAM_ADC))
    {
        return 0;
    }
    if(mpI->RequestBufferStream(&mBufferSize,L_STREAM_ADC))
    {
        return 0;
    }
    if(mpI->FillDAQparameters(&mAdcPar.t1))
    {
        return 0;
    }
    if(mpI->SetParametersStream(&mAdcPar.t1, &mBufferSize, (void **)&mpData, (void **)&mpSync,L_STREAM_ADC))
    {
        return 0;
    }
    if (mpI->InitStartLDevice()) // Инициализируем внутренние переменные драйвера
    {
        return 0;
    }
    return 1;

}
//==================================================================================
void LCard::Start()
{
    if (mStatusAdc!=1&&mStatusAdc!=3) {return;}
    if (!writeAdcPar()) {return;}
    emit Progress(0);
   QtConcurrent::run(this,&LCard::capture);
    // capture();
}
//==================================================================================
void LCard::Stop()
{
    if (mStatusAdc!=2) {return;}
    mStpCapture=true;
}
//==================================================================================
void LCard::setSamplingRate(int value)
{
    mAdcPar.t1.dRate = (double)value;
  if((mStatusAdc==1)||(mStatusAdc==3))
  {
    writeAdcPar();
  }
    emit samplingRateChanged(mAdcPar.t1.dRate);
}
//===================================================================================
void LCard::setTimeCapture(int value)
{
    mTimeCapture=value;
    emit timeCaptureChanged(mTimeCapture);
}
//===================================================================================
void LCard::capture()
{
    for (int i=0;i<mChannel.size();i++)
    {
        connect(this,SIGNAL(Half_Buffer_Full(double,bool)),mChannel.at(i),SLOT(semplesChanged(double,bool)));
    }
    bool append=false;
    setStatus(2);
    mStpCapture=false;
    int N_Data=mTimeCapture*mAdcPar.t1.dRate*1000;
    mTSample=1/mAdcPar.t1.dRate*mChannel.size();
    mDataSize=0;

    QTime t;
    t.start();

    mpI->StartLDevice(); // Запускаем сбор в драйвере
    int fl2,fl1 = fl2 = (*mpSync<=halfBuffer())? 0:1; // Настроили флаги
    int t1=t.elapsed();
    qDebug("ADC_Capture_Time1: %d ms", t1);

    do                            // Цикл по необходимомму количеству половинок
    {
        int t2=t.elapsed();

        while(fl2==fl1)
        {
#ifdef Q_OS_WIN
            Sleep(1);
#endif
#ifdef Q_OS_Linux
            usleep(1000);
#endif
            fl2=(*mpSync<=halfBuffer())? 0:1; // Ждем заполнения половинки буфера
        }

        int t3=t.elapsed();
        qDebug("ADC_Capture_Time2: %d ms", t3-t2);

        for (int i=0;i<mChannel.size();i++)
        {
            mChannel.at(i)->clearSampl();
            mChannel.at(i)->getPSamplMutex()->lock();
        }

        int Ch=0;

        for (unsigned int i=0;i<halfBuffer();i++)
        {
            SHORT b=*(((SHORT *)mpData)+i+halfBuffer()*fl1);
            mChannel.at(Ch)->addSampl(calibrate(Ch,(double)b));
            if((++Ch)>=mChannel.size()) {Ch=0;}
            mDataSize++;
            if (mDataSize>=N_Data||mStpCapture)
            {break;}
        }
        for (int i=0;i<mChannel.size();i++)
        {
            mChannel.at(i)->getPSamplMutex()->unlock();
        }
        emit Progress((mDataSize*100)/N_Data);
        emit Half_Buffer_Full(1/mTSample,append);
        append =true;
        fl1=(*mpSync<=halfBuffer())? 0:1;                 // Обновляем флаг
        int t4=t.elapsed();

        qDebug("ADC_Capture_Time3: %d ms", t4-t3);
#ifdef Q_OS_WIN
            Sleep(100);
#endif
#ifdef Q_OS_Linux
            usleep(100000);
#endif

        //Sleep(200); // если собираем медленно то можно и спать больше
    }
    while(mDataSize<N_Data&&(!mStpCapture));
    mpI->StopLDevice(); // Останавливаем сбор в драйвере
    qDebug("Stop Capture");
    for (int i=0;i<mChannel.size();i++)
    {
        disconnect(this,SIGNAL(Half_Buffer_Full(double, bool)),mChannel.at(i),SLOT(semplesChanged(double, bool)));
    }
    setStatus(3);
    emit Finished();
}
//==================================================================================
void LCard::updateAdcPar()
{
   for (int i=0;i<mChannel.size();i++)
    {
        mAdcPar.t1.Chn[i] = mChannel.at(i)->getInput()-1;
        mAdcPar.t1.Chn[i] =(mAdcPar.t1.Chn[i])|(mChannel.at(i)->getBand()<<6);
    }
    mAdcPar.t1.NCh =mChannel.size();
}
//==============================================================================
ULONG LCard::halfBuffer()
{
    return mAdcPar.t1.IrqStep*mAdcPar.t1.Pages/2;
}
//===============================================================================
double LCard::calibrate(int Ch, double Data)
{
    int Band=mChannel.at(Ch)->getBand();
    int Div=mChannel.at(Ch)->getDiv();
    return (Data+mpd.t5.KoefADC[Band])/8000*qPow(10,Div)*mpd.t5.KoefADC[Band+4];
}
//===============================================================================
int Channel::getBand() const
{
    return mBand;
}

void Channel::setBand(const int &value)
{
    mBand = value;
}

int Channel::getDiv() const
{
    return mDiv;
}

void Channel::setDiv(const int &value)
{
    mDiv = value;
}

void Channel::semplesChanged(double samplerate, bool append)
{
    emit samplesAvailable(samplerate,&mSampl,&mSamplMutex, append);
    qDebug("11");
}

void Channel::addSampl(const double &value)
{
    mSamplMutex.lock();
    mSampl.push_back(value);
    mSamplMutex.unlock();
}

void Channel::clearSampl()
{
    mSampl.clear();
}

ADCData *Channel::getPSempl()
{
    return &mSampl;
}

QMutex *Channel::getPSamplMutex()
{
    return &mSamplMutex;
}

Channel::Channel(QObject *parent): QObject(parent)
{

}

int Channel::getInput() const
{
    return mInput;
}

void Channel::setInput(const int &value)
{
    mInput = value;
}
