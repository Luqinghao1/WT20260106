/*
 * mainwindow.h
 * 文件作用：主窗口类头文件
 * 功能描述：
 * 1. 声明主窗口框架及各个子功能模块指针。
 * 2. 引入 ModelManager 头文件以访问模型系统。
 * 注意：由于 ModelManager 内部实现变化，确保包含正确的头文件链。
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QTimer>
#include <QStandardItemModel>
#include "modelmanager.h"

class NavBtn;
class WT_ProjectWidget;
class DataEditorWidget;
class WT_PlottingWidget;
class FittingPage;
class SettingsWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();

    void initProjectForm();
    void initDataEditorForm();
    void initModelForm();
    void initPlottingForm();
    void initFittingForm();

private slots:
    void onProjectOpened(bool isNew);
    void onProjectClosed();
    void onFileLoaded(const QString& filePath, const QString& fileType);

    void onPlotAnalysisCompleted(const QString &analysisType, const QMap<QString, double> &results);
    void onDataReadyForPlotting();
    void onTransferDataToPlotting();
    void onDataEditorDataChanged();

    void onSystemSettingsChanged();
    void onPerformanceSettingsChanged();
    void onModelCalculationCompleted(const QString &analysisType, const QMap<QString, double> &results);
    void onFittingProgressChanged(int progress);

private:
    Ui::MainWindow *ui;

    WT_ProjectWidget* m_ProjectWidget;
    DataEditorWidget* m_DataEditorWidget;
    ModelManager* m_ModelManager;
    WT_PlottingWidget* m_PlottingWidget;
    FittingPage* m_FittingPage;
    SettingsWidget* m_SettingsWidget;

    QMap<QString, NavBtn*> m_NavBtnMap;
    QTimer m_timer;
    bool m_hasValidData = false;
    bool m_isProjectLoaded = false;

    void transferDataFromEditorToPlotting();
    void updateNavigationState();
    void transferDataToFitting();

    QStandardItemModel* getDataEditorModel() const;
    QString getCurrentFileName() const;
    bool hasDataLoaded();

    QString getMessageBoxStyle() const;
};

#endif // MAINWINDOW_H
