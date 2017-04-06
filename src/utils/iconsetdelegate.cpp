#include "iconsetdelegate.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>

#define DEFAULT_ICON_ROW_COUNT 1

IconsetDelegate::IconsetDelegate(QObject *AParent) : QStyledItemDelegate(AParent)
{

}

IconsetDelegate::~IconsetDelegate()
{
	foreach(const QString &name, FStorages.keys())
		qDeleteAll(FStorages[name]);
}

void IconsetDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QString name = AIndex.data(IDR_STORAGE).toString();
	QString subdir = AIndex.data(IDR_SUBSTORAGE).toString();

	IconStorage *storage = FStorages.value(name).value(subdir);
	if (storage==NULL && !name.isEmpty() && !subdir.isEmpty())
	{
		storage = new IconStorage(name,subdir);
		FStorages[name].insert(subdir,storage);
	}

	if (storage != NULL)
	{
		QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);

#if defined(Q_WS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
		QStyle *style = indexOption.widget ? indexOption.widget->style() : QApplication::style();
		if (qobject_cast<QWindowsVistaStyle *>(style))
		{
			indexOption.palette.setColor(QPalette::All, QPalette::HighlightedText, indexOption.palette.color(QPalette::Active, QPalette::Text));
			indexOption.palette.setColor(QPalette::All, QPalette::Highlight, indexOption.palette.base().color().darker(108));
		}
#endif

		APainter->save();
		APainter->setClipping(true);
		APainter->setClipRect(indexOption.rect);

		drawBackground(APainter,indexOption);

		int space = 2;
		QRect drawRect = indexOption.rect.adjusted(space,space,-space,-space);

		if (!AIndex.data(IDR_HIDE_STORAGE_NAME).toBool())
		{
			QRect ceckRect(drawRect.topLeft(),checkButtonRect(indexOption,drawRect,AIndex.data(Qt::CheckStateRole)).size());
			drawCheckButton(APainter,indexOption,ceckRect,static_cast<Qt::CheckState>(AIndex.data(Qt::CheckStateRole).toInt()));
			drawRect.setLeft(ceckRect.right()+space);

			QString displayText = storage->storageProperty(FILE_STORAGE_NAME,name+"/"+subdir);
			QRect textRect(drawRect.topLeft(),indexOption.fontMetrics.size(Qt::TextSingleLine,displayText));

			QPalette::ColorGroup cg = indexOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
			if (cg == QPalette::Normal && !(indexOption.state & QStyle::State_Active))
				cg = QPalette::Inactive;
			if (indexOption.state & QStyle::State_Selected)
				APainter->setPen(indexOption.palette.color(cg, QPalette::HighlightedText));
			else
				APainter->setPen(indexOption.palette.color(cg, QPalette::Text));
			APainter->drawText(textRect,indexOption.displayAlignment,displayText);

			drawRect.setTop(textRect.bottom() + space);
			drawRect.setLeft(indexOption.rect.left() + space);
		}

		int maxRows = AIndex.data(IDR_ICON_ROW_COUNT).isValid() ? AIndex.data(IDR_ICON_ROW_COUNT).toInt() : DEFAULT_ICON_ROW_COUNT;

		int row = 0;
		int column = 0;
		int iconIndex = 0;
		int left = drawRect.left();
		int top = drawRect.top();
		QList<QString> iconKeys = storage->fileFirstKeys();
		while (drawRect.bottom()>top && drawRect.right()>left && iconIndex<iconKeys.count() && row<maxRows)
		{
			QIcon icon = storage->getIcon(iconKeys.at(iconIndex));
			if (!icon.isNull())
			{
				QPixmap pixmap = icon.pixmap(indexOption.decorationSize,QIcon::Normal,QIcon::On);
				APainter->drawPixmap(left,top,pixmap);
				left += indexOption.decorationSize.width()+space;
			}
			iconIndex++;
			column++;

			if (left >= drawRect.right()-indexOption.decorationSize.width())
			{
				column = 0;
				left = drawRect.left();
				top += indexOption.decorationSize.height()+space;
			}
		}

		drawFocusRect(APainter,indexOption,indexOption.rect);

		APainter->restore();
	}
	else
	{
		QStyledItemDelegate::paint(APainter,AOption,AIndex);
	}
}

QSize IconsetDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QString name = AIndex.data(IDR_STORAGE).toString();
	QString subdir = AIndex.data(IDR_SUBSTORAGE).toString();
	
	IconStorage *storage = FStorages.value(name).value(subdir,NULL);
	if (storage==NULL && !name.isEmpty() && !subdir.isEmpty())
	{
		storage = new IconStorage(name,subdir);
		FStorages[name].insert(subdir,storage);
	}

	if (storage != NULL)
	{
		int space = 2;
		QSize size(0,0);

		QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);

		if (!AIndex.data(IDR_HIDE_STORAGE_NAME).toBool())
		{
			QSize checkSize = checkButtonRect(indexOption,indexOption.rect,AIndex.data(Qt::CheckStateRole)).size();
			QString displayText = storage->storageProperty(FILE_STORAGE_NAME,name+"/"+subdir);
			QSize textSize = indexOption.fontMetrics.size(Qt::TextSingleLine,displayText);
			size.setHeight(qMax(checkSize.height(),textSize.height()));
			size.setWidth(checkSize.width()+textSize.width()+space);
		}

		int rows = AIndex.data(IDR_ICON_ROW_COUNT).isValid() ? AIndex.data(IDR_ICON_ROW_COUNT).toInt() : DEFAULT_ICON_ROW_COUNT;
		int iconWidth = qMin(indexOption.rect.width(),storage->fileFirstKeys().count()*(indexOption.decorationSize.width()+space));
		int iconHeight = (indexOption.decorationSize.height()+space)*rows;

		return QSize(qMax(size.width(),iconWidth)+space,size.height()+iconHeight+space);
	}
	else
	{
		return QStyledItemDelegate::sizeHint(AOption,AIndex);
	}
}

