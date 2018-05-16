#ifndef ADC_H
#define ADC_H
#ifdef Q_OS_LINUX
//#define LCOMP_LINUX 1
#endif
#include <QObject>
//#include <windows.h>
#include <QString>
#include <QVector>
#include <QMutex>
#include <QtConcurrent/QtConcurrentRun>
#include <QtMath>
#ifdef Q_OS_LINUX
#include <dlfcn.h>
#include "../include/stubs.h"
#include "../include/ioctl.h"
#include "../include/ifc_ldev.h"
#endif
#ifdef Q_OS_WIN
#include "..\include\stubs.h"
#include "..\include\ioctl.h"
#include "..\include\ifc_ldev.h"
#include "..\include\create.h"
#endif








class Channel : public QObject
{
     Q_OBJECT
    typedef  QVector<double> ADCData;
private:
    int mInput;
    int mBand;
    int mDiv;
    ADCData mData;
    ADCData mSampl;
    QMutex D_mutex;                         // Мютекс для блокировки доступа к данным
public:
    explicit Channel(QObject *parent = 0);
    int getInput() const;
    int getBand() const;
    int getDiv() const;
    void addData(const double &value);
    void addSampl(const double &value);
    ADCData *data();
    double getData(const int &index);
public slots:
    void setInput(const int &value);
    void setBand(const int &value);
    void setDiv(const int &value);
signals:
    void inputChanged(int);
    void bandChanged(int);
    void divChanged(int);
};

class LCard : public QObject
{
    Q_OBJECT
private:
    QVector<Channel *> mChannel;             // Массив логических каналов
    IDaqLDevice *mpI;                        // Указатель на интерфейс устройства
    SLOT_PAR msl;                            // Параметры виртуального слота
    PLATA_DESCR_U2 mpd;                      // Флеш устройства
    ADC_PAR mAdcPar;                         // Параметры сбора данных
    void *mpData;                            // Указатель на адрес большого буфера
    ULONG *mpSync;                           // Указатель на переменную синхронизации
    ULONG mBufferSize;                       // Размер буфера
    int mStatusAdc=0;                        // Статус устройства
    double mTSample=0;                       // Период дискретизации
    int mDataSize;                           // Объем собранных данных
    int mTimeCapture;                        // Время захвата данных сек.
    bool mStpCapture;                        // Флаг для прерывания сбора даннных

    void setStatus(int St);                  // Установка статуса устройства
    void adcParDefault();                    // Сброс на параметры по умолчанию
    bool writeAdcPar();                       // Запись параметров в устройство
    ULONG halfBuffer();                      // Получить половину буфера
    double calibrate(int Ch,double Data);    // Калибровка измеренных данных
    void capture();                          //Сбор данных
#ifdef Q_OS_LINUX
    typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);
    CREATEFUNCPTR CreateInstance;
#endif

public:
    explicit LCard(QObject *parent = 0);                // Конструктор
    ~LCard();                                           // Деструктор
    Channel *addChannel();                              // Добавить логический канал
    bool removeChannel(Channel * logic_Channel);        // Удалить логический канал
    bool removeChannel(const int &index);               // Удалить логический канал
    int clearChannel();                                 // Удалить все логические каналы
    Channel *channel(const int &index) const;
    Channel *channel() const;
    int channelCount() const;
    bool Init(ULONG Slot);                              // Инициализация устройства

signals:
    void Connect(bool);                              // Сигнал изменения статуса устройства(подключено/не подключено)
    void Status(QString,int);                        // Статус устройства
    void Progress(int);
    void Half_Buffer_Full(int mDataSize,double t);   // Половина буфера заполнена
    void Finished();                                 // Сбор данных завершен
    void samplingRateChanged(double);
    void timeCaptureChanged(int);
private slots:
    void updateAdcPar();
public slots:
    void Start();                               // Старт сбора данных устройством
    void Stop();                                // Стоп сбора данных устройством
    void setSamplingRate(int value);             // Задание частоты дискретизации
    void setTimeCapture(int value);           // Задание времени захвата данных
};
#endif // ADC_H
