#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mandelbrot.h"
#include <cmath>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QTime>
#include <QDesktopWidget>
#include <QClipboard>
#include <QRegExp>
#include <QStringListModel>
#include "timesrender.h"

/**
 * @brief 更换输入框错误标记(消去,添加)
 */
inline static void changeErrorMark(QLineEdit* lineEdit, bool to_set) {
    if(to_set) {
        lineEdit->setStyleSheet("QLineEdit{border:1px solid red }");
    } else {
        lineEdit->setStyleSheet("QLineEdit{border:1px solid gray border-radius:1px}");
    }
}

inline static bool isInteger(QString const& str) {
    bool ok;
    int i = str.toInt(&ok);
    return ok && i > 0;
}
inline static bool isNumber(QString const& str) {
    bool ok;
    str.toDouble(&ok);
    return ok;
}
inline static bool isFilename(QString const& str) {
    if(str.length() == 0) {
        return false;
    } else {
        if(str.contains(QRegExp("[\\\\/:*?\"<>|]"))) {
            return false;
        } else {
            return true;
        }
    }
}

inline static void checkInteger(QLineEdit* lineEdit) {
    changeErrorMark(lineEdit, !isInteger(lineEdit->text()));
}
inline static void checkNumber(QLineEdit* lineEdit) {
    changeErrorMark(lineEdit, !isNumber(lineEdit->text()));
}
inline static void checkFilename(QLineEdit* lineEdit) {
    changeErrorMark(lineEdit, !isFilename(lineEdit->text()));
}

template<typename T>
inline static T max(T a, T b) { return a > b ? a : b; }

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    scene(new QGraphicsScene()),
    pixmapItem(new QGraphicsPixmapItem()),
    viewCalcMgr(NULL),
    viewImg(NULL),
    viewImgReader(NULL),
    geneCalcMgr(NULL),
    geneImg(NULL),
    geneImgReader(NULL),
    model(new QStringListModel(strlist))
{
    ui->setupUi(this);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setScene(scene);
    scene->addItem(pixmapItem);

    ui->historyListView->setModel(model);

//    setMaximumSize(size());
//    setMinimumSize(size());

    ui->centerRealCheckBox->setChecked(true);
    ui->centerRealLineEdit->setText("0");
    ui->centerImagCheckBox->setChecked(true);
    ui->centerImagLineEdit->setText("0");
    ui->widthCheckBox->setChecked(true);
    ui->widthLineEdit->setText("4");
    ui->widthPixelLineEdit->setText("1024");
    ui->heightPixelLineEdit->setText("768");
    ui->threadTotalSpinBox->setValue(4);
    ui->timesSpinBox->setValue(255);
    //ui->viewButton->click();

    ui->historyLine->setVisible(false);
    ui->historyListView->setVisible(false);
    ui->historyLabel->setVisible(false);

    ui->editSenderPushButton->setText(QString::fromUtf8("编辑着色器"));
    ui->shaderLine->setVisible(false);
    ui->shaderTextEdit->setVisible(false);
    ui->shaderLabel->setVisible(false);
    ui->shaderErrorLabel->setVisible(false);
    ui->shaderTextEdit->setText(timesRender.default_render);
    timesRender.read_string(timesRender.default_render);


}

MainWindow::~MainWindow() {
    delete model;
    delete pixmapItem;
    delete scene;
    delete ui;
}

void MainWindow::setViewSize(int w, int h) {
    ui->graphicsView->setMaximumSize(w + 2, h + 2);
    ui->graphicsView->setMinimumSize(w + 2, h + 2);
}

/**
 * @brief 检查勾选框是否数据勾选饱和
 */
template<int N, int M>
inline static int count_checked(QCheckBox* const (&checkBoxs)[N][M]) {
    int cnt = 0;
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < M; j++) {
            if(checkBoxs[i][j] && checkBoxs[i][j]->isChecked()) cnt++;
        }
    }
    return cnt;
}
template<int N>
inline static int count_checked(QCheckBox* const (&checkBoxs)[N]) {
    int cnt = 0;
    for(int i = 0; i < N; i++) {
        if(checkBoxs[i] && checkBoxs[i]->isChecked()) cnt++;
    }
    return cnt;
}

template<int N, int M>
inline static void lock(QCheckBox* const (&checkBoxs)[N][M]) {
    for(int i = 0; i < N; i++) {
        int cnt = count_checked(checkBoxs[i]);
        if(cnt == 0) {
            lock(checkBoxs[i]);
        } else {
            unlock(checkBoxs[i]);
        }
    }
}
template<int N>
inline static void lock(QCheckBox* const (&checkBoxs)[N]) {
    for(int i = 0; i < N; i++) {
        if(checkBoxs[i] && !checkBoxs[i]->isChecked()) checkBoxs[i]->setCheckable(false);
    }
}

