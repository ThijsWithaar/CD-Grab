#pragma once

#include "QMainWindow"

#include "ui_main.h"
#include "ui_tags.h"

#include <memory>
#include <optional>

#include <QThread>

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
	void StartCddaThread();

	QString BaseName();

	Ui::MainWindow ui;

	DiscInfo m_tags;
	TrackTableModel m_trackTable;

	QThread m_cddaThread;
};
