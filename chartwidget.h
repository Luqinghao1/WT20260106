/*
 * chartwidget.h
 * 文件作用: 通用图表组件头文件
 * 功能描述:
 * 1. 封装 MouseZoom，提供统一的图表展示界面。
 * 2. 接收 MouseZoom 的菜单信号，执行具体业务逻辑。
 * 3. 实现了复杂的鼠标交互（移动、拉伸、标注）。
 */

#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QMenu>
#include <QMap>
#include <QMouseEvent>
#include "mousezoom.h"

namespace Ui {
class ChartWidget;
}

struct ChartAnnotation {
    QCPItemText* textItem = nullptr;
    QCPItemLine* arrowItem = nullptr;
};

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    enum ChartMode {
        Mode_Single = 0,
        Mode_Stacked
    };

    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

    void setTitle(const QString& title);
    MouseZoom* getPlot();
    void setDataModel(QStandardItemModel* model);

    void setChartMode(ChartMode mode);
    ChartMode getChartMode() const;
    QCPAxisRect* getTopRect();
    QCPAxisRect* getBottomRect();

signals:
    void exportDataTriggered();

private slots:
    // --- UI按钮槽函数 (Qt会自动连接这些 slots，不要手动 connect) ---
    void on_btnSavePic_clicked();
    void on_btnExportData_clicked();
    void on_btnSetting_clicked();
    void on_btnReset_clicked();
    void on_btnDrawLine_clicked();

    // --- 来自 MouseZoom 的信号处理 ---
    void onAddAnnotationRequested(QCPItemLine* line);
    void onDeleteSelectedRequested();
    void onEditItemRequested(QCPAbstractItem* item);

    // --- 鼠标交互逻辑 ---
    void onPlotMousePress(QMouseEvent* event);
    void onPlotMouseMove(QMouseEvent* event);
    void onPlotMouseRelease(QMouseEvent* event);
    void onPlotMouseDoubleClick(QMouseEvent* event);

    // --- 业务功能 ---
    void addCharacteristicLine(double slope);
    void addAnnotationToLine(QCPItemLine* line);
    void deleteSelectedItems();

private:
    void initUi();
    void initConnections();

    void calculateLinePoints(double slope, double centerX, double centerY,
                             double& x1, double& y1, double& x2, double& y2,
                             bool isLogX, bool isLogY);

    double distToSegment(const QPointF& p, const QPointF& s, const QPointF& e);
    void constrainLinePoint(QCPItemLine* line, bool isMovingStart, double mouseX, double mouseY);
    void updateAnnotationArrow(QCPItemLine* line);

private:
    Ui::ChartWidget *ui;
    MouseZoom* m_plot;
    QStandardItemModel* m_dataModel;
    QMenu* m_lineMenu;

    ChartMode m_chartMode;
    QCPAxisRect* m_topRect;
    QCPAxisRect* m_bottomRect;

    QMap<QCPItemLine*, ChartAnnotation> m_annotations;

    enum InteractionMode {
        Mode_None,
        Mode_Dragging_Line,
        Mode_Dragging_Start,
        Mode_Dragging_End,
        Mode_Dragging_Text,
        Mode_Dragging_ArrowStart,
        Mode_Dragging_ArrowEnd
    };

    InteractionMode m_interMode;
    QCPItemLine* m_activeLine;
    QCPItemText* m_activeText;
    QCPItemLine* m_activeArrow;
    QPointF m_lastMousePos;
};

#endif // CHARTWIDGET_H