template<int N, int M>
inline static void unlock(QCheckBox* const (&checkBoxs)[N][M]) {
    for(int i = 0; i < N; i++) {
        unlock(checkBoxs[i]);
    }
}
template<int N>
inline static void unlock(QCheckBox* const (&checkBoxs)[N]) {
    for(int i = 0; i < N; i++) {
        if(checkBoxs[i]) checkBoxs[i]->setCheckable(true);
    }
}

void MainWindow::countChecked(int& real_cnt, int& imag_cnt, int& final_cnt, int& s) {
    QCheckBox* const checkBoxs_final[][3] = {
        {ui->pixelCheckBox,      ui->widthCheckBox,  ui->heightCheckBox, },//同行的互斥,已保证不会同时勾选
    };
    final_cnt = count_checked(checkBoxs_final);

    QCheckBox* const checkBoxs_real[][2] = {
        {ui->centerRealCheckBox,                     },
        {ui->LURealCheckBox,     ui->LDRealCheckBox, },
        {ui->RURealCheckBox,     ui->RDRealCheckBox, },
    };
    real_cnt = count_checked(checkBoxs_real);

    QCheckBox* const checkBoxs_imag[][2] = {
        {ui->centerImagCheckBox,                     },
        {ui->LUImagCheckBox,     ui->RUImagCheckBox, },
        {ui->LDImagCheckBox,     ui->RDImagCheckBox, },
    };
    imag_cnt = count_checked(checkBoxs_imag);

    s = max<int>(0, real_cnt - 1) + max<int>(0, imag_cnt - 1) + final_cnt;
}

void MainWindow::lockCheckBox() {
    QCheckBox* const checkBoxs_final[][3] = {
        {ui->pixelCheckBox,      ui->widthCheckBox,  ui->heightCheckBox, },
    };
    QCheckBox* const checkBoxs_real[][2] = {
        {ui->centerRealCheckBox,                     },
        {ui->LURealCheckBox,     ui->LDRealCheckBox, },
        {ui->RURealCheckBox,     ui->RDRealCheckBox, },
    };
    QCheckBox* const checkBoxs_imag[][2] = {
        {ui->centerImagCheckBox,                     },
        {ui->LUImagCheckBox,     ui->RUImagCheckBox, },
        {ui->LDImagCheckBox,     ui->RDImagCheckBox, },
    };
    /**
     * 实部至少一个, 虚部至少一个, 除去这两个后, 除去这两个后余下的实部虚部终部里能且只能有一个
     */

    int real_cnt, imag_cnt, final_cnt, sum;
    countChecked(real_cnt, imag_cnt, final_cnt, sum);

    /**
     * | r | i | f | s | r | i | f |
     * | 0 | 0 | 0 | 0 |   |   |   |
     * | 1 | 0 | 0 | 0 |   |   |   |
     * | 0 | 1 | 0 | 0 |   |   |   |
     * | 1 | 1 | 0 | 0 |   |   |   |
     * | 0 | 0 | 1 | 1 |   |   | l |
     * | 2 | 0 | 0 | 1 | l |   | l |
     * | 1 | 0 | 1 | 1 | l |   | l |
     * | 0 | 1 | 1 | 1 |   | l | l |
     * | 0 | 2 | 0 | 1 |   | l | l |
     * | 1 | 2 | 0 | 1 | l | l | l |
     * | 2 | 1 | 0 | 1 | l | l | l |
     * | 1 | 1 | 1 | 1 | l | l | l |
     **/
    if(sum > 0) {
        lock(checkBoxs_final);
        if(real_cnt > 0) {
            lock(checkBoxs_real);
        } else {
            unlock(checkBoxs_real);
        }
        if(imag_cnt > 0) {
            lock(checkBoxs_imag);
        } else {
            unlock(checkBoxs_imag);
        }
    } else {
        unlock(checkBoxs_real);
        unlock(checkBoxs_imag);
        unlock(checkBoxs_final);
    }
}

bool MainWindow::calcFinal(int* pw, int* ph) {
    bool pok;
    /**
     * 终部计算
     */

    int pwidth, pheight;
    pwidth = ui->widthPixelLineEdit->text().toInt(&pok);
    if(pok) pheight = ui->heightPixelLineEdit->text().toInt(&pok);
    if(pok) {
        if(pw) *pw = pwidth;
        if(ph) *ph = pheight;
    }
    if(pok && pwidth > 0 && pheight > 0) {
        double n;
        bool ok;
        if(ui->pixelCheckBox->isChecked()) {
            n = ui->pixelLineEdit->text().toDouble(&ok);
            if(ok) {
                ui->widthLineEdit->setText(QString::number(n * pwidth, 'g', 15));
                ui->heightLineEdit->setText(QString::number(n * pheight, 'g', 15));
            }
        } else if(ui->widthCheckBox->isChecked()) {
            n = ui->widthLineEdit->text().toDouble(&ok);
            if(ok) {
                ui->pixelLineEdit->setText(QString::number(n / pwidth, 'g', 15));
                ui->heightLineEdit->setText(QString::number(n * pheight / pwidth, 'g', 15));
            }
        } else if(ui->heightCheckBox->isChecked()) {
            n = ui->heightLineEdit->text().toDouble(&ok);
            if(ok) {
                ui->pixelLineEdit->setText(QString::number(n / pheight, 'g', 15));
                ui->widthLineEdit->setText(QString::number(n * pwidth / pheight, 'g', 15));
            }
        }
    }
    return pok;
}

