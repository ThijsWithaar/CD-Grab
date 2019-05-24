#include "CddaWorker.h"

#include <iostream>

#include <boost/filesystem.hpp>

#include <QCoreApplication>


CoopMutex::CoopMutex():
	mtx(false)
{
}


/// Single treaded lock (not thread-safe)
class TryLocker
{
public:
	TryLocker(CoopMutex &lock):
		m_lock(&lock)
	{
		if(!lock.mtx)
		{
			m_owns = true;
			lock.mtx = true;
		}
		else
			m_owns = false;
	}

	~TryLocker()
	{
		if(m_owns)
			m_lock->mtx = false;
	}

	operator bool() const
	{
		return m_owns;
	}

private:
	CoopMutex* m_lock;
	bool m_owns;
};



CddaWorker::CddaWorker():
	m_timer(this)
{
	qRegisterMetaType<CDDA::DriveStatus>();
	qRegisterMetaType<CDDA::DiscStatus>();
	qRegisterMetaType<DiscInfo>();

	connect(&m_timer, &QTimer::timeout, this, &CddaWorker::Poll);
	m_timer.start(2000);
}


void CddaWorker::Poll()
{
	if(m_cdda == nullptr)
		return;

	CDDA::DriveStatus dr = m_cdda->GetDriveStatus();
	if(dr != m_dr)
	{
		m_dr = dr;
		emit DriveStatus(dr);
	}

	CDDA::DiscStatus di = m_cdda->GetDiscStatus();
	if(di != m_di)
	{
		emit DiscStatus(di);

		if(di != CDDA::DiscStatus::Audio)
			m_toc.reset();

		if(di == CDDA::DiscStatus::Audio && m_di != CDDA::DiscStatus::Audio)
			RefreshToc();

		m_di = di;
	}
}


void CddaWorker::ChangeDevice(const QString dev)
{
	emit StatusMessage("Chaning device to " + dev);

	m_cdda = std::make_unique<CDDA>(dev.toStdString());

	m_dr = m_cdda->GetDriveStatus();
	emit DriveStatus(m_dr);

	m_di = m_cdda->GetDiscStatus();
	emit DiscStatus(m_di);

	RefreshToc();
}


void CddaWorker::RefreshToc()
{
	TryLocker lock(m_NonReentrantMutex);
	if(m_cdda == nullptr || !lock)
		return;

	m_di = m_cdda->GetDiscStatus();
	emit DiscStatus(m_di);

	if(m_di == CDDA::DiscStatus::Audio)
	{
		auto toc = m_cdda->GetToc(false, false);
		if(m_discDb.Has(toc))
			m_tags = *m_discDb.Get(toc);
		else
		{
			// Get all info from disc, it's used by query:
			m_toc = m_cdda->GetToc(true, true);
			m_tags = m_discDb.Query(*m_toc);
		}
		emit NewToc(m_tags);
	}
}


void CddaWorker::Cancel()
{
	emit StatusMessage("Operation Cancelled");
	m_cancelled = true;
}


void CddaWorker::Grab(QString basename)
{
	TryLocker lock(m_NonReentrantMutex);
	if(!lock)
	{
		emit StatusMessage("Device busy");
		return;
	}

	m_cancelled = false;

	//std::cout << "CddaWorker::Grab di = " << m_di << std::endl;
	if(m_di != CDDA::DiscStatus::Audio)
		return;

	std::string basenames = basename.toStdString();
	std::string sectorFname = basenames + ".sdb";

	emit StatusMessage("Grabbing to: " + QString::fromStdString(sectorFname));

	SectorDb::Statistics stat;
	if(boost::filesystem::is_regular_file(sectorFname))
	{
		m_sdb = SectorDb(sectorFname);
	}
	else
	{
		// Poll() invalidates this: if it's there, we can use it
		if(!m_toc)
			m_toc = m_cdda->GetToc(true, true);
		m_sdb = SectorDb(m_cdda->SizeInSectors(), &m_toc.get());
	}

	const int minimumCommonCrcCount = 2;

	int retry, nRead = 0;
	for(retry = 0; retry < minimumCommonCrcCount + 2; retry++)
	{
		int nRead = m_sdb.Refine(*m_cdda, minimumCommonCrcCount, this);
		if(nRead == 0)
			break;
		else
			m_sdb.Save(sectorFname);
	}
	emit StatusMessage("Grabbing complete");
}


void CddaWorker::Encode()
{
	emit StatusMessage("Encoding");
}


void CddaWorker::Eject()
{
	if(m_cdda == nullptr)
		return;

	emit StatusMessage("Ejecting disc");
	m_cdda->Eject();
}


bool CddaWorker::OnSectorProgress(int nrSectors, int lba, double pct)
{
	emit Progress(pct);
	// Need to check for cancellation, see also m_NonReentrantMutex
	QCoreApplication::processEvents();
	return !m_cancelled;
}
