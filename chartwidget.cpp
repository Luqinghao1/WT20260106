/*
 * chartwidget.cpp
 * 文件作用: 通用图表组件实现文件
 * 功能描述:
 * 1. 修复了重复连接导致的双重弹窗问题。
 * 2. 移除了不存在的右键菜单槽函数连接。
 * 3. 包含完整的交互逻辑（拖拽、标注、斜率线）。
 */

#include "chartwidget.h"
#include "ui_chartwidget.h"
#include "chartsetting1.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QInputDialog>
#include <cmath>

ChartWidget::ChartWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChartWidget),
    m_dataModel(nullptr),
    m_chartMode(Mode_Single),
    m_topRect(nullptr),
    m_bottomRect(nullptr),
    m_interMode(Mode_None),
    m_activeLine(nullptr),
    m_activeText(nullptr),
    m_activeArrow(nullptr)
{
    ui->setupUi(this);
    m_plot = ui->chart; // ui->chart 是 MouseZoom 类型

    initUi();
    initConnections();
}

ChartWidget::~ChartWidget()
{
    delete ui;
}

void ChartWidget::initUi()
{
    m_lineMenu = new QMenu(this);
    QAction* actSlope1 = m_lineMenu->addAction("斜率 k = 1 (井筒储集)");
    connect(actSlope1, &QAction::triggered, this, [=](){ addCharacteristicLine(1.0); });

    QAction* actSlopeHalf = m_lineMenu->addAction("斜率 k = 1/2 (线性流)");
    connect(actSlopeHalf, &QAction::triggered, this, [=](){ addCharacteristicLine(0.5); });

    QAction* actSlopeQuarter = m_lineMenu->addAction("斜率 k = 1/4 (双线性流)");
    connect(actSlopeQuarter, &QAction::triggered, this, [=](){ addCharacteristicLine(0.25); });

    QAction* actHorizontal = m_lineMenu->addAction("水平线 (径向流)");
    connect(actHorizontal, &QAction::triggered, this, [=](){ addCharacteristicLine(0.0); });

    // 基础交互设置
    m_plot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    m_plot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
}

void ChartWidget::initConnections()
{
    // [修正] 移除 onContextMenuRequest 连接
    // 右键菜单现在完全由 MouseZoom 内部生成，并通过下面的特定信号通知 ChartWidget

    // --- 连接 MouseZoom 的自定义信号 (处理右键菜单动作) ---
    connect(m_plot, &MouseZoom::saveImageRequested, this, &ChartWidget::on_btnSavePic_clicked);
    connect(m_plot, &MouseZoom::exportDataRequested, this, &ChartWidget::on_btnExportData_clicked);
    connect(m_plot, &MouseZoom::drawLineRequested, this, &ChartWidget::addCharacteristicLine);
    connect(m_plot, &MouseZoom::settingsRequested, this, &ChartWidget::on_btnSetting_clicked);
    connect(m_plot, &MouseZoom::resetViewRequested, this, &ChartWidget::on_btnReset_clicked);

    connect(m_plot, &MouseZoom::addAnnotationRequested, this, &ChartWidget::onAddAnnotationRequested);
    connect(m_plot, &MouseZoom::deleteSelectedRequested, this, &ChartWidget::onDeleteSelectedRequested);
    connect(m_plot, &MouseZoom::editItemRequested, this, &ChartWidget::onEditItemRequested);

    // --- 接管鼠标左键操作 (实现拖拽/拉伸/双击) ---
    connect(m_plot, &QCustomPlot::mousePress, this, &ChartWidget::onPlotMousePress);
    connect(m_plot, &QCustomPlot::mouseMove, this, &ChartWidget::onPlotMouseMove);
    connect(m_plot, &QCustomPlot::mouseRelease, this, &ChartWidget::onPlotMouseRelease);
    connect(m_plot, &QCustomPlot::mouseDoubleClick, this, &ChartWidget::onPlotMouseDoubleClick);

    // [修正] 移除了 ui->btnXXX 的手动 connect
    // 因为 ui->setupUi() 已经根据命名规则自动连接了 on_btnXXX_clicked 槽函数
    // 再次手动连接会导致槽函数执行两次，从而出现两个弹窗
}

// ---------------- 基础功能 ----------------

