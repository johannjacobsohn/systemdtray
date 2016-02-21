/*
    TODO:
    x listen for property change
    x listen for dbus signal
    - minimieren, maximieren
    x exit
    - unprivilegiert auf systemd zugreifen

    - auf gnome testen
    - install
    - clean up
    - documentation
    - github
    - bessere Texte
    - about screen
*/
#include "QSystemTrayIcon"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtDBus>


//#include <dbus-protocol.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

//QString interface = "/org/freedesktop/systemd1/unit/so2_2dcamera_2eservice";
QString interface = "/org/freedesktop/systemd1/unit/ssh_2eservice";
QSystemTrayIcon *trayicon;

QMenu *tray_icon_menu;

QIcon icon_running;
QIcon icon_dead;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    icon_running = QIcon(":/new/prefix1/yoshimi.png");
    icon_dead = QIcon(":/new/prefix1/audio.png");

    trayicon = new QSystemTrayIcon(icon_running, this);
    tray_icon_menu = new QMenu;

    QAction *quit_action = new QAction( "Exit", trayicon );
    connect( quit_action, SIGNAL(triggered()), this, SLOT(exit_program()) );

    QAction *start_action = new QAction( "Start", trayicon );
    connect( start_action, SIGNAL(triggered()), this, SLOT(startService()) );

    QAction *stop_action = new QAction( "Stop", trayicon );
    connect( stop_action, SIGNAL(triggered()), this, SLOT(stopService()) );

    tray_icon_menu->addAction(start_action );
    tray_icon_menu->addAction(stop_action );
    tray_icon_menu->addAction(quit_action);

    trayicon->setContextMenu(tray_icon_menu);

    this->statusService();
    trayicon->show();
    QDBusConnection::systemBus().connect("org.freedesktop.systemd1",
                                          interface,
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged", this, SLOT(statusService()));
}

void MainWindow::startService()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface dbus_iface("org.freedesktop.systemd1", interface, "org.freedesktop.systemd1.Unit", bus);
//qDebug() << ALLOW_INTERACTIVE_AUTHORIZATION

//QDBUSINTERFACE_H.
    qDebug() << dbus_iface.call("Start", "replace") ;
}

void MainWindow::statusService()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface dbus_iface("org.freedesktop.systemd1", interface,
                              "org.freedesktop.systemd1.Unit", bus);

    QString state = dbus_iface.property("SubState").toString();

    trayicon->showMessage(QString("SystemD Applet"), state);

    ui->statustext->setText(state);

    if(state == "running")
        trayicon->setIcon(icon_running);
    else
        trayicon->setIcon(icon_dead);

}

void MainWindow::stopService()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface dbus_iface("org.freedesktop.systemd1", interface,
                                  "org.freedesktop.systemd1.Unit", bus);
    qDebug() << dbus_iface.call("Stop", "replace");
}

void MainWindow::exit_program()
{
    qDebug() << "exit program";
    QApplication::quit();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    this->startService();
}

void MainWindow::on_pushButton_2_clicked()
{
    this->stopService();
}
