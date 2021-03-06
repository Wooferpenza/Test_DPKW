#ifndef ADC_H
#define ADC_H
#ifdef Q_OS_LINUX

#endif
#include <QObject>
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
 typedef  QVector<double> ADCData;
//-------------------------------------------------------------------------------------------
class Channel : public QObject
{
     Q_OBJECT
private:
    int mInput;                             // Физический вход
    int mBand;                              // Диапазон
    int mDiv;                               // Делитель
    ADCData mSampl;                         // Массив данных
    QMutex mSamplMutex;                     // Мютекс для блокировки доступа к данным
public:
    explicit Channel(QObject *parent = 0);
    int getInput() const;
    int getBand() const;
    int getDiv() const;
    void addSampl(const double &value);     // Добавить значение в массив данных
    void clearSampl();                      // Очистить массив данных
    ADCData *getPSempl();                   // Получить указатель на массив данных
    QMutex *getPSamplMutex();               // Получить указатель на мютекс
public slots:
    void setInput(const int &value);
    void setBand(const int &value);
    void setDiv(const int &value);
    void semplesChanged(double samplerate, bool append);
signals:
    void inputChanged(int);
    void bandChanged(int);
    void divChanged(int);
    void samplesAvailable(double,ADCData*,QMutex*,bool);    // Данные обновлены
};
//--------------------------------------------------------------------------------------------------------
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
    bool writeAdcPar();                      // Запись параметров в устройство
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
    bool Init(ULONG Slot);                                  // Инициализация устройства

signals:
    void Connect(bool);                                     // Сигнал изменения статуса устройства(подключено/не подключено)
    void Status(QString,int);                               // Статус устройства
    void Progress(int);
    void Half_Buffer_Full(double samplerate, bool append);  // Половина буфера заполнена
    void Finished();                                        // Сбор данных завершен
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
