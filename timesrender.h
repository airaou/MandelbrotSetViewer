#ifndef TIMESRENDER_H
#define TIMESRENDER_H

#include <QRgb>
#include <QVector>
#include "lua/lua.hpp"
#include <QString>
#include "mandelbrot.h"

int TimesRender_errhandler(lua_State* L);

class TimesRender : public Mandelbrot::Render
{
    friend int TimesRender_errhandler(lua_State* L);
private:
    lua_State* L;
    QVector<QRgb> times_colors;
    QString last_errstr;
    QMutex mutex_times_colors;

public:

    static const char* const default_render;

    QRgb default_color;

    TimesRender();
    ~TimesRender();

    QString read_render();
    QString read_render(const char *filename);
    QString read_string(QString const& luacode);

    QRgb getPixelColor(size_t times);

private:
    QString load_render();
};

#endif // TIMESRENDER_H
