#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QtCharts>
#include <QChartView>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // void closeEvent(QCloseEvent *event) override;

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    void on_dataPathButton_clicked();
    void saveChartAsImage();

private:
    Ui::MainWindow *ui;
    QProcess *process;

    QString dataCsvPath;
    QChartView *chartView;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QChart *chart;
    QMap<QString, QLineSeries*> seriesMap;
    QMap<QString, int> deviceDataCount;

    double minMetric1 = std::numeric_limits<double>::max();
    double maxMetric1 = std::numeric_limits<double>::lowest();

    QScrollBar *horizontalScrollBar;
    QDateTime startDate, endDate;
    int dataChunkSize = 1000;
    void onScrollValueChanged(int value);
    static const int MAX_POINTS_PER_DEVICE = 1000;
    QVector<QPointF> downsample(const QVector<QPointF>& points, int targetPoints);
    // void loadDataForRange(const QDateTime &start, const QDateTime &end);

    void toggleDownsampling(bool enable);
};

#endif // MAINWINDOW_H
