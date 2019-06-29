#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mandelbrot.h>
#include "calculatormanager.h"
#include "timesrender.h"

class QGraphicsScene;
class QGraphicsLineItem;
class QCheckBox;
class QGraphicsPixmapItem;
class QStringListModel;
class QModelIndex;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_viewButton_clicked();
    void on_filenameLineEdit_editingFinished();
    void on_LUImagCheckBox_stateChanged(int arg1);
    void on_RUImagCheckBox_stateChanged(int arg1);
    void on_LURealCheckBox_stateChanged(int arg1);
    void on_LDRealCheckBox_stateChanged(int arg1);
    void on_RURealCheckBox_stateChanged(int arg1);
    void on_RDRealCheckBox_stateChanged(int arg1);
    void on_LDImagCheckBox_stateChanged(int arg1);
    void on_RDImagCheckBox_stateChanged(int arg1);
    void on_LUImagLineEdit_textChanged(const QString &arg1);
    void on_RUImagLineEdit_textChanged(const QString &arg1);
    void on_RDImagLineEdit_textChanged(const QString &arg1);
    void on_LDImagLineEdit_textChanged(const QString &arg1);
    void on_RURealLineEdit_textChanged(const QString &arg1);
    void on_RDRealLineEdit_textChanged(const QString &arg1);
    void on_LDRealLineEdit_textChanged(const QString &arg1);
    void on_LURealLineEdit_textChanged(const QString &arg1);
    void on_centerRealCheckBox_stateChanged(int arg1);
    void on_widthPixelLineEdit_editingFinished();
    void on_heightPixelLineEdit_editingFinished();
    void on_centerRealLineEdit_editingFinished();
    void on_centerImagLineEdit_editingFinished();
    void on_LURealLineEdit_editingFinished();
    void on_LUImagLineEdit_editingFinished();
    void on_RURealLineEdit_editingFinished();
    void on_RUImagLineEdit_editingFinished();
    void on_RDRealLineEdit_editingFinished();
    void on_RDImagLineEdit_editingFinished();
    void on_LDRealLineEdit_editingFinished();
    void on_LDImagLineEdit_editingFinished();
    void on_pixelLineEdit_editingFinished();
    void on_widthLineEdit_editingFinished();
    void on_heightLineEdit_editingFinished();
    void on_centerImagCheckBox_stateChanged(int arg1);
    void on_pixelCheckBox_stateChanged(int arg1);
    void on_widthCheckBox_stateChanged(int arg1);
    void on_heightCheckBox_stateChanged(int arg1);
    void on_pixelLineEdit_textChanged(const QString &arg1);
    void on_widthLineEdit_textChanged(const QString &arg1);
    void on_heightLineEdit_textChanged(const QString &arg1);
    void on_widthPixelLineEdit_textChanged(const QString &arg1);
    void on_heightPixelLineEdit_textChanged(const QString &arg1);
    void on_centerRealLineEdit_textChanged(const QString &arg1);
    void on_centerImagLineEdit_textChanged(const QString &arg1);
    void on_generateButton_clicked();
    void on_getScreenSizePushButton_clicked();
    void on_threadTotalSlider_valueChanged(int value);
    void on_timespowerSlider_valueChanged(int value);
    void on_timesSpinBox_valueChanged(int arg1);
    void on_threadTotalSpinBox_valueChanged(int arg1);
    void on_getNiceFilenamePushButton_clicked();
    void on_copyConfigPushButton_clicked();
    void on_pasteConfigPushButton_clicked();

    void onViewcalcmgrFinished(int ms_time);
    void onGenecalcmgrFinished(int ms_time);

    void on_openHistoryPushButton_clicked();
    void on_historyListView_doubleClicked(const QModelIndex &index);

    void on_editSenderPushButton_clicked();

private:
    Ui::MainWindow *ui;
    QGraphicsScene* scene;
    QGraphicsPixmapItem* pixmapItem;

    CalculatorManager* viewCalcMgr;
    QImage* viewImg;
    Mandelbrot::RectangleImageReader<double>* viewImgReader;

    CalculatorManager* geneCalcMgr;
    QImage* geneImg;
    Mandelbrot::RectangleImageReader<double>* geneImgReader;

    QStringList strlist;
    QStringListModel* model;

    TimesRender timesRender;

    void setViewSize(int w, int h);
    void countChecked(int& real_cnt, int& imag_cnt, int& final_cnt, int& s);
    void lockCheckBox();
    void calc();
    bool calcFinal(int* pw = 0, int* ph = 0);
    QString getConfigString() const;
    QString setConfigString(QString const& str);

    void getX(double&L, bool& lok, double& M, bool& mok, double& R, bool& rok);
    void getY(double&D, bool& dok, double& M, bool& mok, double& U, bool& uok);
    bool getLU(double& x, double& y);
    void setXL(double n);
    void setXM(double n);
    void setXR(double n);
    void setYD(double n);
    void setYM(double n);
    void setYU(double n);
    void getWH(double& width, double& height);
    void setWidth(double n);
    void setHeight(double n);
    void setPixel(double n);
    size_t getMaxtimes();
};

#endif // MAINWINDOW_H
