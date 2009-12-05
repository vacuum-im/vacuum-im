#include "miscoptionswidget.h"

#include <QDir>
#include <QSettings>

MiscOptionsWidget::MiscOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
#ifdef Q_WS_WIN
  QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  QString startPath = reg.value(CLIENT_NAME, "").toString();
  if (startPath.isEmpty())
    ui.chbAutorun->setCheckState(Qt::Unchecked);
  else if (startPath.toLower() == QDir::toNativeSeparators(QApplication::applicationFilePath()).toLower())
    ui.chbAutorun->setCheckState(Qt::Checked);
  else
    ui.chbAutorun->setCheckState(Qt::PartiallyChecked);
#else
  ui.chbAutorun->setEnabled(false);
#endif
}

MiscOptionsWidget::~MiscOptionsWidget()
{

}

void MiscOptionsWidget::apply()
{
#ifdef Q_WS_WIN
  QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  if (ui.chbAutorun->checkState() == Qt::Checked)
    reg.setValue(CLIENT_NAME, QDir::toNativeSeparators(QApplication::applicationFilePath()));
  else if (ui.chbAutorun->checkState() == Qt::Unchecked)
    reg.remove(QApplication::applicationName());
#endif
}