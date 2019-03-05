#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <iostream>
#include <QDebug>
#include <QImage>
#include <QThread>
#include <QVector>
#include <QRunnable>
#include <QThreadPool>
#include <QMutex>

namespace Mandelbrot {

    class Writer {
    public:
        virtual void set(size_t final_times) = 0;
    };

    template<typename T>
    class Reader {
    public:
        virtual int getProgress() = 0;
        virtual Writer* get(T& c_real, T& c_imag, size_t& max_times) = 0;
    };

    class RectangleImageWriter : public Writer {
    private:
        unsigned char* const row_data;
        const size_t max_times;

    public:
        RectangleImageWriter(unsigned char* row_data, size_t max_times) :
            row_data(row_data), max_times(max_times) {
        }
        virtual void set(size_t final_times) {
            row_data[0] = 255 - 255 * final_times / max_times;
            row_data[1] = row_data[0];
            row_data[2] = row_data[0];
        }
    };

    template<typename T>
    class RectangleImageReader : public Reader<T> {
    private:
        QImage* img;
        const T lux;
        const T luy;
        const T width;
        const T height;
        const int pwidth;
        const int pheight;
        int x;
        int y;
        const size_t max_times;
        QMutex mutex;
    public:
        RectangleImageReader(QImage* img, T lux, T luy, T width, T height, size_t max_times) :
            img(img), lux(lux), luy(luy), width(width), height(height),
            pwidth(img->width()), pheight(img->height()),
            x(0), y(0), max_times(max_times), mutex() {
        }
        virtual int getProgress() {
            return (y * pwidth + x) * 100 / (pwidth * pheight);
        }
        virtual Writer* get(T& c_real, T& c_imag, size_t& max_times) {
            mutex.lock();
            if(x >= pwidth || y >= pheight) {
                mutex.unlock();
                return NULL;
            }
            c_real = width * x / (T)(pwidth - 1) + lux;
            c_imag = height * y / (T)(pheight - 1) - luy;
            Writer* w = new RectangleImageWriter(img->scanLine(y) + x * 3, this->max_times);
            x++;
            if(x >= pwidth) {
                x = 0;
                y++;
            }
            max_times = this->max_times;
            mutex.unlock();
            return w;
        }
    };

    template<typename T>
    size_t calc(T c_real, T c_imag, size_t max_times) {
        T z_real = 0;
        T z_imag = 0;
        for(size_t times = 0; times < max_times; times++) {
            T nz_real = z_real * z_real - z_imag * z_imag + c_real;
            T nz_imag = 2 * z_real * z_imag + c_imag;
            z_real = nz_real;
            z_imag = nz_imag;
            if(z_real * z_real + z_imag * z_imag > 4) {
                return times;
            }
        }
        return max_times;
    }

    template<typename T>
    void calc(Reader<T>& r) {
        T x;
        T y;
        size_t times;
        Writer* w;
        while((w = r.get(x, y, times)) != NULL) {
            times = calc<T>(x, y, times);
            w->set(times);
            delete w;
        }
    }

    template<typename T>
    class Calculator : public QRunnable {
    private:
        Reader<double>& r;

    public:
        explicit Calculator(Reader<double>& reader) :
            r(reader) {
            if(!this->autoDelete()) {
                qDebug("未设置autoDelete默认值为true");
                this->setAutoDelete(true);
            }
        }

        virtual void run() {
            calc(r);
        }
    };
}

#endif // MANDELBROT_H
