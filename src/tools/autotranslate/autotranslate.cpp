#include <QtDebug>

#include <QDir>
#include <QObject>
#include <QDirIterator>
#include <QDomDocument>
#include <QCoreApplication>
#include <QMessageLogContext>

void myMessageOutput(QtMsgType AType, const QMessageLogContext &AContext, const QString &AMessage)
{
	Q_UNUSED(AContext)
	QByteArray localMsg = AMessage.toLocal8Bit();
	switch (AType)
	{
		 case QtDebugMsg:
			 fprintf(stderr, "%s\n", localMsg.constData());
			 break;
		 case QtWarningMsg:
			 fprintf(stderr, "Warning: %s\n", localMsg.constData());
			 break;
		 case QtCriticalMsg:
			 fprintf(stderr, "Critical: %s\n", localMsg.constData());
			 break;
		 case QtFatalMsg:
			 fprintf(stderr, "Fatal: %s\n", localMsg.constData());
			 abort();
	}
}

int main(int argc, char *argv[])
{
	qInstallMessageHandler(myMessageOutput);
	QCoreApplication app(argc, argv);

	if (argc != 2)
	{
		qCritical("Usage: autotranslate <diresctory>.");
		return -1;
	}

	QDir dir(app.arguments().value(1),"*.ts",QDir::Name,QDir::Files);
	if (!dir.exists())
	{
		qCritical("Directory '%s' not found.",dir.dirName().toLocal8Bit().constData());
		return -1;
	}

	QDirIterator it(dir);
	while (it.hasNext())
	{
		QFile file(it.next());
		if (file.open(QFile::ReadOnly))
		{
			QDomDocument doc;
			if (doc.setContent(file.readAll(),true))
			{
				qDebug("Auto translating file '%s'.",file.fileName().toLocal8Bit().constData());
				QDomElement rootElem = doc.firstChildElement("TS");
				
				QDomElement contextElem = rootElem.firstChildElement("context");
				while(!contextElem.isNull())
				{
					QDomElement messageElem = contextElem.firstChildElement("message");
					while(!messageElem.isNull())
					{
						QDomElement sourceElem = messageElem.firstChildElement("source");
						QDomElement translationElem = messageElem.firstChildElement("translation");
						if (!sourceElem.isNull() && !translationElem.isNull())
						{
							QString sourceText = sourceElem.text();
							if (messageElem.attribute("numerus") == "yes")
							{
								QDomElement numerusEelem = translationElem.firstChildElement("numerusform");
								while(!numerusEelem.isNull())
								{
									numerusEelem.removeChild(numerusEelem.firstChild());
									numerusEelem.appendChild(doc.createTextNode(sourceText));
									numerusEelem = numerusEelem.nextSiblingElement("numerusform");
								}
							}
							else
							{
								translationElem.removeChild(translationElem.firstChild());
								translationElem.appendChild(doc.createTextNode(sourceText));
							}
							translationElem.removeAttribute("type");
						}
						messageElem = messageElem.nextSiblingElement("message");
					}
					contextElem = contextElem.nextSiblingElement("context");
				}
				file.close();

				if (file.open(QFile::WriteOnly|QFile::Truncate))
				{
					file.write(doc.toByteArray(4));
				}
				else
				{
					qWarning("Failed to open file '%s' for write.",file.fileName().toLocal8Bit().constData());
				}
			}
			else
			{
				qWarning("Invalid translation source file '%s'.",file.fileName().toLocal8Bit().constData());
			}
			file.close();
		}
		else
		{
			qWarning("Could not open translation source file '%s'.",file.fileName().toLocal8Bit().constData());
		}
	}
	return 0;
}