void ChartWidget::setTitle(const QString &title) { ui->labelTitle->setText(title); }
MouseZoom *ChartWidget::getPlot() { return m_plot; }
void ChartWidget::setDataModel(QStandardItemModel *model) { m_dataModel = model; }

void ChartWidget::setChartMode(ChartMode mode) {
    if (m_chartMode == mode) return;
    m_chartMode = mode;
    m_plot->plotLayout()->clear();

    if (mode == Mode_Single) {
        QCPAxisRect* defaultRect = new QCPAxisRect(m_plot);
        m_plot->plotLayout()->addElement(0, 0, defaultRect);
        m_topRect = nullptr; m_bottomRect = nullptr;
    } else if (mode == Mode_Stacked) {
        m_topRect = new QCPAxisRect(m_plot);
        m_bottomRect = new QCPAxisRect(m_plot);
        m_plot->plotLayout()->addElement(0, 0, m_topRect);
        m_plot->plotLayout()->addElement(1, 0, m_bottomRect);

        m_topRect->setRangeDrag(Qt::Horizontal | Qt::Vertical);
        m_topRect->setRangeZoom(Qt::Horizontal | Qt::Vertical);
        m_bottomRect->setRangeDrag(Qt::Horizontal | Qt::Vertical);
        m_bottomRect->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    }
    m_plot->replot();
}

ChartWidget::ChartMode ChartWidget::getChartMode() const { return m_chartMode; }
QCPAxisRect* ChartWidget::getTopRect() { if (m_chartMode == Mode_Single) return m_plot->axisRect(); return m_topRect; }
QCPAxisRect* ChartWidget::getBottomRect() { if (m_chartMode == Mode_Single) return nullptr; return m_bottomRect; }

// 这些槽函数会被 Qt 的 MetaObject 自动调用
void ChartWidget::on_btnSavePic_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存图片", "chart.png", "PNG (*.png);;JPG (*.jpg);;PDF (*.pdf)");
    if (fileName.isEmpty()) return;
    if (fileName.endsWith(".png")) m_plot->savePng(fileName);
    else if (fileName.endsWith(".jpg")) m_plot->saveJpg(fileName);
    else m_plot->savePdf(fileName);
}

void ChartWidget::on_btnExportData_clicked() { emit exportDataTriggered(); }
void ChartWidget::on_btnSetting_clicked() { ChartSetting1 dlg(m_plot, nullptr, this); dlg.exec(); }
void ChartWidget::on_btnReset_clicked() {
    m_plot->rescaleAxes();
    if(m_plot->xAxis->scaleType()==QCPAxis::stLogarithmic && m_plot->xAxis->range().lower<=0) m_plot->xAxis->setRangeLower(1e-3);
    if(m_plot->yAxis->scaleType()==QCPAxis::stLogarithmic && m_plot->yAxis->range().lower<=0) m_plot->yAxis->setRangeLower(1e-3);
    m_plot->replot();
}
void ChartWidget::on_btnDrawLine_clicked() { m_lineMenu->exec(ui->btnDrawLine->mapToGlobal(QPoint(0, ui->btnDrawLine->height()))); }

// ---------------- 标识线绘制与修复 ----------------

void ChartWidget::addCharacteristicLine(double slope)
{
    QCPAxisRect* rect = m_plot->axisRect();
    if (m_chartMode == Mode_Stacked && m_topRect) rect = m_topRect;

    double lowerX = rect->axis(QCPAxis::atBottom)->range().lower;
    double upperX = rect->axis(QCPAxis::atBottom)->range().upper;
    double lowerY = rect->axis(QCPAxis::atLeft)->range().lower;
    double upperY = rect->axis(QCPAxis::atLeft)->range().upper;

    bool isLogX = (rect->axis(QCPAxis::atBottom)->scaleType() == QCPAxis::stLogarithmic);
    bool isLogY = (rect->axis(QCPAxis::atLeft)->scaleType() == QCPAxis::stLogarithmic);

    double centerX = isLogX ? pow(10, (log10(lowerX) + log10(upperX)) / 2.0) : (lowerX + upperX) / 2.0;
    double centerY = isLogY ? pow(10, (log10(lowerY) + log10(upperY)) / 2.0) : (lowerY + upperY) / 2.0;

    double x1, y1, x2, y2;
    calculateLinePoints(slope, centerX, centerY, x1, y1, x2, y2, isLogX, isLogY);

    QCPItemLine* line = new QCPItemLine(m_plot);
    line->setClipAxisRect(rect);
    line->start->setCoords(x1, y1);
    line->end->setCoords(x2, y2);

    QPen pen(Qt::black, 2, Qt::DashLine);
    line->setPen(pen);
    line->setSelectedPen(QPen(Qt::blue, 2, Qt::SolidLine));

    line->setProperty("fixedSlope", slope);
    line->setProperty("isLogLog", (isLogX && isLogY));
    line->setProperty("isCharacteristic", true);

    m_plot->replot();
}

