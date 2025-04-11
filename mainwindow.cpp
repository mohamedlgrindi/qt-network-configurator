
/*
#include <QFont>
#include <QVBoxLayout>
#include <QProcess>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThread>
#include <QMap>
#include <QTime>  // For time measurement
#include <QElapsedTimer>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ===== UI STYLING =====
    this->resize(1200, 800);

    // Center all content
    QWidget* centralContainer = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(centralContainer);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(ui->Network_Interfaces);
    mainLayout->addWidget(ui->groupBox);
    mainLayout->addWidget(ui->groupBox_2);
    mainLayout->setContentsMargins(50, 30, 50, 30);
    this->setCentralWidget(centralContainer);

    // Font styling
    QFont bigFont;
    bigFont.setPointSize(14);
    ui->label_2->setFont(bigFont);
    ui->label_3->setFont(bigFont);
    ui->label->setFont(bigFont);
    ui->label_4->setFont(bigFont);
    ui->editIP->setFont(bigFont);
    ui->editSubnet->setFont(bigFont);
    ui->comboInterface->setFont(bigFont);
    ui->comboActivateDeactivate->setFont(bigFont);

    // Button styling
    QFont buttonFont;
    buttonFont.setPointSize(12);
    ui->btnApply->setMinimumSize(200, 50);
    ui->btnActivate->setMinimumSize(200, 50);
    ui->btnDeactivate->setMinimumSize(200, 50);
    ui->btnApply->setFont(buttonFont);
    ui->btnActivate->setFont(buttonFont);
    ui->btnDeactivate->setFont(buttonFont);

    // Group box styling
    QFont groupBoxFont;
    groupBoxFont.setPointSize(16);
    groupBoxFont.setBold(true);
    ui->Network_Interfaces->setFont(groupBoxFont);
    ui->groupBox->setFont(groupBoxFont);
    ui->groupBox_2->setFont(groupBoxFont);

    // Spacing
    ui->verticalLayout->setSpacing(15);
    ui->horizontalLayout_3->setSpacing(20);

    // Set input hints
    ui->editSubnet->setPlaceholderText("e.g., 255.255.240.0 or 20");
    ui->editSubnet->setToolTip("Enter subnet as:\n- CIDR (e.g., '20')\n- Dotted-decimal (e.g., '255.255.240.0')");

    // ===== INITIALIZE NETWORK =====
    loadNetworkInterfaces();
    refreshInterfaceInfo();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::subnetToCidr(const QString &subnet) {
    // Handle CIDR notation directly (e.g., "20")
    bool ok;
    int cidr = subnet.toInt(&ok);
    if (ok && cidr >= 0 && cidr <= 32) {
        return QString::number(cidr);
    }

    // Handle dotted-decimal notation
    QMap<QString, QString> subnetMap = {
        {"255.255.255.0", "24"},
        {"255.255.0.0", "16"},
        {"255.0.0.0", "8"},
        {"255.255.240.0", "20"},
        {"255.255.252.0", "22"}
    };

    if (subnetMap.contains(subnet)) {
        return subnetMap[subnet];
    }

    return ""; // Invalid
}

void MainWindow::loadNetworkInterfaces()
{
    ui->comboInterface->clear();
    ui->comboActivateDeactivate->clear();

    QProcess process;
    process.start("ip", QStringList() << "-br" << "link" << "show");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList interfaces = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : interfaces) {
        QString interface = line.split(' ').first();
        if (interface != "lo") {
            ui->comboInterface->addItem(interface);
            ui->comboActivateDeactivate->addItem(interface);
        }
    }
}

void MainWindow::refreshInterfaceInfo()
{
    QProcess process;
    process.start("ip", QStringList() << "a");
    process.waitForFinished();
    ui->textInterfaces->setText(process.readAllStandardOutput());
}

void MainWindow::on_btnApply_clicked()
{
    QString ip = ui->editIP->text().trimmed();
    QString subnet = ui->editSubnet->text().trimmed();
    QString interface = ui->comboInterface->currentText();

    // Validate IP format
    if (!QRegularExpression("^\\d{1,3}(\\.\\d{1,3}){3}$").match(ip).hasMatch()) {
        QMessageBox::warning(this, "Error", "Invalid IP address format");
        return;
    }

    // Enhanced subnet validation
    QString cidr = subnetToCidr(subnet);
    if (cidr.isEmpty()) {
        QMessageBox::warning(this, "Error", 
            "Unsupported subnet mask.\n"
            "Accepted formats:\n"
            "- CIDR (e.g., '20')\n"
            "- Dotted-decimal (e.g., '255.255.240.0')");
        return;
    }

    // Enhanced authentication notification
    QMessageBox authNotice(this);
    authNotice.setIcon(QMessageBox::Information);
    authNotice.setWindowTitle("Authentication Required");
    authNotice.setText("Network changes require administrator privileges.");
    authNotice.setInformativeText(
        "A system authentication dialog will appear shortly.\n\n"
        "If you don't see it:\n"
        "1. Check for minimized windows\n"
        "2. Look in your taskbar/notification area\n"
        "3. The dialog might be behind this window");
    authNotice.addButton("I Understand", QMessageBox::AcceptRole);
    authNotice.addButton("Cancel", QMessageBox::RejectRole);
    
    if (authNotice.exec() != QMessageBox::AcceptRole) {
        return; // User canceled
    }

    // Create complete command sequence
    QString command = QString(
        "ip addr flush dev %1 && "
        "ip addr add %2/%3 dev %1 && "
        "ip link set %1 up && "
        "(systemctl restart NetworkManager 2>/dev/null || "
        "service networking restart 2>/dev/null || true) && "
        "ip addr show %1"
    ).arg(interface, ip, cidr);

    // Execute with pkexec and track time
    QTime executionTime;
    executionTime.start();
    m_process.start("pkexec", QStringList() << "bash" << "-c" << command);
    
    // Handle timeout with user feedback
    while (!m_process.waitForFinished(500)) { // Check every 500ms
        if (executionTime.elapsed() > 15000) { // 15 second timeout
            if (QMessageBox::question(this, "Still Waiting",
                "The operation is taking longer than expected.\n"
                "Possible reasons:\n"
                "1. Password dialog is hidden\n"
                "2. Authentication service is slow\n\n"
                "Keep waiting?",
                QMessageBox::Yes|QMessageBox::No) == QMessageBox::No) {
                
                m_process.kill();
                QMessageBox::warning(this, "Canceled", "Operation was canceled");
                return;
            }
        }
        qApp->processEvents(); // Keep UI responsive
    }

    // Add brief delay for changes to stabilize
    QThread::msleep(500);
    refreshInterfaceInfo();

    // Enhanced verification with detailed diagnostics
    QString output = ui->textInterfaces->toPlainText();
    QString successMessage = QString(
        "Successfully configured %1\n"
        "New IP: %2/%3\n"
        "Interface state: UP").arg(interface, ip, cidr);
    
    QString errorMessage;
    bool success = true;

    if (!output.contains(ip)) {
        errorMessage += "✖ New IP not found in configuration\n";
        success = false;
    }
    if (!output.contains("state UP")) {
        errorMessage += "✖ Interface is not in UP state\n";
        success = false;
    }
    if (m_process.exitCode() != 0) {
        errorMessage += QString("✖ Command failed (exit code: %1)\n").arg(m_process.exitCode());
        success = false;
    }

    if (success) {
        QMessageBox::information(this, "Success", successMessage);
    } else {
        errorMessage += "\nTechnical Details:\n";
        errorMessage += m_process.readAllStandardError();
        if (errorMessage.isEmpty()) {
            errorMessage += "No error output from commands";
        }
        
        QMessageBox errorDialog(this);
        errorDialog.setIcon(QMessageBox::Critical);
        errorDialog.setWindowTitle("Configuration Failed");
        errorDialog.setText("Failed to apply network settings");
        errorDialog.setDetailedText(errorMessage);
        errorDialog.setStandardButtons(QMessageBox::Ok);
        errorDialog.exec();
    }
}

void MainWindow::on_btnDeactivate_clicked()
{
    QString interface = ui->comboActivateDeactivate->currentText();
    if (interface.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select an interface");
        return;
    }

    m_process.start("pkexec", QStringList() << "ip" << "link" << "set" << interface << "down");
    m_process.waitForFinished();

    if (m_process.exitCode() == 0) {
        refreshInterfaceInfo();
        QMessageBox::information(this, "Success", QString("Interface %1 deactivated").arg(interface));
    } else {
        QMessageBox::critical(this, "Error", "Failed to deactivate interface");
    }
}

*/








