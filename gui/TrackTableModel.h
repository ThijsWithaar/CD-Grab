#pragma once

#include <QAbstractTableModel>

#include <cdgrab/DiscInfoDatabase.h>


class TrackTableModel: public QAbstractTableModel
{
	Q_OBJECT
public:
	TrackTableModel(QObject *parent = nullptr);

	void SetTracks(const std::vector<TrackInfo>& tracks);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	int rowCount(const QModelIndex&) const override;
	int columnCount(const QModelIndex&) const override;
	QVariant data(const QModelIndex&, int role) const override;
private:
	std::vector<TrackInfo> m_tracks;
	const QVector<QString> m_headers;
};
