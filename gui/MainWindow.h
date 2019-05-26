#pragma once

#include "QMainWindow"

#include "ui_main.h"

#include <memory>
#include <optional>

#include <QThread>
#include <QTranslator>

#include "TrackTableModel.h"
#include "CddaWorker.h"


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent);

	~MainWindow() override;

private slots:
	// Response from the drive
	void OnDriveStatus(CDDA::DriveStatus);
	void OnDiscStatus(CDDA::DiscStatus);
	void OnTocChange(DiscInfo);
	void OnProgress(double);

	// User actions, with some bookkeeping
	void OnGrab();
	void OnEncode();
	void OnStatusMessage(QString msg);
signals:
	void ChangeDevice(const QString& deviceName);
	void Grab(const QString& baseName);
	void Encode(QString sectorDbFname, QString flacFname);

private:
	enum Languages {
		English,
		Nederlands
	};

	void SetLanguage(Languages);

	void StartCddaThread();

	QString BaseName();

	Ui::MainWindow ui;
	QTranslator m_translator;

	DiscInfo m_tags;
	TrackTableModel m_trackTable;

	QThread m_cddaThread;
};