#include <QFont>
#include <QVBoxLayout>
#include <QProcess>
#include <QMessageBox>
#include <QRegularExpression>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ===== UI STYLING =====
    this->resize(1200, 800); // Larger window size

    // Center all content
    QWidget* centralContainer = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(centralContainer);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(ui->Network_Interfaces);
    mainLayout->addWidget(ui->groupBox);
    mainLayout->addWidget(ui->groupBox_2);
    mainLayout->setContentsMargins(50, 30, 50, 30);
    this->setCentralWidget(centralContainer);

    // Font styling
    QFont bigFont;
    bigFont.setPointSize(14);
    ui->label_2->setFont(bigFont);
    ui->label_3->setFont(bigFont);
    ui->label->setFont(bigFont);
    ui->label_4->setFont(bigFont);
    ui->editIP->setFont(bigFont);
    ui->editSubnet->setFont(bigFont);
    ui->comboInterface->setFont(bigFont);
    ui->comboActivateDeactivate->setFont(bigFont);

    // Button styling
    QFont buttonFont;
    buttonFont.setPointSize(12);
    ui->btnApply->setMinimumSize(200, 50);
    ui->btnActivate->setMinimumSize(200, 50);
    ui->btnDeactivate->setMinimumSize(200, 50);
    ui->btnApply->setFont(buttonFont);
    ui->btnActivate->setFont(buttonFont);
    ui->btnDeactivate->setFont(buttonFont);

    // Group box styling
    QFont groupBoxFont;
    groupBoxFont.setPointSize(16);
    groupBoxFont.setBold(true);
    ui->Network_Interfaces->setFont(groupBoxFont);
    ui->groupBox->setFont(groupBoxFont);
    ui->groupBox_2->setFont(groupBoxFont);

    // Spacing
    ui->verticalLayout->setSpacing(15);
    ui->horizontalLayout_3->setSpacing(20);

    // ===== FUNCTIONALITY INITIALIZATION =====
    loadNetworkInterfaces();
    refreshInterfaceInfo();

    // In MainWindow constructor, after all other UI setup:
    ui->editSubnet->setPlaceholderText("e.g., 255.255.240.0 or 20");
    ui->editSubnet->setToolTip("Enter subnet as:\n- CIDR (e.g., '20')\n- Dotted-decimal (e.g., '255.255.240.0')");
    connect(ui->btnApply, &QPushButton::clicked, this, &MainWindow::on_btnApply_clicked);
}
/*
QString MainWindow::subnetToCidr(const QString &subnet) {
    // Handle CIDR notation directly (e.g., "20")
    bool ok;
    int cidr = subnet.toInt(&ok);
    if (ok && cidr >= 0 && cidr <= 32) {
        return QString::number(cidr);
    }

    // Handle dotted-decimal notation
    QMap<QString, QString> subnetMap = {
        {"255.255.255.0", "24"},
        {"255.255.0.0", "16"},
        {"255.0.0.0", "8"},
        {"255.255.240.0", "20"},  // For /20 networks
        {"255.255.252.0", "22"}   // Additional common masks
    };

    if (subnetMap.contains(subnet)) {
        return subnetMap[subnet];
    }

    return ""; // Invalid
}
*/
QString MainWindow::subnetToCidr(const QString &subnet) {
    // Try parsing as CIDR first (e.g., "24")
    bool ok;
    int cidr = subnet.toInt(&ok);
    if (ok && cidr >= 0 && cidr <= 32) {
        return QString::number(cidr);
    }

    // Parse as dotted-decimal (e.g., "255.255.255.0")
    QRegularExpression ipRegex("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$");
    QRegularExpressionMatch match = ipRegex.match(subnet);
    if (match.hasMatch()) {
        quint32 mask = 0;
        for (int i = 1; i <= 4; ++i) {
            quint8 octet = match.captured(i).toUInt();
            mask = (mask << 8) | octet;
        }

        // Count the number of leading 1s in the binary mask
        int cidr = 0;
        while (mask & 0x80000000) {
            cidr++;
            mask <<= 1;
        }

        if (mask == 0 && cidr <= 32) {
            return QString::number(cidr);
        }
    }

    return ""; // Invalid
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadNetworkInterfaces()
{
    ui->comboInterface->clear();
    ui->comboActivateDeactivate->clear();

    QProcess process;
    process.start("ip", QStringList() << "-br" << "link" << "show");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList interfaces = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : interfaces) {
        QString interface = line.split(' ').first();
        if (interface != "lo") {  // Skip loopback interface
            ui->comboInterface->addItem(interface);
            ui->comboActivateDeactivate->addItem(interface);
        }
    }
}

