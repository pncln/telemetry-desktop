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
};

#endif // MAINWINDOW_H
