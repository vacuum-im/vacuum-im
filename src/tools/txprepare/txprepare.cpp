#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QDomDocument>
#include <QCoreApplication>

void myMessageHandler(QtMsgType type, const char *msg)
{
	switch (type)
	{
	case QtDebugMsg:
		fprintf(stderr, "%s\n", msg);
		break;
	case QtWarningMsg:
		fprintf(stderr, "Warning: %s\n", msg);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "Critical: %s\n", msg);
		break;
	case QtFatalMsg:
		fprintf(stderr, "Fatal: %s\n", msg);
		abort();
	}
}

int main(int argc, char *argv[])
{
	qInstallMsgHandler(myMessageHandler);
	QCoreApplication app(argc, argv);

	if (argc != 4)
	{
		qCritical("Usage: txprepare <source-dir> <prepared-dir> <numerus-count>.");
		return -1;
	}

	QDir srcDir(app.arguments().value(1),"*.ts",QDir::Name,QDir::Files);
	if (!srcDir.exists())
	{
		qCritical("Source directory '%s' not found.",srcDir.dirName().toLocal8Bit().constData());
		return -1;
	}

	QDir prepDir(app.arguments().value(2));
	if (!prepDir.exists() && !QDir::current().mkpath(app.arguments().value(2)))
	{
		qCritical("Destination directory '%s' not found.",prepDir.dirName().toLocal8Bit().constData());
		return -1;
	}

	int numerousCount = app.arguments().value(3).toInt();
	if (numerousCount < 1)
	{
		qCritical("Invalid numerous forms count %d.",numerousCount);
		return -1;
	}

	QDirIterator srcIt(srcDir);
	while (srcIt.hasNext())
	{
		QFile srcFile(srcIt.next());
		if (srcFile.open(QFile::ReadOnly))
		{
			QDomDocument doc;
			if (doc.setContent(&srcFile,true))
			{
				qDebug("Prepare translation for transifex '%s'.",srcFile.fileName().toLocal8Bit().constData());

				bool hasChanges = false;
				QDomElement rootElem = doc.firstChildElement("TS");
				QDomElement contextElem = rootElem.firstChildElement("context");
				while(!contextElem.isNull())
				{
					QDomElement messageElem = contextElem.firstChildElement("message");
					while(!messageElem.isNull())
					{
						QDomElement translationElem = messageElem.firstChildElement("translation");
						if (!translationElem.isNull() && messageElem.attribute("numerus")=="yes")
						{
							QString numerusText;
							QDomElement numerusEelem = translationElem.firstChildElement("numerusform");
							for(int count=0; !numerusEelem.isNull() || count<numerousCount; count++)
							{
								if (numerusEelem.isNull())
								{
									hasChanges = true;
									QDomElement newElem = doc.createElement("numerusform");
									newElem.appendChild(doc.createTextNode(numerusText));
									translationElem.appendChild(newElem);
								}
								else if (count >= numerousCount)
								{
									hasChanges = true;
									QDomElement oldElem = numerusEelem;
									numerusEelem = numerusEelem.nextSiblingElement("numerusform");
									translationElem.removeChild(oldElem);
								}
								else
								{
									numerusText = numerusEelem.text();
									numerusEelem = numerusEelem.nextSiblingElement("numerusform");
								}
							}
						}
						messageElem = messageElem.nextSiblingElement("message");
					}
					contextElem = contextElem.nextSiblingElement("context");
				}
				srcFile.close();

				if (hasChanges)
				{
					QFileInfo srcFileInfo(srcDir.absoluteFilePath(srcFile.fileName()));
					QFile prepFile(prepDir.absoluteFilePath(srcFileInfo.fileName()));
					if (prepFile.open(QFile::WriteOnly|QFile::Truncate))
					{
						prepFile.write(doc.toByteArray(2));
						prepFile.close();
					}
					else
					{
						qWarning("Failed to open destination file '%s' for write.",prepFile.fileName().toLocal8Bit().constData());
					}
				}
			}
			else
			{
				qWarning("Invalid translation source file '%s'.",srcFile.fileName().toLocal8Bit().constData());
			}
			srcFile.close();
		}
		else
		{
			qWarning("Could not open translation source file '%s'.",srcFile.fileName().toLocal8Bit().constData());
		}
	}

	return 0;
}