void MainWindow::refreshInterfaceInfo()
{
    QProcess process;
    process.start("ip", QStringList() << "a");
    process.waitForFinished();
    ui->textInterfaces->setText(process.readAllStandardOutput());
}
/*
void MainWindow::on_btnApply_clicked() {
    QString ip = ui->editIP->text().trimmed();
    QString subnet = ui->editSubnet->text().trimmed();
    QString interface = ui->comboInterface->currentText();

    // Validate IP format
    if (!QRegularExpression("^\\d{1,3}(\\.\\d{1,3}){3}$").match(ip).hasMatch()) {
        QMessageBox::warning(this, "Error", "Invalid IP address format");
        return;
    }

    // Enhanced subnet validation
    QString cidr = subnetToCidr(subnet);
    if (cidr.isEmpty()) {
        QMessageBox::warning(this, "Error", 
            "Unsupported subnet mask.\n"
            "Accepted formats:\n"
            "- CIDR (e.g., '20')\n"
            "- Dotted-decimal (e.g., '255.255.240.0')");
        return;
    }

    // Execute command
    QString cmd = QString("ip addr add %1/%2 dev %3").arg(ip, cidr, interface);
    m_process.start("pkexec", QStringList() << "bash" << "-c" << cmd);
    m_process.waitForFinished();

    if (m_process.exitCode() == 0) {
        QMessageBox::information(this, "Success", "IP address updated");
        refreshInterfaceInfo();
    } else {
        QMessageBox::critical(this, "Error", "Failed to update IP:\n" + m_process.readAllStandardError());
    }
}
*/
/*
void MainWindow::on_btnApply_clicked() {
    QString ip = ui->editIP->text().trimmed();
    QString subnet = ui->editSubnet->text().trimmed();
    QString interface = ui->comboInterface->currentText();

    // Validate IP format
    if (!QRegularExpression("^\\d{1,3}(\\.\\d{1,3}){3}$").match(ip).hasMatch()) {
        QMessageBox::warning(this, "Error", "Invalid IP address format");
        return;
    }

    // Enhanced subnet validation
    QString cidr = subnetToCidr(subnet);
    if (cidr.isEmpty()) {
        QMessageBox::warning(this, "Error", 
            "Unsupported subnet mask.\n"
            "Accepted formats:\n"
            "- CIDR (e.g., '20')\n"
            "- Dotted-decimal (e.g., '255.255.240.0')");
        return;
    }

    // Notify user about authentication
    QMessageBox::information(this, "Authentication Required",
        "You will need to enter your password to change network settings.");

    // Create complete command sequence
    QString command = QString(
        "ip addr flush dev %1 && "      // First remove all IPs
        "ip addr add %2/%3 dev %1 && "  // Then add new IP
        "ip link set %1 up"             // Ensure interface is up
    ).arg(interface, ip, cidr);

    // Execute with pkexec
    m_process.start("pkexec", QStringList() << "bash" << "-c" << command);
    m_process.waitForFinished();

    // Verify the change
    refreshInterfaceInfo();
    
    if (ui->textInterfaces->toPlainText().contains(ip)) {
        QMessageBox::information(this, "Success", 
            QString("Successfully changed %1 to %2/%3")
            .arg(interface, ip, cidr));
    } else {
        QString errorOutput = QString::fromUtf8(m_process.readAllStandardError());
        QMessageBox::critical(this, "Error", 
            QString("Failed to verify IP change. Error:\n%1")
            .arg(errorOutput.isEmpty() ? "Unknown error" : errorOutput));
    }
}
*/

