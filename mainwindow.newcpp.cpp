#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QCloseEvent>
#include <QFile>
#include <QDir>
#include <QWidget>
#include <QWindow>
#include <QTabWidget>
#include <QFileDialog>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QtCharts>

#include <QSurfaceFormat>
#include <QWindow>
#include <QTimer>
#include <QProgressDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bool m_enableDownsampling = true;
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QProgressDialog progress("Loading data...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    progress.setWindowTitle("Processing Data");
    progress.setLabelText("Preparing to load data...");

    // Get the total number of lines in the file
    qint64 totalLines = 0;

    
    if (dataCsvPath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select a CSV file first.");
        return;
    }

    QFile inputFile(dataCsvPath);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not open the input file.");
        return;
    }


    QString outputFilePath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "../../../../output_dataset.csv");
    qDebug() << outputFilePath;
    QFile outputFile(outputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not create the output file.");
        return;
    }



    QTextStream in(&inputFile);
    QTextStream out(&outputFile);

    // Write header to output file
    out << "date,device,metric1,units\n";

    QString currentDevice;
    QString currentUnits;
    double minMetric1 = std::numeric_limits<double>::max();
    double maxMetric1 = std::numeric_limits<double>::lowest();
    bool skipNextLine = false;

    
    bool m_enableDownsampling = false;
    qint64 processedLines = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
          
        if (line.startsWith("Mnemonic:")) {
            currentDevice = line.split(":").at(1).trimmed();
            skipNextLine = true;
        } else if (line.startsWith("Units:")) {
            currentUnits = line.split(":").at(1).trimmed();
        } else if (line.startsWith("X Time,")) {
            // Skip this line
            continue;
        } else if (line.contains(",") && !skipNextLine) {
            QStringList parts = line.split(",");
            if (parts.size() >= 2) {
                QString date = parts[0].trimmed();
                QString metric1 = parts[1].trimmed();

                // Only write the line if metric1 is not zero
                if (metric1.toDouble() != 0.0) {
                    out << date << "," << currentDevice << "," << metric1 << "," << currentUnits << "\n";
                }
            }
        } else {
            skipNextLine = false;
        }
    }

    progress.setValue(100);
    progress.setLabelText("Data loaded successfully!");
    qDebug() << "Data Processed, file written";

    inputFile.close();
    outputFile.close();

    qDebug() << "Files closed";

    QMessageBox::information(this, "Success", "File has been processed and saved as output_dataset.csv");

    // After processing the CSV file, create the charts
    QChart *chart = new QChart();
    chart->setTitle("Metric1 vs Date for All Devices");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("yyyy-MM-dd");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("Metric [Volts]");
    axisY->setRange(minMetric1, 40);
    chart->addAxis(axisY, Qt::AlignLeft);

    chart->setTheme(QChart::ChartThemeDark);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    std::vector<QLineSeries*> seriesVector;
    std::vector<QString> deviceNames;
    std::vector<int> deviceDataCount;
    const int TARGET_POINTS = 1000;
    std::vector<QVector<QPointF>> devicePoints;
    // Reopen the output file to read the processed data
    QFile processedFile(outputFilePath);

    if (processedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {

        QTextStream in(&processedFile);
        while (!in.atEnd()) {
            in.readLine();
            totalLines++;
        }

        in.seek(0);  // Reset to the beginning of the file

        in.readLine(); // Skip header
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(",");
            if (parts.size() >= 4) {
                QString date = parts[0];
                QString device = parts[1];
                double metric1 = parts[2].toDouble();
                
                auto it = std::find(deviceNames.begin(), deviceNames.end(), device);
                size_t index;
                if (it == deviceNames.end()) {
                    index = seriesVector.size();
                    deviceNames.push_back(device);
                    seriesVector.push_back(new QLineSeries());
                    seriesVector.back()->setName(device);
                    devicePoints.push_back(QVector<QPointF>());
                } else {
                    index = std::distance(deviceNames.begin(), it);
                }

                QDateTime dateTime = QDateTime::fromString(date, "yyyy-MMM-dd-hh:mm:ss.zzz");
                devicePoints[index].append(QPointF(dateTime.toMSecsSinceEpoch(), metric1));
            }
        }
        processedFile.close();
    }

    // qDebug() << "Number of unique devices:" << seriesMap.size();
    // for (const auto& device : seriesMap.keys()) {
    //     qDebug() << "Device:" << device << "Data points:" << deviceDataCount[device];
    // }

    // Add all series to the chart
    // for (auto series : seriesMap.values()) {
    //     chart->addSeries(series);
    //     series->attachAxis(axisX);
    //     series->attachAxis(axisY);
    // }
    for (size_t i = 0; i < seriesVector.size(); ++i) {
        QVector<QPointF> pointsToUse;
        if (m_enableDownsampling) {
            pointsToUse = downsample(devicePoints[i], TARGET_POINTS);
        } else {
            pointsToUse = devicePoints[i];
        }
        seriesVector[i]->replace(pointsToUse);
        chart->addSeries(seriesVector[i]);
        seriesVector[i]->attachAxis(axisX);
        seriesVector[i]->attachAxis(axisY);
    }

    // Now set the y-axis range
    axisY->setRange(minMetric1, maxMetric1);

    // Optionally, add some padding to the range
    double range = maxMetric1 - minMetric1;
    qDebug() << "Max: ", maxMetric1;
    axisY->setRange(minMetric1 - range * 0.05, maxMetric1 + range * 0.05);

    // Create a chart view and set it as the central widget
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);


    chartView->setRubberBand(QChartView::RectangleRubberBand);
    chartView->setInteractive(true);

    QToolBar *toolBar = addToolBar(tr("Chart Actions"));

    QAction *saveAction = toolBar->addAction(tr("Save as Image"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveChartAsImage);

    QAction *zoomInAction = toolBar->addAction(tr("Zoom In"));
    connect(zoomInAction, &QAction::triggered, this, [this, chartView]() {
        chartView->chart()->zoomIn();
    });

    QAction *zoomOutAction = toolBar->addAction(tr("Zoom Out"));
    connect(zoomOutAction, &QAction::triggered, this, [this, chartView]() {
        chartView->chart()->zoomOut();
    });

    QAction *zoomResetAction = toolBar->addAction(tr("Reset Zoom"));
    connect(zoomResetAction, &QAction::triggered, this, [this, chartView]() {
        chartView->chart()->zoomReset();
    });

    // Create a QGraphicsProxyWidget to wrap the QChartView
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget();
    proxy->setWidget(chartView);

    // Set the chart view as the central widget of the graphics view
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->addItem(proxy);
    proxy->setGeometry(QRectF(0, 0, ui->graphicsView->width(), ui->graphicsView->height()));
}

