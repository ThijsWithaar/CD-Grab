#pragma once

#include <QDialog>

#include "ui_about.h"


class About: public QDialog
{
	Q_OBJECT
public:
	About(QWidget* parent);

private:
	Ui::About ui;
};
