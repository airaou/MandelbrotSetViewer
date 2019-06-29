#include "timesrender.h"
#include <QDebug>
#include <QMutex>
#include "lua/lua.hpp"

const char* const TimesRender::default_render =
        "function times_render(t)\n"
        "  local a = 255 - t % 256\n"
        "  return a, a, a\n"
        "end";


int TimesRender_errhandler(lua_State* L) {
    TimesRender* self;
    lua_getallocf(L, (void**)&self);
    self->last_errstr = luaL_checkstring(L, -1);
    return 0;
}

static void* timesrender_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    ((void)ud);
    ((void)osize);
    return realloc(ptr, nsize);
}

TimesRender::TimesRender() : times_colors()
{
    // 设置 ud 所属类指针
    L = lua_newstate(timesrender_alloc, this);
    luaL_openlibs(L);
    lua_pushcfunction(L, TimesRender_errhandler);

    default_color = qRgb(0, 255, 0);
}

TimesRender::~TimesRender() {
    lua_close(L);
}

static const char* const err_descs[] = {
    "LUA_OK",
    "LUA_YIELD",
    "运行错误",
    "语法错误",
    "内存错误",
    "内存回收错误",
    "错误机制错误",
};
static const int err_descs_num = sizeof(err_descs) / sizeof(const char* const);

// 假定加载函数已加载到栈
QString TimesRender::load_render() {

    // 删除原函数
    lua_pushnil(L);
    lua_setglobal(L, "times_shader");

    times_colors.clear();

    int r;

    // 运行代码 假定栈中第1为错误处理函数
    r = lua_pcall(L, 0, LUA_MULTRET, 1);

    if(r != LUA_OK) {
        if(r < err_descs_num) {
            return QString::fromUtf8(err_descs[r]) + ": " + QString::number(r);
        } else {
            return QString::fromUtf8("未知错误: ") + QString::number(r);
        }
    }

    // 获取着色函数
    r = lua_getglobal(L, "times_render");

    if(r != LUA_TFUNCTION) {
        if(r == LUA_TNIL) {
            return QString::fromUtf8("错误: 找不到着色函数\"times_render\"");
        }
        return QString::fromUtf8("错误: 着色器\"times_render\"应为函数");
    }

    lua_pushinteger(L, 0);

    // 假定错误处理函数在栈第1
    r = lua_pcall(L, 1, 3, 1);

    if(r != LUA_OK) {
        return QString::fromUtf8(err_descs[r]) + ": " + last_errstr;
    }

    int rgb[3];

    for(int i = 0; i < 3; i++) {
        int isnum;
        rgb[i] = lua_tointegerx(L, i - 3, &isnum);
        if(!isnum) {
            return QString::fromUtf8("返回值类型错误: 非整数");
        }
    }

    qDebug("ok: %d %d %d", rgb[0], rgb[1], rgb[2]);
    return "";
}

QString TimesRender::read_render(const char* filename) {
    int r;
    r = luaL_loadfile(L, filename);
    if(r != LUA_OK) {
        if(r < err_descs_num) {
            return QString::fromUtf8(err_descs[r]);
        } else {
            return QString::fromUtf8("未知错误: ") + QString::number(r);
        }
    }
    QString errstr = load_render();
    lua_settop(L, 1);
    qDebug("stack len: %d", lua_gettop(L));
    return errstr;
}

QString TimesRender::read_string(QString const& luacode) {
    int r;
    r = luaL_loadstring(L, luacode.toStdString().c_str());
    if(r != LUA_OK) {
        if(r < err_descs_num) {
            return QString::fromUtf8(err_descs[r]);
        } else {
            return QString::fromUtf8("未知错误: ") + QString::number(r);
        }
    }
    QString errstr = load_render();
    lua_settop(L, 1);
    return errstr;
}

QString TimesRender::read_render() {
    return read_string(default_render);
}

QRgb TimesRender::getPixelColor(size_t times) {
    QMutexLocker locker(&mutex_times_colors);
    if(times >= (size_t)times_colors.size()) {
        if(times < (size_t)times_colors.size()) {
            return times_colors[times];
        }
        qDebug("debug: add!, %d", times_colors.size());
        lua_settop(L, 1);
        int t = lua_getglobal(L, "times_render");
        if(t != LUA_TFUNCTION) {
            qDebug("err: cannot find times_render or the type of times_render is not function");
            return default_color;
        }

        // 栈状态: 错误处理函数, 着色函数, 着色函数
        for(size_t t = times_colors.size(); t <= times; t++) {
            lua_settop(L, 3);
            lua_copy(L, -2, -1);

            lua_pushinteger(L, t);
            int r = lua_pcall(L, 1, 3, 1);
            if(r != LUA_OK) {
                qDebug("err: pcall fatel");
                times_colors.append(default_color);
                continue;
            }

            int rgb[3];

            for(int i = 0; i < 3; i++) {
                int isnum;
                rgb[i] = lua_tointegerx(L, i - 3, &isnum);
                if(!isnum) {
                    qDebug("err: return invaild");
                    times_colors.append(default_color);
                    continue;
                }
            }

            times_colors.append(qRgb(rgb[0] & 0xff, rgb[1] & 0xff, rgb[2] & 0xff));
            // qDebug("%d: %d %d %d", t, rgb[0], rgb[1], rgb[2]);
        }
    }
    return times_colors[times];
}
