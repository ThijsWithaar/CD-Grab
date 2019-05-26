#include "MainWindow.h"

#include <memory>
#include <sstream>
#include <iostream>

#include <cdgrab/cdda.h>

#include "TrackTableModel.h"
#include "About.h"


static std::string Join(std::string sep, std::vector<std::string> list)
{
	std::string r;
	auto it = list.begin();
	if(it == list.end())
		return r;
	r = *it;
	for(++it; it != list.end(); ++it)
		r += sep + *it;
	return r;
}


static std::vector<std::string> NonEmpty(std::vector<std::string> in)
{
	std::vector<std::string> r;
	for(auto& el: in)
		if(!el.empty())
			r.push_back(el);
	return r;
}



MainWindow::MainWindow(QWidget* parent):
	QMainWindow(parent),
	m_trackTable(this)
{
    qApp->installTranslator(&m_translator);

	ui.setupUi(this);

	ui.cDevice->clear();
	for(auto& dev: GetDevices())
		ui.cDevice->addItem(QString::fromStdString(dev));

	connect(ui.actionQuit, &QAction::triggered, QApplication::instance(), &QApplication::quit);
	connect(ui.actionAbout, &QAction::triggered, this, [this](){About about(this); about.exec();});
	connect(ui.actionEnglish, &QAction::triggered, this, [this](){ SetLanguage(English); });
	connect(ui.actionDutch, &QAction::triggered, this, [this](){ SetLanguage(Nederlands); });

	connect(ui.pbGrab, &QPushButton::clicked, this, &MainWindow::OnGrab);
	connect(ui.pbEncode, &QPushButton::clicked, this, &MainWindow::OnEncode);

	ui.tvTracks->setModel(&m_trackTable);

	StartCddaThread();
}


void MainWindow::SetLanguage(Languages lan)
{
	QString qmFile;
	switch(lan)
	{
		default:
			qmFile = ":/translations/dutch.qm";
	}

	m_translator.load(qmFile);
}


void MainWindow::StartCddaThread()
{
	CddaWorker *worker = new CddaWorker;
	worker->moveToThread(&m_cddaThread);
	// This will cleanup worker, so MainWindow doesn't track ownership
	connect(&m_cddaThread, &QThread::finished, worker, &QObject::deleteLater);

	// main -> worker
	connect(ui.cDevice, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), worker, &CddaWorker::ChangeDevice);
	connect(ui.pbAbort, &QPushButton::clicked, worker, &CddaWorker::Cancel);
	connect(this, &MainWindow::Grab, worker, &CddaWorker::Grab);
	connect(this, &MainWindow::Encode, worker, &CddaWorker::Encode);
	connect(ui.pbEject, &QPushButton::clicked, worker, &CddaWorker::Eject);

	// worker -> main
	connect(worker, &CddaWorker::DriveStatus, this, &MainWindow::OnDriveStatus);
	connect(worker, &CddaWorker::DiscStatus, this, &MainWindow::OnDiscStatus);
	connect(worker, &CddaWorker::NewToc, this, &MainWindow::OnTocChange);
	connect(worker, &CddaWorker::Progress, this, &MainWindow::OnProgress);
	connect(worker, &CddaWorker::StatusMessage, this, &MainWindow::OnStatusMessage);

	m_cddaThread.start();

	// Initialize the worker and drive status
	connect(this, &MainWindow::ChangeDevice, worker, &CddaWorker::ChangeDevice);
	emit ChangeDevice(ui.cDevice->currentText());
}


MainWindow::~MainWindow()
{
	std::cout << "m_cddaThread.quit" << std::endl;
	m_cddaThread.quit();
	std::cout << "m_cddaThread.wait" << std::endl;
	m_cddaThread.wait();
}


template<typename T>
QString AsQString(T v)
{
	std::stringstream ss;
	ss << v;
	return QString::fromStdString(ss.str());
}


void MainWindow::OnDriveStatus(CDDA::DriveStatus dr)
{
	ui.leDrive->setText(AsQString(dr));
}


void MainWindow::OnDiscStatus(CDDA::DiscStatus di)
{
	ui.leDisc->setText(AsQString(di));
}


void MainWindow::OnTocChange(DiscInfo tags)
{
	m_tags = tags;

	ui.leTitle->setText(QString::fromStdString(tags.title));
	ui.leArtist->setText(QString::fromStdString(tags.artist));
	ui.leUPC->setText(QString::fromStdString(tags.upc));

	m_trackTable.SetTracks(tags.track);
}


void MainWindow::OnProgress(double pct)
{
	ui.pbDevice->setValue((int)pct);
}


QString MainWindow::BaseName()
{
	QString basename;
	auto nes = NonEmpty({m_tags.artist, m_tags.title, m_tags.upc});
	basename = QString::fromStdString(Join(" - ", nes));
	if(nes.empty())
		basename.setNum(DiscInfoId(m_tags).id, 16);
	return basename;
}


void MainWindow::OnGrab()
{
	emit Grab(BaseName());
}


void MainWindow::OnEncode()
{
	QString basename = BaseName();
	QString sectorFname = basename + ".sdb";
	QString flacFname = basename + ".flac";

	emit Encode(sectorFname, flacFname);
}


void MainWindow::OnStatusMessage(QString msg)
{
	ui.statusbar->showMessage(msg, 5000);
}