void MainWindow::on_btnApply_clicked() {
    QString interface = ui->comboInterface->currentText();
    QString ip = ui->editIP->text().trimmed();
    QString subnet = ui->editSubnet->text().trimmed();

    // Validate inputs
    if (interface.isEmpty() || ip.isEmpty() || subnet.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select an interface and enter both IP and subnet.");
        return;
    }

    // Convert subnet to CIDR
    QString cidr = subnetToCidr(subnet);
    if (cidr.isEmpty()) {
        QMessageBox::warning(this, "Error", "Invalid subnet format. Use CIDR (e.g., '24') or dotted-decimal (e.g., '255.255.255.0').");
        return;
    }

    // Construct and run the command (modern way)
// Construct the sudo command
     QString program = "sudo";
     QStringList arguments;
     arguments << "ip" << "addr" << "add" << QString("%1/%2").arg(ip).arg(cidr) << "dev" << interface;

     QProcess process;
     process.start(program, arguments);  // Runs with sudo
     process.waitForFinished();

    if (process.exitCode() == 0) {
        QMessageBox::information(this, "Success", "IP address applied successfully!");
        refreshInterfaceInfo(); // Update display
    } else {
        QString error = process.readAllStandardError();
        QMessageBox::critical(this, "Error", "Failed to apply IP:\n" + error);
    }
}

