#include "TrackTableModel.h"


std::string asString(std::array<char, 12> ar)
{
	if(ar[0] == '\0')
		return std::string();
	return std::string(ar.data(), ar.size());
}



TrackTableModel::TrackTableModel(QObject* parent):
	QAbstractTableModel(parent),
	m_headers{
		tr("Artist"),
		tr("Title"),
		tr("ISRC")
	}
{
}


void TrackTableModel::SetTracks(const std::vector<TrackInfo>& tracks)
{
	int nTracksOld = (int)m_tracks.size();
	int nTracksNew = (int)tracks.size();

	if(nTracksOld > 0)
	{
		beginRemoveRows(QModelIndex(), 0, nTracksOld-1);
		removeRows(0, nTracksOld);
		endRemoveRows();
	}

	m_tracks = tracks;

	beginInsertRows(QModelIndex(), 0, nTracksNew-1);
	insertRows(0, nTracksNew);
	endInsertRows();
}


QVariant TrackTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return {};
	return m_headers[section];
}


int TrackTableModel::rowCount(const QModelIndex&) const
{
	return static_cast<int>(m_tracks.size());
}


int TrackTableModel::columnCount(const QModelIndex&) const
{
	return m_headers.size();
}


QVariant TrackTableModel::data(const QModelIndex& idx, int role) const
{
	if(role != Qt::DisplayRole || !idx.isValid())
		return {};

	QVariant r;
	switch(idx.column())
	{
	case 0:
		r = QString::fromStdString(m_tracks[idx.row()].artist);
		break;
	case 1:
		r = QString::fromStdString(m_tracks[idx.row()].songName);
		break;
	case 2:
	{
		auto isrc = asString(m_tracks[idx.row()].isrc);
		r = QString::fromStdString(isrc);
		break;
	}
	default:
		assert(!"Invalid column");
	}
	return r;
}
