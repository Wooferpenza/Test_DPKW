#ifndef DATASTORRAGE_H
#define DATASTORRAGE_H

#include <QObject>
namespace Datastorrage
{
class Channel : public QObject
{
     Q_OBJECT
private:
public:
explicit Channel(QObject *parent = 0);
};
}
class DataStorrage : public QObject
{
    Q_OBJECT

public:
    explicit DataStorrage(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DATASTORRAGE_H