void MainWindow::calc() {
    int pwidth = 0, pheight = 0;
    bool pok = calcFinal(&pwidth, &pheight);
    /**
     * 实部一个 虚部一个 实部虚部终部任一个即可计算 其中已知终部的计算需要填写像素长宽
     */

    double lux = 0, luy = 0, width = 0, height = 0;

    int real_cnt, imag_cnt, final_cnt, sum;
    countChecked(real_cnt, imag_cnt, final_cnt, sum);
    if(real_cnt && imag_cnt && sum) {
        if(final_cnt == 0) {
            if(real_cnt > 1) {
                double l, m, r;
                bool lok, mok, rok;
                getX(l, lok, m, mok, r, rok);
                if(lok && mok) {
                    lux = l;
                    width = (m - l) * 2;
                    setXR(2 * m - l);
                } else if(lok && rok) {
                    lux = l;
                    width = r - l;
                    setXM((l + r) / 2);
                } else if(mok && rok) {
                    lux = 2 * m - r;
                    width = (r - m) * 2;
                    setXL(lux);
                } else {
                    return;
                }
                setWidth(width);
                if(pok && pwidth > 0 && pheight > 0) {
                    height = width * pheight / pwidth;
                    setHeight(height);
                    setPixel(width / pwidth);
                    double d, n, u;
                    bool dok, nok, uok;
                    getY(d, dok, n, nok, u, uok);
                    if(dok) {
                        luy = d + height;
                        setYM(d + height / 2);
                        setYU(d + height);
                    } else if(nok) {
                        luy = n + height / 2;
                        setYD(n - height / 2);
                        setYU(n + height / 2);
                    } else if(uok) {
                        luy = u;
                        setYD(u - height);
                        setYM(u - height / 2);
                    }
                }
            } else if(imag_cnt > 1) {
                double d, m, u;
                bool dok, mok, uok;
                getY(d, dok, m, mok, u, uok);
                if(dok && mok) {
                    luy = 2 * m - d;
                    height = 2 * (m - d);
                    setYU(luy);
                } else if(dok && uok) {
                    luy = u;
                    height = u - d;
                    setYM((d + u) / 2);
                } else if(mok && uok) {
                    luy = u;
                    height = 2 * (u - m);
                    setYD(2 * m - u);
                } else {
                    return;
                }
                setHeight(height);
                qDebug("pok: %d, pw: %d, ph: %d", pok, pwidth, pheight);
                if(pok && pwidth > 0 && pheight > 0) {
                    width = height * pwidth / pheight;
                    setWidth(width);
                    setPixel(height / pheight);
                    double l, n, r;
                    bool lok, nok, rok;
                    getX(l, lok, n, nok, r, rok);
                    if(lok) {
                        lux = l;
                        setXM(l + width / 2);
                        setXR(l + width);
                    } else if(nok) {
                        lux = n - width / 2;
                        setXL(lux);
                        setXR(n + width / 2);
                    } else if(rok) {
                        lux = r - width;
                        setXL(lux);
                        setXM(r - width / 2);
                    }
                }
            } else {
                qDebug("勾选欠饱和错误");
            }
        } else {
            getWH(width, height);
            double l, m1, r, d, m2, u;
            bool lok, m1ok, rok, dok, m2ok, uok;
            getX(l, lok, m1, m1ok, r, rok);
            if(lok) {
                setXM(l + width / 2);
                setXR(l + width);
            } else if(m1ok) {
                setXL(m1 - width / 2);
                setXR(m1 + width / 2);
            } else if(rok) {
                setXL(r - width);
                setXM(r - width / 2);
            }
            getY(d, dok, m2, m2ok, u, uok);
            if(dok) {
                setYM(d + height / 2);
                setYU(d + height);
            } else if(m2ok) {
                setYD(m2 - height / 2);
                setYU(m2 + height / 2);
            } else if(uok) {
                setYD(u - height);
                setYM(u - height / 2);
            }
        }
    }
}

