#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>       // For running commands
#include <QMessageBox>    // For error dialogs
#include <QRegularExpression> // For input validation
#include "ui_mainwindow.h"
namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Auto-connected slots (name must match UI elements)
    
    void on_btnApply_clicked();      // Handles "Apply" button
    void on_btnActivate_clicked();   // Handles "Activate" button
    void on_btnDeactivate_clicked(); // Handles "Deactivate" button

    // Manual slots
    void loadNetworkInterfaces();    // Loads interfaces into comboboxes
    void refreshInterfaceInfo();     // Updates the QTextBrowser with `ip a`

private:
    QString subnetToCidr(const QString &subnet);
    Ui::MainWindow *ui;  // Links to your UI elements (from ui_mainwindow.h)
    QProcess m_process;  // For executing commands
};
#endif











/*
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
*/