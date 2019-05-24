#pragma once

#include <QObject>
#include <QTimer>

#include <cdgrab/cdda.h>
#include <cdgrab/DiscInfoDatabase.h>
#include <cdgrab/file/sectorDb.h>


Q_DECLARE_METATYPE(CDDA::DriveStatus)
Q_DECLARE_METATYPE(CDDA::DiscStatus)
Q_DECLARE_METATYPE(DiscInfo)


struct CoopMutex
{
	CoopMutex();

	bool mtx;
};

class CddaWorker :
	public QObject,
	private SectorDbProgress
{
    Q_OBJECT
public:
	CddaWorker();

public slots:
	void ChangeDevice(const QString);
	void Cancel();
	void Grab(QString basename);
	void Encode();
	void Eject();

private slots:
	void Poll();

signals:
	void DriveStatus(CDDA::DriveStatus);
	void DiscStatus(CDDA::DiscStatus);
	void NewToc(DiscInfo);
	void Progress(double);
	void StatusMessage(QString msg);

private:
	void RefreshToc();
	bool OnSectorProgress(int nrSectors, int lba, double pct) override;

	QTimer m_timer;
	std::unique_ptr<CDDA> m_cdda;
	DiscInfoDatabase m_discDb;
	boost::optional<Toc> m_toc;
	DiscInfo m_tags;
	SectorDb m_sdb;

	/// Protect from being called on OnSectorProgress(), which handles the event loop
	CoopMutex m_NonReentrantMutex;

	CDDA::DriveStatus m_dr;
	CDDA::DiscStatus m_di;
	bool m_cancelled;
};
