#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void startService();
    void stopService();
    void changeService(const char * method);
    void exit_program();
    void statusService();
    void getInterface();
    void setLog();
    void connectPropertiesWatcher();

    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    void on_comboBox_currentIndexChanged();

    void journalChanged();
    void connectLogWatcher();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
