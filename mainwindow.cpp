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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , process(new QProcess(this))
{
    ui->setupUi(this);

    connect(process, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessReadyRead);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::onProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &MainWindow::onProcessErrorOccurred);
    connect(process, &QProcess::stateChanged, this, &MainWindow::onProcessStateChanged);
}

MainWindow::~MainWindow()
{
    if (process->state() == QProcess::Running) {
        disconnect(process, nullptr, this, nullptr);
        process->terminate();
        if (!process->waitForFinished(3000)) {
            process->kill();
        }
    }

    delete process;
    delete ui;
}

// void MainWindow::on_pushButton_clicked()
// {
//     ui->textEdit->append("[" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] " + "The model has started, please wait...");
//     qDebug() << "onPushButtonClicked called";
//     if (process->state() == QProcess::Running) {
//         QMessageBox::warning(this, "Warning", "The process is already running.");
//         return;
//     }

//     QString executablePath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "../../../../defaultCode");
//     qDebug() << executablePath;

//     QFile file(executablePath);
//     if (!file.exists()) {
//         QMessageBox::warning(this, "File Not Found", "The specified executable file does not exist.");
//         return;
//     }

//     ui->pushButton->setEnabled(false);

//     process->setProcessChannelMode(QProcess::MergedChannels);
//     process->start(executablePath);

//     if (!process->waitForStarted()) {
//         QMessageBox::critical(this, "Error", "Failed to start the model.");
//         ui->pushButton->setEnabled(true);
//     } else {
//         QWidget* newTab = new QWidget(ui->tabWidget); // Create a new QWidget
//         ui->tabWidget->addTab(newTab, "Graphs");

//         QString windowId = "PYTHON_APP_WINDOW_ID";

//         // Embed the Python app window into the tab
//         QWidget* embeddedWindow = QWidget::createWindowContainer(QWindow::fromWinId(windowId.toULong()), newTab);
//         embeddedWindow->setParent(newTab);
//     }
// }

void MainWindow::on_pushButton_clicked()
{
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

    inputFile.close();
    outputFile.close();

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

    QMap<QString, QLineSeries*> seriesMap;
    QMap<QString, int> deviceDataCount;

    // Reopen the output file to read the processed data
    QFile processedFile(outputFilePath);
    if (processedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&processedFile);
        in.readLine(); // Skip header

        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(",");
            if (parts.size() >= 4) {
                QString date = parts[0];
                QString device = parts[1];
                double metric1 = parts[2].toDouble();
                minMetric1 = qMin(minMetric1, metric1);
                maxMetric1 = qMax(maxMetric1, metric1);

                if (!seriesMap.contains(device)) {
                    seriesMap[device] = new QLineSeries();
                    seriesMap[device]->setName(device);
                    deviceDataCount[device] = 0;
                }

                QDateTime dateTime = QDateTime::fromString(date, "yyyy-MMM-dd-hh:mm:ss.zzz");
                seriesMap[device]->append(dateTime.toMSecsSinceEpoch(), metric1);
                deviceDataCount[device]++;
            }
        }
        processedFile.close();
    }

    qDebug() << "Number of unique devices:" << seriesMap.size();
    for (const auto& device : seriesMap.keys()) {
        qDebug() << "Device:" << device << "Data points:" << deviceDataCount[device];
    }

    // Add all series to the chart
    for (auto series : seriesMap.values()) {
        chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }

    // Now set the y-axis range
    axisY->setRange(minMetric1, maxMetric1);

    // Optionally, add some padding to the range
    double range = maxMetric1 - minMetric1;
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

void MainWindow::onProcessReadyRead()
{
    qDebug() << "Process output available";
    if (process->state() != QProcess::Running) {
        qDebug() << "Process is not running, ignoring output";
        return;
    }
    QByteArray output = process->readAllStandardOutput();
    qDebug() << "Output received: " << output;
    ui->textEdit->append("[" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] " + QString::fromLocal8Bit(output));
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Process finished with exit code:" << exitCode << "and exit status:" << exitStatus;
    ui->pushButton->setEnabled(true);
}

void MainWindow::onProcessErrorOccurred(QProcess::ProcessError error)
{
    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = "The process failed to start.";
        break;
    case QProcess::Crashed:
        errorMessage = "The process crashed.";
        break;
    case QProcess::Timedout:
        errorMessage = "The process timed out.";
        break;
    case QProcess::ReadError:
        errorMessage = "An error occurred while reading the process output.";
        break;
    case QProcess::WriteError:
        errorMessage = "An error occurred while writing to the process.";
        break;
    case QProcess::UnknownError:
    default:
        errorMessage = "An unknown error occurred.";
        break;
    }

    QMessageBox::critical(this, "Process Error", errorMessage);
    ui->pushButton->setEnabled(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (process->state() == QProcess::Running) {
        disconnect(process, nullptr, this, nullptr);
        process->terminate();
        if (!process->waitForFinished(3000)) {
            process->kill();
        }
    }

    event->accept();
}

void MainWindow::onProcessStateChanged(QProcess::ProcessState newState)
{
    switch (newState) {

    case QProcess::NotRunning:
        ui->textEdit->append("[" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] " + "DEBUG: Process is not running.");
        ui->pushButton->setEnabled(true);
        break;
    case QProcess::Starting:
        ui->textEdit->append("[" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] " + "DEBUG: Process is starting...");
        break;
    case QProcess::Running:
        ui->textEdit->append("[" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "] " + "DEBUG: Process is running.");
        break;
    }
    
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

