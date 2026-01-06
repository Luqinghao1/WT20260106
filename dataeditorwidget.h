/*
 * 文件名: dataeditorwidget.h
 * 文件作用: 数据编辑器主窗口头文件
 * 功能描述:
 * 1. 定义数据编辑器的主界面类 DataEditorWidget。
 * 2. 声明表格数据模型、代理模型和撤销栈。
 * 3. 声明文件加载、保存、导出、错误检查及列操作功能。
 * 4. 新增隐藏/显示行列功能声明。
 */

#ifndef DATAEDITORWIDGET_H
#define DATAEDITORWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QUndoStack>
#include <QMenu>
#include <QJsonArray>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QDialog>
#include "dataimportdialog.h"

// 定义列的枚举类型
enum class WellTestColumnType {
    SerialNumber, Date, Time, TimeOfDay, Pressure, CasingPressure, BottomHolePressure,
    Temperature, FlowRate, Depth, Viscosity, Density, Permeability, Porosity, WellRadius,
    SkinFactor, Distance, Volume, PressureDrop, Custom
};

// 定义列属性结构体
struct ColumnDefinition {
    QString name;
    WellTestColumnType type;
    QString unit;
    bool isRequired;
    int decimalPlaces;

    ColumnDefinition() : type(WellTestColumnType::Custom), isRequired(false), decimalPlaces(3) {}
};

namespace Ui {
class DataEditorWidget;
}

// 内部类前置声明
class InternalSplitDialog;

class NoContextMenuDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NoContextMenuDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
};

class DataEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataEditorWidget(QWidget *parent = nullptr);
    ~DataEditorWidget();

    void clearAllData();
    void loadFromProjectData();
    QStandardItemModel* getDataModel() const;
    void loadData(const QString& filePath, const QString& fileType = "auto");
    QString getCurrentFileName() const;
    bool hasData() const;
    QList<ColumnDefinition> getColumnDefinitions() const { return m_columnDefinitions; }

signals:
    void dataChanged();
    void fileChanged(const QString& filePath, const QString& fileType);

private slots:
    // 文件操作
    void onOpenFile();
    void onSave();
    void onExportExcel();

    // 数据处理
    void onDefineColumns();
    void onTimeConvert();
    void onPressureDropCalc();
    void onCalcPwf();
    void onHighlightErrors();

    // 搜索
    void onSearchTextChanged();

    // 右键菜单
    void onCustomContextMenu(const QPoint& pos);
    void onMergeCells();
    void onUnmergeCells();
    void onSortAscending();
    void onSortDescending();
    void onSplitColumn();

    // 行操作 (新增隐藏/显示)
    void onAddRow(int insertMode = 0);
    void onDeleteRow();
    void onHideRow();
    void onShowAllRows();

    // 列操作 (新增隐藏/显示)
    void onAddCol(int insertMode = 0);
    void onDeleteCol();
    void onHideCol();
    void onShowAllCols();

    void onModelDataChanged();

private:
    Ui::DataEditorWidget *ui;

    QStandardItemModel* m_dataModel;
    QSortFilterProxyModel* m_proxyModel;
    QUndoStack* m_undoStack;

    QList<ColumnDefinition> m_columnDefinitions;
    QString m_currentFilePath;
    QMenu* m_contextMenu;
    QTimer* m_searchTimer;

    void initUI();
    void setupConnections();
    void setupModel();
    void updateButtonsState();

    bool loadFileInternal(const QString& path);
    bool loadFileWithConfig(const DataImportSettings& settings);

    QJsonArray serializeModelToJson() const;
    void deserializeJsonToModel(const QJsonArray& array);
};

#endif // DATAEDITORWIDGET_H