bool IconsetDelegate::editorEvent(QEvent *AEvent, QAbstractItemModel *AModel, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex)
{
	Qt::ItemFlags flags = AModel->flags(AIndex);
	if (!(flags & Qt::ItemIsUserCheckable) || !(AOption.state & QStyle::State_Enabled) || !(flags & Qt::ItemIsEnabled))
		return false;

	QVariant value = AIndex.data(Qt::CheckStateRole);
	if (!value.isValid())
		return false;

	if ((AEvent->type() == QEvent::MouseButtonRelease) || (AEvent->type() == QEvent::MouseButtonDblClick))
	{
		int space = 2;
		QRect drawRect = AOption.rect.adjusted(space,space,-space,-space);
		QRect checkRect(drawRect.topLeft(),checkButtonRect(AOption, AOption.rect, Qt::Checked).size());

		if (!checkRect.contains(static_cast<QMouseEvent*>(AEvent)->pos()))
			return false;

		if (AEvent->type() == QEvent::MouseButtonDblClick)
			return true;
	}
	else if (AEvent->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(AEvent);
		if (keyEvent->key() != Qt::Key_Space && keyEvent->key() != Qt::Key_Select)
			return false;
	}
	else
	{
		return false;
	}

	Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
	return AModel->setData(AIndex, state, Qt::CheckStateRole);
}

void IconsetDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption) const
{
	QStyle *style = AIndexOption.widget ? AIndexOption.widget->style() : QApplication::style();
	style->proxy()->drawPrimitive(QStyle::PE_PanelItemViewItem,&AIndexOption,APainter,AIndexOption.widget);
}

void IconsetDelegate::drawFocusRect(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption, const QRect &ARect) const
{
	if ((AIndexOption.state & QStyle::State_HasFocus) > 0)
	{
		QStyle *style = AIndexOption.widget ? AIndexOption.widget->style() : QApplication::style();

		QStyleOptionFocusRect focusOption;
		focusOption.QStyleOption::operator=(AIndexOption);
		focusOption.rect = ARect;
		focusOption.state |= QStyle::State_KeyboardFocusChange|QStyle::State_Item;

		QPalette::ColorGroup cg = (AIndexOption.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
		QPalette::ColorRole cr = (AIndexOption.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window;
		focusOption.backgroundColor = AIndexOption.palette.color(cg,cr);

		style->proxy()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, APainter);
	}
}

QRect IconsetDelegate::checkButtonRect(const QStyleOptionViewItemV4 &AIndexOption, const QRect &ABounding, const QVariant &AValue) const
{
	if (AValue.isValid())
	{
		QStyleOptionButton buttonOption;
		buttonOption.QStyleOption::operator=(AIndexOption);
		buttonOption.rect = ABounding;

		const QStyle *style = AIndexOption.widget ? AIndexOption.widget->style()->proxy() : QApplication::style()->proxy();
		return style->subElementRect(QStyle::SE_ViewItemCheckIndicator, &buttonOption, AIndexOption.widget);
	}
	return QRect();
}

void IconsetDelegate::drawCheckButton(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption, const QRect &ARect, Qt::CheckState AState) const
{
	if (ARect.isValid())
	{
		QStyleOptionViewItem checkOption(AIndexOption);
		checkOption.rect = ARect;
		checkOption.state = checkOption.state & ~QStyle::State_HasFocus;

		switch (AState)
		{
		case Qt::Unchecked:
			checkOption.state |= QStyle::State_Off;
			break;
		case Qt::PartiallyChecked:
			checkOption.state |= QStyle::State_NoChange;
			break;
		case Qt::Checked:
			checkOption.state |= QStyle::State_On;
			break;
		}

		const QStyle *style = AIndexOption.widget ? AIndexOption.widget->style()->proxy() : QApplication::style()->proxy();
		style->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &checkOption, APainter, AIndexOption.widget);
	}
}

QStyleOptionViewItemV4 IconsetDelegate::indexStyleOption(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 indexOption = AOption;

	indexOption.index = AIndex;

	QVariant value = AIndex.data(Qt::FontRole);
	if (value.isValid() && !value.isNull()) 
	{
		indexOption.font = qvariant_cast<QFont>(value).resolve(indexOption.font);
		indexOption.fontMetrics = QFontMetrics(indexOption.font);
	}

	value = AIndex.data(Qt::TextAlignmentRole);
	if (value.isValid() && !value.isNull())
		indexOption.displayAlignment = Qt::Alignment(value.toInt());

	value = AIndex.data(Qt::ForegroundRole);
	if (value.canConvert<QBrush>())
		indexOption.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(value));

	value = AIndex.data(Qt::CheckStateRole);
	if (value.isValid() && !value.isNull()) 
		indexOption.features |= QStyleOptionViewItemV2::HasCheckIndicator;

	value = AIndex.data(Qt::DecorationRole);
	if (value.isValid() && !value.isNull()) 
		indexOption.features |= QStyleOptionViewItemV2::HasDecoration;

	value = AIndex.data(Qt::DisplayRole);
	if (value.isValid() && !value.isNull()) 
		indexOption.features |= QStyleOptionViewItemV2::HasDisplay;

	indexOption.backgroundBrush = qvariant_cast<QBrush>(AIndex.data(Qt::BackgroundRole));

	return indexOption;
}
