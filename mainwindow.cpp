#include "QSystemTrayIcon"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtDBus>
#include <QFileSystemWatcher>

extern "C" {
#include "systemd/sd-bus.h"
#include "systemd/sd-journal.h"
}

#include "mainwindow.h"
#include "ui_mainwindow.h"

QString interface;
QString service;
QSystemTrayIcon *trayicon;

QMenu *tray_icon_menu;

QIcon icon_running;
QIcon icon_dead;
QIcon icon_unknown;

sd_bus_error error = SD_BUS_ERROR_NULL;
sd_bus_message *m = NULL;
sd_bus *bus = NULL;
sd_journal *j;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    icon_running = QIcon(":/default/icons/status-running.png");
    icon_dead = QIcon(":/default/icons/status-failed.png");
    icon_unknown = QIcon(":/default/icons/status-unknown.png");

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
    trayicon->show();

    MainWindow::getInterface();
    MainWindow::connectPropertiesWatcher();

    int r;

    /* Connect to the system bus */
    r = sd_bus_open_system(&bus);
    if (r < 0) {
            fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
    }

    sd_bus_set_allow_interactive_authorization(bus, 1);

    MainWindow::setLog();

    QDBusConnection bus1 = QDBusConnection::systemBus();
    QDBusInterface dbus_iface("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                              "org.freedesktop.systemd1.Manager", bus1);
    QDBusMessage res = dbus_iface.call("ListUnits");
    QDBusArgument dbusarg = qvariant_cast<QDBusArgument> ( res.arguments() [0] );
    dbusarg.beginArray();

    while ( !dbusarg.atEnd() ){
        QVariant variant = dbusarg.asVariant();
        QDBusArgument dbusArgs = variant.value<QDBusArgument>();
        QString name;
        dbusArgs.beginMap();
        dbusArgs >> name;
        if(name.contains(".service")) {
           ui->comboBox->addItem(name);
        }
     }

    MainWindow::connectLogWatcher();
}

void MainWindow::connectLogWatcher()
{
    QFileSystemWatcher *watcher = new QFileSystemWatcher();
    QDirIterator it("/var/log/journal/", QStringList() << "*", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()){
        watcher->addPath(it.filePath());
        it.next();
    }
    QObject::connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(journalChanged()));
    QObject::connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(journalChanged()));
}

void MainWindow::connectPropertiesWatcher()
{
    // disconnect?
    QDBusConnection::systemBus().connect("org.freedesktop.systemd1",
                                          interface,
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged", this, SLOT(statusService()));
}

void MainWindow::setLog()
{
    const char *data;
    QString d =  "_SYSTEMD_UNIT=" + service;

    QByteArray ba = d.toLatin1();
    const char *match = ba.data();

    size_t length;
    sd_journal_open(&j, SD_JOURNAL_SYSTEM);

    sd_journal_add_match(j, match, 0);
    sd_journal_seek_tail(j);

    ui->textBrowser->clear();

    for(int i = 0; i < 10; i++){
        if(!sd_journal_previous(j)) break;

        sd_journal_get_data(j, "MESSAGE", (const void **)&data, &length);

        ui->textBrowser->append( data );
    }
}

void MainWindow::getInterface()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface dbus_iface("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                              "org.freedesktop.systemd1.Manager", bus);
    QDBusReply<QDBusObjectPath> unit = dbus_iface.call("GetUnit", service);
    if (unit.isValid()) {
            interface = unit.value().path();
    }
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

void MainWindow::changeService(const char * method)
{
    QByteArray ba = service.toLatin1();
    const char *service_string = ba.data();

    /* Issue the method call and store the respons message in m */
    int r = sd_bus_call_method(bus,
                           "org.freedesktop.systemd1",           /* service to contact */
                           "/org/freedesktop/systemd1",          /* object path */
                           "org.freedesktop.systemd1.Manager",   /* interface name */
                           method,                               /* method name */
                           &error,                               /* object to return error in */
                           &m,                                   /* return message on success */
                           "ss",                                 /* input signature */
                           service_string,                               /* first argument */
                           "replace");                           /* second argument */

    if(r < 0){
        qDebug() << "failed to start service" << service;
        qDebug() << error.message;
        qDebug() << r << strerror(r);
    }
}

void MainWindow::startService()
{
    MainWindow::changeService("StartUnit");
}

void MainWindow::stopService()
{
    MainWindow::changeService("StopUnit");
}

void MainWindow::exit_program()
{
    QApplication::quit();
}

MainWindow::~MainWindow()
{
    delete ui;
    sd_journal_close(j);

    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    sd_bus_unref(bus);
}

void MainWindow::on_pushButton_clicked()
{
    this->startService();
}

void MainWindow::on_pushButton_2_clicked()
{
    this->stopService();
}

void MainWindow::on_comboBox_currentIndexChanged()
{
    service = ui->comboBox->currentText();
    MainWindow::getInterface();
    MainWindow::connectPropertiesWatcher();
    MainWindow::statusService();
    MainWindow::setLog();
}

void MainWindow::journalChanged()
{
    MainWindow::setLog();
}
