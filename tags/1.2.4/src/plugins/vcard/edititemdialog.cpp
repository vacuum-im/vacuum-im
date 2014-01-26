#include "edititemdialog.h"

#include <QVBoxLayout>

EditItemDialog::EditItemDialog(const QString &AValue, QStringList ATags,
                               QStringList ATagList, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	ui.lneEdit->setText(AValue);
	QVBoxLayout *layout = new QVBoxLayout;
	ui.grbTags->setLayout(layout);
	foreach(const QString &tag, ATagList)
	{
		QCheckBox *checkBox = new QCheckBox(ui.grbTags);
		checkBox->setText(tag);
		checkBox->setCheckState(ATags.contains(tag) ? Qt::Checked : Qt::Unchecked);
		FCheckBoxes.append(checkBox);
		layout->addWidget(checkBox);
	}
	layout->addStretch();
}

EditItemDialog::~EditItemDialog()
{

}

QString EditItemDialog::value() const
{
	return ui.lneEdit->text();
}

QStringList EditItemDialog::tags() const
{
	QStringList tagList;
	foreach(QCheckBox *checkBox, FCheckBoxes)
		if (checkBox->checkState() == Qt::Checked)
			tagList.append(checkBox->text());
	return tagList;
}

void EditItemDialog::setLabelText(const QString &AText)
{
	ui.lblLabel->setText(AText);
}

void EditItemDialog::setValueEditable(bool AEditable)
{
	ui.lneEdit->setReadOnly(!AEditable);
}

void EditItemDialog::setTagsEditable(bool AEditable)
{
	ui.grbTags->setEnabled(AEditable);
}