void ChartWidget::calculateLinePoints(double slope, double centerX, double centerY,
                                      double& x1, double& y1, double& x2, double& y2,
                                      bool isLogX, bool isLogY)
{
    if (isLogX && isLogY) {
        double span = 3.0;
        x1 = centerX / span;
        x2 = centerX * span;
        y1 = centerY * pow(x1 / centerX, slope);
        y2 = centerY * pow(x2 / centerX, slope);
    }
    else if (!isLogX && !isLogY) {
        // 常规坐标系修复：使用缩放因子保持视觉斜率
        QCPAxisRect* rect = (m_chartMode == Mode_Stacked && m_topRect) ? m_topRect : m_plot->axisRect();
        double rangeX = rect->axis(QCPAxis::atBottom)->range().size();
        double rangeY = rect->axis(QCPAxis::atLeft)->range().size();

        double dx = rangeX * 0.15;
        double scaleFactor = rangeY / rangeX;
        double dy = slope * dx * scaleFactor;

        if (std::abs(dy) > rangeY * 0.5) {
            dy = rangeY * 0.2 * (slope >= 0 ? 1.0 : -1.0);
            if (std::abs(slope) > 1e-9) dx = dy / (slope * scaleFactor);
            else dx = rangeX * 0.2;
        }

        x1 = centerX - dx/2.0;
        x2 = centerX + dx/2.0;
        y1 = centerY - dy/2.0;
        y2 = centerY + dy/2.0;
    }
    else {
        QCPAxisRect* rect = m_plot->axisRect();
        x1 = rect->axis(QCPAxis::atBottom)->range().lower;
        x2 = rect->axis(QCPAxis::atBottom)->range().upper;
        y1 = centerY;
        y2 = centerY;
    }
}

// ---------------- 鼠标交互逻辑 ----------------

double ChartWidget::distToSegment(const QPointF& p, const QPointF& s, const QPointF& e)
{
    double l2 = (s.x()-e.x())*(s.x()-e.x()) + (s.y()-e.y())*(s.y()-e.y());
    if (l2 == 0) return std::sqrt((p.x()-s.x())*(p.x()-s.x()) + (p.y()-s.y())*(p.y()-s.y()));
    double t = ((p.x()-s.x())*(e.x()-s.x()) + (p.y()-s.y())*(e.y()-s.y())) / l2;
    t = std::max(0.0, std::min(1.0, t));
    QPointF proj = s + t * (e - s);
    return std::sqrt((p.x()-proj.x())*(p.x()-proj.x()) + (p.y()-proj.y())*(p.y()-proj.y()));
}

