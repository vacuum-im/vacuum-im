#include "iconsetdelegate.h"

#include <QKeyEvent>
#include <QMouseEvent>

#define DEFAULT_ROWS 2

IconsetDelegate::IconsetDelegate(QObject *AParent) : QItemDelegate(AParent)
{

}

IconsetDelegate::~IconsetDelegate()
{
	foreach(const QString &name, FStorages.keys())
		qDeleteAll(FStorages[name]);
}

void IconsetDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QString name = AIndex.data(IDR_STORAGE_NAME).toString();
	QString subdir = AIndex.data(IDR_STORAGE_SUBDIR).toString();
	IconStorage *storage = FStorages.value(name).value(subdir,NULL);
	if (storage==NULL)
	{
		storage = new IconStorage(name,subdir);
		FStorages[name].insert(subdir,storage);
	}

	if (storage)
	{
		APainter->save();
		if (hasClipping())
			APainter->setClipRect(AOption.rect);

		drawBackground(APainter,AOption,AIndex);

		int space = 2;
		QRect drawRect = AOption.rect.adjusted(space,space,-space,-space);

		if (!AIndex.data(IDR_HIDE_ICONSET_NAME).toBool())
		{
			QRect checkRect(drawRect.topLeft(),doCheck(AOption,drawRect,AIndex.data(Qt::CheckStateRole)).size());
			drawCheck(APainter,AOption,checkRect,static_cast<Qt::CheckState>(AIndex.data(Qt::CheckStateRole).toInt()));
			drawRect.setLeft(checkRect.right()+space);


			QString displayText = storage->storageProperty(FILE_STORAGE_NAME,name+"/"+subdir);
			QRect textRect(drawRect.topLeft(),AOption.fontMetrics.size(Qt::TextSingleLine,displayText));

			QPalette::ColorGroup cg = AOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
			if (cg == QPalette::Normal && !(AOption.state & QStyle::State_Active))
				cg = QPalette::Inactive;
			if (AOption.state & QStyle::State_Selected)
				APainter->setPen(AOption.palette.color(cg, QPalette::HighlightedText));
			else
				APainter->setPen(AOption.palette.color(cg, QPalette::Text));
			APainter->drawText(textRect,AOption.displayAlignment,displayText);

			drawRect.setTop(textRect.bottom() + space);
			drawRect.setLeft(AOption.rect.left() + space);
		}

		int maxRows = AIndex.data(IDR_ICON_ROWS).isValid() ? AIndex.data(IDR_ICON_ROWS).toInt() : DEFAULT_ROWS;

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
				QPixmap pixmap = icon.pixmap(AOption.decorationSize,QIcon::Normal,QIcon::On);
				APainter->drawPixmap(left,top,pixmap);
				left+=AOption.decorationSize.width()+space;
			}
			iconIndex++;
			column++;

			if (left >= drawRect.right()-AOption.decorationSize.width())
			{
				column = 0;
				left = drawRect.left();
				top+=AOption.decorationSize.height()+space;
			}
		}

		drawFocus(APainter,AOption,AOption.rect);
		APainter->restore();
	}
	else
		QItemDelegate::paint(APainter,AOption,AIndex);
}

QSize IconsetDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QString name = AIndex.data(IDR_STORAGE_NAME).toString();
	QString subdir = AIndex.data(IDR_STORAGE_SUBDIR).toString();
	IconStorage *storage = FStorages.value(name).value(subdir,NULL);
	if (storage==NULL)
	{
		storage = new IconStorage(name,subdir);
		FStorages[name].insert(subdir,storage);
	}

	if (storage)
	{
		int space = 2;
		QSize size(0,0);

		if (!AIndex.data(IDR_HIDE_ICONSET_NAME).toBool())
		{
			QSize checkSize = doCheck(AOption,AOption.rect,AIndex.data(Qt::CheckStateRole)).size();
			QString displayText = storage->storageProperty(FILE_STORAGE_NAME,name+"/"+subdir);
			QSize textSize = AOption.fontMetrics.size(Qt::TextSingleLine,displayText);
			size.setHeight(qMax(checkSize.height(),textSize.height()));
			size.setWidth(checkSize.width()+textSize.width()+space);
		}

		int rows = AIndex.data(IDR_ICON_ROWS).isValid() ? AIndex.data(IDR_ICON_ROWS).toInt() : DEFAULT_ROWS;
		int iconWidth = qMin(AOption.rect.width(),storage->fileFirstKeys().count()*(AOption.decorationSize.width()+space));
		int iconHeight = (AOption.decorationSize.height()+space)*rows;
		return QSize(qMax(size.width(),iconWidth)+space,size.height()+iconHeight+space);
	}
	else
		return QItemDelegate::sizeHint(AOption,AIndex);
}

void IconsetDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &/*AIndex*/) const
{
	if (AOption.state & QStyle::State_Selected)
	{
		QPalette::ColorGroup cg = AOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
		if (cg == QPalette::Normal && !(AOption.state & QStyle::State_Active))
			cg = QPalette::Inactive;
		APainter->fillRect(AOption.rect, AOption.palette.brush(cg, QPalette::Highlight));
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
		QRect checkRect(drawRect.topLeft(),doCheck(AOption, AOption.rect, Qt::Checked).size());
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