void MainWindow::getX(double&L, bool& lok, double& M, bool& mok, double& R, bool& rok) {
    if(ui->LDRealCheckBox->isChecked() || ui->LURealCheckBox->isChecked()) {
        L = ui->LDRealLineEdit->text().toDouble(&lok);
    } else {
        lok = false;
    }
    if(ui->centerRealCheckBox->isChecked()) {
        M = ui->centerRealLineEdit->text().toDouble(&mok);
    } else {
        mok = false;
    }
    if(ui->RDRealCheckBox->isChecked() || ui->RURealCheckBox->isChecked()) {
        R = ui->RDRealLineEdit->text().toDouble(&rok);
    } else {
        rok = false;
    }
}
void MainWindow::getY(double&D, bool& dok, double& M, bool& mok, double& U, bool& uok) {
    if(ui->LDImagCheckBox->isChecked() || ui->RDImagCheckBox->isChecked()) {
        D = ui->LDImagLineEdit->text().toDouble(&dok);
    } else {
        dok = false;
    }
    if(ui->centerImagCheckBox->isChecked()) {
        M = ui->centerImagLineEdit->text().toDouble(&mok);
    } else {
        mok = false;
    }
    if(ui->LUImagCheckBox->isChecked() || ui->RUImagCheckBox->isChecked()) {
        U = ui->LUImagLineEdit->text().toDouble(&uok);
    } else {
        uok = false;
    }
}
bool MainWindow::getLU(double& x, double& y) {
    bool ok;
    x = ui->LURealLineEdit->text().toDouble(&ok);
    if(!ok) return false;
    y = ui->LUImagLineEdit->text().toDouble(&ok);
    return ok;
}
void MainWindow::setXL(double n) {
    ui->LDRealLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::setXM(double n) {
    ui->centerRealLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::setXR(double n) {
    ui->RDRealLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::setYD(double n) {
    ui->LDImagLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::setYM(double n) {
    ui->centerImagLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::setYU(double n) {
    ui->LUImagLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::getWH(double& width, double& height) {
    width = height = 0;
    width = ui->widthLineEdit->text().toDouble();
    height = ui->heightLineEdit->text().toDouble();
}
void MainWindow::setWidth(double n) {
    ui->widthLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::setHeight(double n) {
    ui->heightLineEdit->setText(QString::number(n, 'g', 15));
}
void MainWindow::setPixel(double n) {
    ui->pixelLineEdit->setText(QString::number(n, 'g', 15));
}
size_t MainWindow::getMaxtimes() {
    return ui->timesSpinBox->value();
}

/**
 * @brief 预览事件
 */
void MainWindow::on_viewButton_clicked() {

    int real_cnt, imag_cnt, final_cnt, sum;
    countChecked(real_cnt, imag_cnt, final_cnt, sum);
    if(!(real_cnt && imag_cnt && sum)) return;
    double width, height;
    getWH(width, height);
    if(width == 0 || height == 0) return;
    double lux, luy;
    if(!getLU(lux, luy)) return;

    int pw = 300, ph = 300;
    if(width > height) {
        ph = pw * height / width;
    } else {
        pw = ph * width / height;
    }

    if(viewImg || viewImgReader || viewCalcMgr) {
        ui->noticeLabel->setText(QString::fromUtf8("预览图已在计算中,暂不能终止."));
        return;
    }

    viewImg = new QImage(pw, ph, QImage::Format_RGB888);
    viewImgReader = new Mandelbrot::RectangleImageReader<double>(
                viewImg, lux, luy, width, height, getMaxtimes(), &timesRender);
    viewCalcMgr = new CalculatorManager(
                *viewImgReader, ui->threadTotalSpinBox->value());
    QObject::connect(viewCalcMgr, SIGNAL(finished(int)),
                     this, SLOT(onViewcalcmgrFinished(int)));
    QObject::connect(viewCalcMgr, SIGNAL(progress(int)),
                     ui->progressBar, SLOT(setValue(int)));
    viewCalcMgr->start();
    ui->noticeLabel->setText(QString::fromUtf8("预览图计算中..."));

    QString cfg = getConfigString();
    if(!strlist.contains(cfg)) {
        strlist << cfg;
        model->setStringList(strlist);
    }
}

/**
 * @brief 预览图形计算完成
 */
void MainWindow::onViewcalcmgrFinished(int ms_time) {
    setViewSize(viewImg->width(), viewImg->height());
    ui->noticeLabel->setText(QString::fromUtf8("计算完毕,用时:%1ms.").arg(ms_time));
    pixmapItem->setPixmap(QPixmap::fromImage(*viewImg));
    delete viewCalcMgr;
    viewCalcMgr = NULL;
    delete viewImgReader;
    viewImgReader = NULL;
    delete viewImg;
    viewImg = NULL;
}

/**
 * @brief 生成事件
 */
void MainWindow::on_generateButton_clicked() {

    int real_cnt, imag_cnt, final_cnt, sum;
    countChecked(real_cnt, imag_cnt, final_cnt, sum);
    if(!(real_cnt && imag_cnt && sum)) return;
    double width, height;
    getWH(width, height);
    if(width == 0 || height == 0) return;
    double lux, luy;
    if(!getLU(lux, luy)) return;

    QString filename = ui->filenameLineEdit->text();
    if(!isFilename(filename)) return;

    int pw, ph;
    {
        bool ok;
        pw = ui->widthPixelLineEdit->text().toInt(&ok);
        if(!ok) return;
        ph = ui->heightPixelLineEdit->text().toInt(&ok);
        if(!ok) return;
        if(pw <= 0 || ph <= 0) return;
    }

    if(geneImg || geneImgReader || geneCalcMgr) {
        ui->noticeLabel->setText(QString::fromUtf8("图片已在计算中,暂不能终止."));
        return;
    }

    geneImg = new QImage(pw, ph, QImage::Format_RGB888);
    geneImgReader = new Mandelbrot::RectangleImageReader<double>(
                geneImg, lux, luy, width, height, getMaxtimes(), &timesRender);
    geneCalcMgr = new CalculatorManager(
                *geneImgReader, ui->threadTotalSpinBox->value());
    QObject::connect(geneCalcMgr, SIGNAL(progress(int)),
                     ui->progressBar, SLOT(setValue(int)));
    QObject::connect(geneCalcMgr, SIGNAL(finished(int)),
                     this, SLOT(onGenecalcmgrFinished(int)));
    geneCalcMgr->start();
    ui->noticeLabel->setText(QString::fromUtf8("图片计算中..."));
}

void MainWindow::onGenecalcmgrFinished(int ms_time) {
    QString filename = ui->filenameLineEdit->text();
    ui->noticeLabel->setText(QString::fromUtf8("生成完毕,用时:%1ms,已保存到\"%2\".").arg(ms_time).arg(filename));
    geneImg->save(filename);
    delete geneCalcMgr;
    geneCalcMgr = NULL;
    delete geneImgReader;
    geneImgReader = NULL;
    delete geneImg;
    geneImg = NULL;
}

/**
 * @brief 开则排斥其他勾选框,关则清除输入和错误标记
 * 互斥输入框排斥其他输入框时其内容文字不清除
 */
#define SINGLE_CHECKBOX                           \
    this_lineedit->setReadOnly(!arg1);            \
    if(arg1) {                                    \
        if(this_lineedit->text().length() > 0) {  \
            checkNumber(this_lineedit);           \
        }                                         \
    } else {                                      \
        changeErrorMark(this_lineedit, false);    \
    }                                             \
    lockCheckBox();                               \
    calc();

#define DOUBLE_CHECKBOX                           \
    this_lineedit->setReadOnly(!arg1);            \
    if(arg1) {                                    \
        QString ctx = this_lineedit->text();      \
        twin_checkbox->setChecked(false);         \
        this_lineedit->setText(ctx);              \
        if(this_lineedit->text().length() > 0) {  \
            checkNumber(this_lineedit);           \
        }                                         \
    } else {                                      \
        changeErrorMark(this_lineedit, false);    \
    }                                             \
    lockCheckBox();                               \
    calc();

#define TRIPLE_CHECKBOX                           \
    this_lineedit->setReadOnly(!arg1);            \
    if(arg1) {                                    \
        QString ctx = this_lineedit->text();      \
        twin1_checkbox->setChecked(false);        \
        twin2_checkbox->setChecked(false);        \
        this_lineedit->setText(ctx);              \
        if(this_lineedit->text().length() > 0) {  \
            checkNumber(this_lineedit);           \
        }                                         \
    } else {                                      \
        changeErrorMark(this_lineedit, false);    \
    }                                             \
    lockCheckBox();                               \
    calc();

void MainWindow::on_centerRealCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->centerRealCheckBox
#define this_lineedit ui->centerRealLineEdit
    SINGLE_CHECKBOX
#undef this_lineedit
#undef this_checkbox
}
void MainWindow::on_centerImagCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->centerImagCheckBox
#define this_lineedit ui->centerImagLineEdit
    SINGLE_CHECKBOX
#undef this_lineedit
#undef this_checkbox
}

void MainWindow::on_LUImagCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->LUImagCheckBox
#define this_lineedit ui->LUImagLineEdit
#define twin_checkbox ui->RUImagCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}
void MainWindow::on_RUImagCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->RUImagCheckBox
#define this_lineedit ui->RUImagLineEdit
#define twin_checkbox ui->LUImagCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}

void MainWindow::on_LURealCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->LURealCheckBox
#define this_lineedit ui->LURealLineEdit
#define twin_checkbox ui->LDRealCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}
void MainWindow::on_LDRealCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->LDRealCheckBox
#define this_lineedit ui->LDRealLineEdit
#define twin_checkbox ui->LURealCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}

void MainWindow::on_RURealCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->RURealCheckBox
#define this_lineedit ui->RURealLineEdit
#define twin_checkbox ui->RDRealCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}
void MainWindow::on_RDRealCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->RDRealCheckBox
#define this_lineedit ui->RDRealLineEdit
#define twin_checkbox ui->RURealCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}

void MainWindow::on_LDImagCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->LDImagCheckBox
#define this_lineedit ui->LDImagLineEdit
#define twin_checkbox ui->RDImagCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}
void MainWindow::on_RDImagCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->RDImagCheckBox
#define this_lineedit ui->RDImagLineEdit
#define twin_checkbox ui->LDImagCheckBox
    DOUBLE_CHECKBOX
#undef twin_checkbox
#undef this_lineedit
#undef this_checkbox
}

void MainWindow::on_pixelCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->pixelCheckBox
#define this_lineedit ui->pixelLineEdit
#define twin1_checkbox ui->widthCheckBox
#define twin2_checkbox ui->heightCheckBox
    TRIPLE_CHECKBOX
#undef twin2_checkbox
#undef twin1_checkbox
#undef this_lineedit
#undef this_checkbox
}
void MainWindow::on_widthCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->widthCheckBox
#define this_lineedit ui->widthLineEdit
#define twin1_checkbox ui->pixelCheckBox
#define twin2_checkbox ui->heightCheckBox
    TRIPLE_CHECKBOX
#undef twin2_checkbox
#undef twin1_checkbox
#undef this_lineedit
#undef this_checkbox
}
void MainWindow::on_heightCheckBox_stateChanged(int arg1) {
#define this_checkbox ui->heightCheckBox
#define this_lineedit ui->heightLineEdit
#define twin1_checkbox ui->pixelCheckBox
#define twin2_checkbox ui->widthCheckBox
    TRIPLE_CHECKBOX
#undef twin2_checkbox
#undef twin1_checkbox
#undef this_lineedit
#undef this_checkbox
}

/**
 * @brief 互斥输入框文字实时计算和同步
 */

void MainWindow::on_widthPixelLineEdit_textChanged(const QString &arg1) {
    arg1.length();
    calc();
}
void MainWindow::on_heightPixelLineEdit_textChanged(const QString &arg1) {
    arg1.length();
    calc();
}

void MainWindow::on_centerRealLineEdit_textChanged(const QString &arg1) {
    arg1.length();
    calc();
}
void MainWindow::on_centerImagLineEdit_textChanged(const QString &arg1) {
    arg1.length();
    calc();
}

void MainWindow::on_LUImagLineEdit_textChanged(const QString &arg1) {
    ui->RUImagLineEdit->setText(arg1);
    calc();
}
void MainWindow::on_RUImagLineEdit_textChanged(const QString &arg1) {
    ui->LUImagLineEdit->setText(arg1);
    calc();
}

void MainWindow::on_RDImagLineEdit_textChanged(const QString &arg1) {
    ui->LDImagLineEdit->setText(arg1);
    calc();
}
void MainWindow::on_LDImagLineEdit_textChanged(const QString &arg1) {
    ui->RDImagLineEdit->setText(arg1);
    calc();
}

void MainWindow::on_RURealLineEdit_textChanged(const QString &arg1) {
    ui->RDRealLineEdit->setText(arg1);
    calc();
}
void MainWindow::on_RDRealLineEdit_textChanged(const QString &arg1) {
    ui->RURealLineEdit->setText(arg1);
    calc();
}

void MainWindow::on_LDRealLineEdit_textChanged(const QString &arg1) {
    ui->LURealLineEdit->setText(arg1);
    calc();
}
void MainWindow::on_LURealLineEdit_textChanged(const QString &arg1) {
    ui->LDRealLineEdit->setText(arg1);
    calc();
}

void MainWindow::on_pixelLineEdit_textChanged(const QString &arg1) {
    arg1.length();
    calc();
}
void MainWindow::on_widthLineEdit_textChanged(const QString &arg1) {
    arg1.length();
    calc();
}
void MainWindow::on_heightLineEdit_textChanged(const QString &arg1) {
    arg1.length();
    calc();
}

void MainWindow::on_threadTotalSpinBox_valueChanged(int arg1) {
    ui->threadTotalSlider->setValue(arg1);
}
void MainWindow::on_threadTotalSlider_valueChanged(int value) {
    ui->threadTotalSpinBox->setValue(value);
}

void MainWindow::on_timesSpinBox_valueChanged(int arg1) {
    arg1 = arg1;
}
void MainWindow::on_timespowerSlider_valueChanged(int value) {
    ui->timesSpinBox->setValue(pow(2, value) - 1);
}

/**
 * @brief 输入框文字格式检查(整数,实数,文件名)
 */
void MainWindow::on_widthPixelLineEdit_editingFinished() {
    checkInteger(ui->widthPixelLineEdit);
}
void MainWindow::on_heightPixelLineEdit_editingFinished() {
    checkInteger(ui->heightPixelLineEdit);
}
void MainWindow::on_centerRealLineEdit_editingFinished() {
    if(ui->centerRealCheckBox->isChecked())checkNumber(ui->centerRealLineEdit);
}
void MainWindow::on_centerImagLineEdit_editingFinished() {
    if(ui->centerImagCheckBox->isChecked())checkNumber(ui->centerImagLineEdit);
}
void MainWindow::on_LURealLineEdit_editingFinished() {
    if(ui->LURealCheckBox->isChecked())checkNumber(ui->LURealLineEdit);
}
void MainWindow::on_LUImagLineEdit_editingFinished() {
    if(ui->LUImagCheckBox->isChecked())checkNumber(ui->LUImagLineEdit);
}
void MainWindow::on_RURealLineEdit_editingFinished() {
    if(ui->RURealCheckBox->isChecked())checkNumber(ui->RURealLineEdit);
}
void MainWindow::on_RUImagLineEdit_editingFinished() {
    if(ui->RUImagCheckBox->isChecked())checkNumber(ui->RUImagLineEdit);
}
void MainWindow::on_RDRealLineEdit_editingFinished() {
    if(ui->RDRealCheckBox->isChecked())checkNumber(ui->RDRealLineEdit);
}
void MainWindow::on_RDImagLineEdit_editingFinished() {
    if(ui->RDImagCheckBox->isChecked())checkNumber(ui->RDImagLineEdit);
}
void MainWindow::on_LDRealLineEdit_editingFinished() {
    if(ui->LDRealCheckBox->isChecked())checkNumber(ui->LDRealLineEdit);
}
void MainWindow::on_LDImagLineEdit_editingFinished() {
    if(ui->LDImagCheckBox->isChecked())checkNumber(ui->LDImagLineEdit);
}
void MainWindow::on_pixelLineEdit_editingFinished() {
    if(ui->pixelCheckBox->isChecked())checkNumber(ui->pixelLineEdit);
}
void MainWindow::on_widthLineEdit_editingFinished() {
    if(ui->widthCheckBox->isChecked())checkNumber(ui->widthLineEdit);
}
void MainWindow::on_heightLineEdit_editingFinished() {
    if(ui->heightCheckBox->isChecked())checkNumber(ui->heightLineEdit);
}
void MainWindow::on_filenameLineEdit_editingFinished() {
    QString str = ui->filenameLineEdit->text();
    if(str.length() == 0) {
        ui->filenameLineEdit->setText("output.bmp");
    } else {
        if(!str.endsWith(".bmp", Qt::CaseInsensitive)) {
            ui->filenameLineEdit->setText(str + ".bmp");
        }
    }
    checkFilename(ui->filenameLineEdit);
}

/**
 * @brief 取屏幕分辨率
 */
void MainWindow::on_getScreenSizePushButton_clicked() {
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect r = desktopWidget->screenGeometry();
    ui->heightPixelLineEdit->setText(QString::number(r.height()));
    ui->widthPixelLineEdit->setText(QString::number(r.width()));
}

/**
 * @brief 取推荐文件名
 */
void MainWindow::on_getNiceFilenamePushButton_clicked() {
    ui->filenameLineEdit->setText(getConfigString() + ".bmp");
}

QString MainWindow::getConfigString() const {
    // M_1920x1080_cx-0.416_cy0.547_pd2e-06_t255
    QString n = QString("M_%1x%2").arg(ui->widthPixelLineEdit->text()).arg(ui->heightPixelLineEdit->text());
    struct {
        const char* str;
        QCheckBox* cb;
        QLineEdit* le;
    } const scl[] = {
        {"cx", ui->centerRealCheckBox, ui->centerRealLineEdit},
        {"cy", ui->centerImagCheckBox, ui->centerImagLineEdit},

        {"l", ui->LURealCheckBox, ui->LURealLineEdit},
        {"l", ui->LDRealCheckBox, ui->LDRealLineEdit},

        {"r", ui->RURealCheckBox, ui->RURealLineEdit},
        {"r", ui->RDRealCheckBox, ui->RDRealLineEdit},

        {"d", ui->LDImagCheckBox, ui->LDImagLineEdit},
        {"d", ui->RDImagCheckBox, ui->RDImagLineEdit},

        {"u", ui->LUImagCheckBox, ui->LUImagLineEdit},
        {"u", ui->RUImagCheckBox, ui->RUImagLineEdit},

        {"pd", ui->pixelCheckBox, ui->pixelLineEdit},
        {"w", ui->widthCheckBox, ui->widthLineEdit},
        {"h", ui->heightCheckBox, ui->heightLineEdit},
        {0, 0, 0}
    };
    for(int i = 0; scl[i].str; i++) {
        if(scl[i].cb->isChecked()) {
            n.append(QString("_") + scl[i].str + scl[i].le->text());
        }
    }
    n.append("_t" + ui->timesSpinBox->text());
    return n;
}

QString MainWindow::setConfigString(QString const& str) {
    struct {
        const char* str;
        QCheckBox* cb;
        QLineEdit* le;
    } const scl[] = {
        {"cx", ui->centerRealCheckBox, ui->centerRealLineEdit},
        {"cy", ui->centerImagCheckBox, ui->centerImagLineEdit},

        {"l", ui->LURealCheckBox, ui->LURealLineEdit},
        {"l", ui->LDRealCheckBox, ui->LDRealLineEdit},

        {"r", ui->RURealCheckBox, ui->RURealLineEdit},
        {"r", ui->RDRealCheckBox, ui->RDRealLineEdit},

        {"d", ui->LDImagCheckBox, ui->LDImagLineEdit},
        {"d", ui->RDImagCheckBox, ui->RDImagLineEdit},

        {"u", ui->LUImagCheckBox, ui->LUImagLineEdit},
        {"u", ui->RUImagCheckBox, ui->RUImagLineEdit},

        {"pd", ui->pixelCheckBox, ui->pixelLineEdit},
        {"w", ui->widthCheckBox, ui->widthLineEdit},
        {"h", ui->heightCheckBox, ui->heightLineEdit},
        {0, 0, 0}
    };

    QRegExp ptn1("\\{M_(\\d+)x(\\d+)([^}]*)_t(\\d*)\\}");
    int pos;
    bool ok;

    pos = ptn1.indexIn(str);
    if(pos == -1) return QString::fromUtf8("第一语法匹配失败");
    ui->widthPixelLineEdit->setText(ptn1.cap(1));
    ui->heightPixelLineEdit->setText(ptn1.cap(2));
    ui->timesSpinBox->setValue(ptn1.cap(4).toInt(&ok));
    if(!ok) return QString::fromUtf8("迭代次数非数字");

    for(int i = 0; scl[i].str; i++) {
        scl[i].cb->setChecked(false);
    }

    pos = 0;
    QString sub1 = ptn1.cap(3);
    QRegExp ptn2("_([a-zA-Z]*)([^_]*)");
    int cb_cnt = 0;
    while((pos = ptn2.indexIn(sub1, pos)) != -1) {
        bool found = false;
        for(int i = 0; scl[i].str; i++) {
            QString w = QString::fromUtf8(scl[i].str);
            qDebug() << ptn2.cap(1) << ", " << w << ", " << ptn2.cap(1).compare(w);
            if(ptn2.cap(1).compare(w) == 0) {
                qDebug() << "w: " << w;
                cb_cnt++;
                scl[i].cb->setChecked(true);
                scl[i].le->setText(ptn2.cap(2));
                pos += ptn2.matchedLength();
                found = true;
                break;
            }
        }
        if(!found) return QString::fromUtf8("未知提示词\"\"").arg(ptn2.cap(1));
    }

    if(cb_cnt != 3) return QString::fromUtf8("必要信息不足三项");
    return "";
}

void MainWindow::on_copyConfigPushButton_clicked() {
    QString s = QString("{%1}").arg(getConfigString());
    QApplication::clipboard()->setText(s);
    ui->noticeLabel->setText(QString::fromUtf8("配置复制完毕"));
}

void MainWindow::on_pasteConfigPushButton_clicked() {
    QString info = setConfigString(QApplication::clipboard()->text());
    if(info.length() != 0) {
        ui->noticeLabel->setText(QString::fromUtf8("设置失败: ") + info + ".");
    } else {
        ui->getNiceFilenamePushButton->click();
        ui->viewButton->click();
    }
}

void MainWindow::on_openHistoryPushButton_clicked() {
    bool status = ui->historyListView->isVisible();
    ui->historyListView->setVisible(!status);
    ui->historyLine->setVisible(!status);
    ui->historyLabel->setVisible(!status);
}

void MainWindow::on_historyListView_doubleClicked(const QModelIndex &index) {
    QString info = setConfigString("{" + strlist[index.row()] + "}");
    if(info.length() != 0) {
        ui->noticeLabel->setText(QString::fromUtf8("异常配置: %1, 请报告开发者.").arg(info));
    } else {
        ui->viewButton->click();
    }
}

void MainWindow::on_editSenderPushButton_clicked() {
    bool status = ui->shaderTextEdit->isVisible();
    if(status == true) { // 应用着色器
        QString errstr = timesRender.read_string(ui->shaderTextEdit->toPlainText());
        ui->shaderErrorLabel->setText(errstr);
        if(errstr.length() == 0) { // 成功
            ui->editSenderPushButton->setText(QString::fromUtf8("编辑着色器"));
            ui->shaderTextEdit->setVisible(false);
            ui->shaderLabel->setVisible(false);
            ui->shaderErrorLabel->setVisible(false);
            ui->shaderLine->setVisible(false);
        }
    } else { // 编辑着色器
        ui->editSenderPushButton->setText(QString::fromUtf8("应用着色器"));
        ui->shaderLine->setVisible(true);
        ui->shaderTextEdit->setVisible(true);
        ui->shaderLabel->setVisible(true);
        ui->shaderErrorLabel->setVisible(true);
    }
}