void ChartWidget::onPlotMousePress(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    m_interMode = Mode_None;
    m_activeLine = nullptr;
    m_activeText = nullptr;
    m_activeArrow = nullptr;
    m_lastMousePos = event->pos();

    double tolerance = 8.0;

    // 1. 优先检查文本标注 (QCPItemText)
    for (int i = 0; i < m_plot->itemCount(); ++i) {
        if (auto text = qobject_cast<QCPItemText*>(m_plot->item(i))) {
            if (text->selectTest(event->pos(), false) < tolerance) {
                m_interMode = Mode_Dragging_Text;
                m_activeText = text;
                m_plot->deselectAll();
                text->setSelected(true);
                m_plot->setInteractions(QCP::Interaction(0));
                m_plot->replot();
                return;
            }
        }
    }

    // 2. 检查箭头 (普通 QCPItemLine)
    for (int i = 0; i < m_plot->itemCount(); ++i) {
        auto line = qobject_cast<QCPItemLine*>(m_plot->item(i));
        if (line && !line->property("isCharacteristic").isValid()) {
            double x1 = m_plot->xAxis->coordToPixel(line->start->coords().x());
            double y1 = m_plot->yAxis->coordToPixel(line->start->coords().y());
            double x2 = m_plot->xAxis->coordToPixel(line->end->coords().x());
            double y2 = m_plot->yAxis->coordToPixel(line->end->coords().y());

            QPointF pMouse = event->pos();
            QPointF pStart(x1, y1);
            QPointF pEnd(x2, y2);

            if (std::sqrt(pow(pMouse.x()-pStart.x(),2) + pow(pMouse.y()-pStart.y(),2)) < tolerance) {
                m_interMode = Mode_Dragging_ArrowStart;
                m_activeArrow = line;
                m_plot->setInteractions(QCP::Interaction(0));
                return;
            }
            else if (std::sqrt(pow(pMouse.x()-pEnd.x(),2) + pow(pMouse.y()-pEnd.y(),2)) < tolerance) {
                m_interMode = Mode_Dragging_ArrowEnd;
                m_activeArrow = line;
                m_plot->setInteractions(QCP::Interaction(0));
                return;
            }
        }
    }

    // 3. 检查特征标识线
    for (int i = 0; i < m_plot->itemCount(); ++i) {
        QCPItemLine* line = qobject_cast<QCPItemLine*>(m_plot->item(i));
        if (!line || !line->property("isCharacteristic").isValid()) continue;

        double x1 = m_plot->xAxis->coordToPixel(line->start->coords().x());
        double y1 = m_plot->yAxis->coordToPixel(line->start->coords().y());
        double x2 = m_plot->xAxis->coordToPixel(line->end->coords().x());
        double y2 = m_plot->yAxis->coordToPixel(line->end->coords().y());

        QPointF pMouse = event->pos();
        QPointF pStart(x1, y1);
        QPointF pEnd(x2, y2);

        if (std::sqrt(pow(pMouse.x()-pStart.x(),2) + pow(pMouse.y()-pStart.y(),2)) < tolerance) {
            m_interMode = Mode_Dragging_Start;
            m_activeLine = line;
        } else if (std::sqrt(pow(pMouse.x()-pEnd.x(),2) + pow(pMouse.y()-pEnd.y(),2)) < tolerance) {
            m_interMode = Mode_Dragging_End;
            m_activeLine = line;
        } else if (distToSegment(pMouse, pStart, pEnd) < tolerance) {
            m_interMode = Mode_Dragging_Line;
            m_activeLine = line;
        }

        if (m_interMode != Mode_None) {
            m_plot->deselectAll();
            line->setSelected(true);
            m_plot->setInteractions(QCP::Interaction(0));
            m_plot->replot();
            return;
        }
    }

    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectItems);
    m_plot->deselectAll();
    m_plot->replot();
}

