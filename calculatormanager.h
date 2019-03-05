#ifndef CALCULATORMANAGER_H
#define CALCULATORMANAGER_H

#include <QObject>
#include "mandelbrot.h"

class CalculatorManager : public QThread {
    Q_OBJECT
private:
    Mandelbrot::Reader<double>& r;
    const int thread_total;

public:
    CalculatorManager(Mandelbrot::Reader<double>& reader, int thread_total);
    virtual void run();

signals:
    void progress(int percentage);
    void finished(int ms_time);
};

#endif // CALCULATORMANAGER_H
