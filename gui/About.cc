#include "About.h"
#include <QtGlobal>

#include "ui_about.h"

#include "version.h"


About::About(QWidget* parent):
	QDialog(parent)
{
	ui.setupUi(this);

	ui.leBuildDate->setText(buildDate);
	ui.leCommit->setText(gitCommit);
	ui.leQtVersion->setText(QT_VERSION_STR);
}