void ChartWidget::onPlotMouseMove(QMouseEvent* event)
{
    if (m_interMode != Mode_None && (event->buttons() & Qt::LeftButton)) {
        QPointF currentPos = event->pos();
        QPointF delta = currentPos - m_lastMousePos;
        double mouseX = m_plot->xAxis->pixelToCoord(currentPos.x());
        double mouseY = m_plot->yAxis->pixelToCoord(currentPos.y());

        // 1. 拖动文本
        if (m_interMode == Mode_Dragging_Text && m_activeText) {
            double tPx = m_plot->xAxis->coordToPixel(m_activeText->position->coords().x());
            double tPy = m_plot->yAxis->coordToPixel(m_activeText->position->coords().y());
            m_activeText->position->setCoords(
                m_plot->xAxis->pixelToCoord(tPx + delta.x()),
                m_plot->yAxis->pixelToCoord(tPy + delta.y())
                );
        }
        // 2. 拖动箭头端点
        else if (m_interMode == Mode_Dragging_ArrowStart && m_activeArrow) {
            if(m_activeArrow->start->parentAnchor()) m_activeArrow->start->setParentAnchor(nullptr);
            m_activeArrow->start->setCoords(mouseX, mouseY);
        }
        else if (m_interMode == Mode_Dragging_ArrowEnd && m_activeArrow) {
            if(m_activeArrow->end->parentAnchor()) m_activeArrow->end->setParentAnchor(nullptr);
            m_activeArrow->end->setCoords(mouseX, mouseY);
        }
        // 3. 拖动线段 (整体平移，带标注)
        else if (m_interMode == Mode_Dragging_Line && m_activeLine) {
            double sPx = m_plot->xAxis->coordToPixel(m_activeLine->start->coords().x());
            double sPy = m_plot->yAxis->coordToPixel(m_activeLine->start->coords().y());
            double ePx = m_plot->xAxis->coordToPixel(m_activeLine->end->coords().x());
            double ePy = m_plot->yAxis->coordToPixel(m_activeLine->end->coords().y());

            m_activeLine->start->setCoords(m_plot->xAxis->pixelToCoord(sPx + delta.x()), m_plot->yAxis->pixelToCoord(sPy + delta.y()));
            m_activeLine->end->setCoords(m_plot->xAxis->pixelToCoord(ePx + delta.x()), m_plot->yAxis->pixelToCoord(ePy + delta.y()));

            // 联动标注
            if (m_annotations.contains(m_activeLine)) {
                ChartAnnotation note = m_annotations[m_activeLine];
                if (note.textItem) {
                    double tPx = m_plot->xAxis->coordToPixel(note.textItem->position->coords().x());
                    double tPy = m_plot->yAxis->coordToPixel(note.textItem->position->coords().y());
                    note.textItem->position->setCoords(
                        m_plot->xAxis->pixelToCoord(tPx + delta.x()),
                        m_plot->yAxis->pixelToCoord(tPy + delta.y())
                        );
                }
                if (note.arrowItem) {
                    double aPx = m_plot->xAxis->coordToPixel(note.arrowItem->end->coords().x());
                    double aPy = m_plot->yAxis->coordToPixel(note.arrowItem->end->coords().y());
                    note.arrowItem->end->setCoords(
                        m_plot->xAxis->pixelToCoord(aPx + delta.x()),
                        m_plot->yAxis->pixelToCoord(aPy + delta.y())
                        );
                }
            }
        }
        // 4. 拉伸线段
        else if ((m_interMode == Mode_Dragging_Start || m_interMode == Mode_Dragging_End) && m_activeLine) {
            constrainLinePoint(m_activeLine, m_interMode == Mode_Dragging_Start, mouseX, mouseY);
        }

        m_lastMousePos = currentPos;
        m_plot->replot();
    }
}

void ChartWidget::onPlotMouseRelease(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_interMode = Mode_None;
    if (!m_activeLine && !m_activeText && !m_activeArrow)
        m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectItems);
}

void ChartWidget::onPlotMouseDoubleClick(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    double tolerance = 10.0;
    for (int i = 0; i < m_plot->itemCount(); ++i) {
        if (auto text = qobject_cast<QCPItemText*>(m_plot->item(i))) {
            if (text->selectTest(event->pos(), false) < tolerance) {
                onEditItemRequested(text);
                return;
            }
        }
    }
}

void ChartWidget::constrainLinePoint(QCPItemLine* line, bool isMovingStart, double mouseX, double mouseY)
{
    double k = line->property("fixedSlope").toDouble();
    bool isLogLog = line->property("isLogLog").toBool();
    double xFixed = isMovingStart ? line->end->coords().x() : line->start->coords().x();
    double yFixed = isMovingStart ? line->end->coords().y() : line->start->coords().y();
    double xNew = mouseX;
    double yNew;

    if (isLogLog) {
        if (xFixed <= 0) xFixed = 1e-5;
        if (xNew <= 0) xNew = 1e-5;
        yNew = yFixed * pow(xNew / xFixed, k);
    } else {
        QCPAxisRect* rect = (m_chartMode == Mode_Stacked && m_topRect) ? m_topRect : m_plot->axisRect();
        double rangeX = rect->axis(QCPAxis::atBottom)->range().size();
        double rangeY = rect->axis(QCPAxis::atLeft)->range().size();
        double scaleFactor = rangeY / rangeX;

        yNew = yFixed + (k * scaleFactor) * (xNew - xFixed);
    }

    if (isMovingStart) line->start->setCoords(xNew, yNew);
    else line->end->setCoords(xNew, yNew);
}