void MainWindow::on_btnActivate_clicked() {
    QString interface = ui->comboActivateDeactivate->currentText();
    
    if (interface.isEmpty()) {
        QMessageBox::warning(this, "Error", "No interface selected!");
        return;
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels); // Combine stdout/stderr
    process.start("sudo", QStringList() << "ip" << "link" << "set" << interface << "up");
    process.waitForFinished();

    if (process.exitCode() == 0) {
        QMessageBox::information(this, "Success", QString("Interface %1 activated!").arg(interface));
        refreshInterfaceInfo(); // Update UI
    } else {
        QString error = process.readAllStandardError();
        QMessageBox::critical(this, "Error", 
            QString("Failed to activate %1 (needs root?):\n%2").arg(interface, error));
    }
}

void MainWindow::on_btnDeactivate_clicked() {
    QString interface = ui->comboActivateDeactivate->currentText();
    
    if (interface.isEmpty()) {
        QMessageBox::warning(this, "Error", "No interface selected!");
        return;
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("sudo", QStringList() << "ip" << "link" << "set" << interface << "down");
    process.waitForFinished();

    if (process.exitCode() == 0) {
        QMessageBox::information(this, "Success", QString("Interface %1 deactivated!").arg(interface));
        refreshInterfaceInfo(); // Update UI
    } else {
        QString error = process.readAllStandardError();
        QMessageBox::critical(this, "Error", 
            QString("Failed to deactivate %1 (needs root?):\n%2").arg(interface, error));
    }
}










/*
#include <QFont>       
#include <QVBoxLayout>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

      // ===== WINDOW SIZE =====
      this->resize(1200, 800); // Larger window size

      // ===== CENTER ALL CONTENT =====
      QWidget* centralContainer = new QWidget();
      QVBoxLayout* mainLayout = new QVBoxLayout(centralContainer);
      mainLayout->setAlignment(Qt::AlignCenter); // Center alignment
      
      // Add existing UI elements to the centered layout
      mainLayout->addWidget(ui->Network_Interfaces);
      mainLayout->addWidget(ui->groupBox);
      mainLayout->addWidget(ui->groupBox_2);
      
      // Set margins around the content (left, top, right, bottom)
      mainLayout->setContentsMargins(50, 30, 50, 30);
      this->setCentralWidget(centralContainer);
  
      // ===== FONT SIZES =====
      QFont bigFont;
      bigFont.setPointSize(14); // Base font size
      
      // Labels
      ui->label_2->setFont(bigFont);    // "IP Address:"
      ui->label_3->setFont(bigFont);    // "Subnet Mask:"
      ui->label->setFont(bigFont);      // "Interface Name:"
      ui->label_4->setFont(bigFont);    // Activate/Deactivate label
      
      // Input fields
      ui->editIP->setFont(bigFont);
      ui->editSubnet->setFont(bigFont);
      ui->comboInterface->setFont(bigFont);
      ui->comboActivateDeactivate->setFont(bigFont);
  
      // ===== BUTTON STYLING =====
      QFont buttonFont;
      buttonFont.setPointSize(12);
      
      // Set minimum button sizes (width, height)
      ui->btnApply->setMinimumSize(200, 50);
      ui->btnActivate->setMinimumSize(200, 50);
      ui->btnDeactivate->setMinimumSize(200, 50);
      
      // Apply fonts to buttons
      ui->btnApply->setFont(buttonFont);
      ui->btnActivate->setFont(buttonFont);
      ui->btnDeactivate->setFont(buttonFont);
  
      // ===== GROUP BOX STYLING =====
      QFont groupBoxFont;
      groupBoxFont.setPointSize(16);
      groupBoxFont.setBold(true);
      
      ui->Network_Interfaces->setFont(groupBoxFont);
      ui->groupBox->setFont(groupBoxFont);
      ui->groupBox_2->setFont(groupBoxFont);
  
      // ===== INTERNAL SPACING =====
      ui->verticalLayout->setSpacing(15);    // Spacing in Modify IP section
      ui->horizontalLayout_3->setSpacing(20);
}

MainWindow::~MainWindow()
{
    delete ui;
}
*/