void MainWindow::on_pushButton_2_clicked()
{

}


void MainWindow::on_dataPathButton_clicked()
{
    QString dataCsvPath = QFileDialog::getOpenFileName(this,
        tr("Open CSV File"), "", tr("CSV Files (*.csv)"));

    if (!dataCsvPath.isEmpty()) {
        // Save the path to a member variable if you want to use it later
        this->dataCsvPath = dataCsvPath;

        // Optionally, you can display the selected path in a QLineEdit or QLabel
        ui->dataPathEdit->setText(dataCsvPath);

        qDebug() << "Selected CSV file:" << dataCsvPath;
    }
}

void MainWindow::saveChartAsImage()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Chart As Image"), "", tr("Images (*.png *.jpg *.bmp)"));
    if (!fileName.isEmpty()) {
        QPixmap pixmap = chartView->grab();
        pixmap.save(fileName);
    }
}

QVector<QPointF> MainWindow::downsample(const QVector<QPointF>& points, int targetPoints) {
    if (points.size() <= targetPoints) return points;

    QVector<QPointF> result;
    result.reserve(targetPoints);

    // Always add the first point
    result.append(points.first());

    double every = (points.size() - 2) / (targetPoints - 2.0);
    int a = 0;
    for (int i = 0; i < targetPoints - 2; ++i) {
        int nextA = qMin(static_cast<int>((i + 1) * every) + 1, points.size() - 1);

        double maxArea = -1;
        int maxAreaIndex = a;

        for (int j = a + 1; j < nextA; ++j) {
            double area = qAbs((points[a].x() - points[nextA].x()) * (points[j].y() - points[a].y()) -
                               (points[a].x() - points[j].x()) * (points[nextA].y() - points[a].y())) * 0.5;

            if (area > maxArea) {
                maxArea = area;
                maxAreaIndex = j;
            }
        }

        result.append(points[maxAreaIndex]);
        a = nextA;
    }

    // Always add the last point
    result.append(points.last());

    return result;
}

