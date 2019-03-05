#include "calculatormanager.h"
#include <QTime>

CalculatorManager::CalculatorManager(Mandelbrot::Reader<double>& reader, int thread_total) :
    r(reader), thread_total(thread_total) {
}

void CalculatorManager::run() {
    emit progress(0);
    QTime t;
    t.start();
    QThreadPool pool;
    for(int i = 0; i < thread_total; i++) {
        Mandelbrot::Calculator<double>* c = new Mandelbrot::Calculator<double>(r);
        pool.start(c);
    }
    int p = 0;
    while(!pool.waitForDone(1)) {
        int new_p = r.getProgress();
        if(new_p != p) {
            p = new_p;
            emit progress(p);
        }
    }
    emit progress(100);
    emit finished(t.elapsed());
}