void ChartWidget::updateAnnotationArrow(QCPItemLine* line)
{
    if (m_annotations.contains(line)) {
        ChartAnnotation note = m_annotations[line];
        double midX = (line->start->coords().x() + line->end->coords().x()) / 2.0;
        double midY = (line->start->coords().y() + line->end->coords().y()) / 2.0;
        note.arrowItem->end->setCoords(midX, midY);
    }
}

// ---------------- 槽函数 (响应 MouseZoom 信号) ----------------

void ChartWidget::onAddAnnotationRequested(QCPItemLine* line)
{
    addAnnotationToLine(line);
}

void ChartWidget::onDeleteSelectedRequested()
{
    deleteSelectedItems();
}

void ChartWidget::onEditItemRequested(QCPAbstractItem* item)
{
    if (auto text = qobject_cast<QCPItemText*>(item)) {
        bool ok;
        QString newContent = QInputDialog::getText(this, "修改标注", "内容:", QLineEdit::Normal, text->text(), &ok);
        if (ok && !newContent.isEmpty()) {
            text->setText(newContent);
            m_plot->replot();
        }
    }
}

void ChartWidget::addAnnotationToLine(QCPItemLine* line)
{
    if (!line) return;

    if (m_annotations.contains(line)) {
        ChartAnnotation old = m_annotations.take(line);
        if(old.textItem) m_plot->removeItem(old.textItem);
        if(old.arrowItem) m_plot->removeItem(old.arrowItem);
    }

    double k = line->property("fixedSlope").toDouble();

    // 默认文本描述
    QString slopeDesc = QString("k=%1").arg(k);
    if (std::abs(k - 1.0) < 0.01) slopeDesc += " (井筒储集)";
    else if (std::abs(k - 0.5) < 0.01) slopeDesc += " (线性流)";
    else if (std::abs(k - 0.25) < 0.01) slopeDesc += " (双线性流)";
    else if (std::abs(k - 0.0) < 0.01) slopeDesc = "径向流";

    bool ok;
    QString text = QInputDialog::getText(this, "添加标注", "输入标注内容:", QLineEdit::Normal, slopeDesc, &ok);
    if (!ok || text.isEmpty()) return;

    QCPItemText* txt = new QCPItemText(m_plot);
    txt->setText(text);
    txt->position->setType(QCPItemPosition::ptPlotCoords);
    txt->setFont(QFont("Microsoft YaHei", 9));
    txt->setSelectable(true);

    QCPItemLine* arr = new QCPItemLine(m_plot);
    arr->setHead(QCPLineEnding::esSpikeArrow);
    arr->setSelectable(true); // 允许拖动

    double midX = (line->start->coords().x() + line->end->coords().x()) / 2.0;
    double midY = (line->start->coords().y() + line->end->coords().y()) / 2.0;

    bool isLogY = (m_plot->yAxis->scaleType() == QCPAxis::stLogarithmic);
    if (isLogY) txt->position->setCoords(midX, midY * 1.5);
    else txt->position->setCoords(midX, midY + (m_plot->yAxis->range().upper - m_plot->yAxis->range().lower)*0.05);

    arr->start->setParentAnchor(txt->bottom);
    arr->end->setCoords(midX, midY);

    ChartAnnotation note;
    note.textItem = txt;
    note.arrowItem = arr;
    m_annotations.insert(line, note);

    m_plot->replot();
}

void ChartWidget::deleteSelectedItems()
{
    auto items = m_plot->selectedItems();
    for (auto item : items) {
        if (auto line = qobject_cast<QCPItemLine*>(item)) {
            if (m_annotations.contains(line)) {
                ChartAnnotation note = m_annotations.take(line);
                if(note.textItem) m_plot->removeItem(note.textItem);
                if(note.arrowItem) m_plot->removeItem(note.arrowItem);
            }
        }
        if (auto txt = qobject_cast<QCPItemText*>(item)) {
            QMutableMapIterator<QCPItemLine*, ChartAnnotation> i(m_annotations);
            while (i.hasNext()) {
                i.next();
                if (i.value().textItem == txt) {
                    if(i.value().arrowItem) m_plot->removeItem(i.value().arrowItem);
                    i.remove();
                    break;
                }
            }
        }
        m_plot->removeItem(item);
    }
    m_plot->replot();
}
