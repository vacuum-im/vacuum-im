#include <QtDebug>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QDomDocument>
#include <QCoreApplication>

void myMessageOutput(QtMsgType type, const char *msg)
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
	qInstallMsgHandler(myMessageOutput);
	QCoreApplication app(argc, argv);

	if (argc != 3)
	{
		qCritical("Usage: autotranslate <destination-dir> <source-dir>.");
		return -1;
	}

	QDir dstDir(app.arguments().value(1));
	if (!dstDir.exists())
	{
		qCritical("Destination directory '%s' not found.",dstDir.dirName().toLocal8Bit().constData());
		return -1;
	}

	QDir srcDir(app.arguments().value(2),"*.ts",QDir::Name,QDir::Files);
	if (!srcDir.exists())
	{
		qCritical("Source directory '%s' not found.",srcDir.dirName().toLocal8Bit().constData());
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
				qDebug("Generation auto translation from '%s'.",srcFile.fileName().toLocal8Bit().constData());

				QDomElement rootElem = doc.firstChildElement("TS");
				rootElem.setAttribute("language",rootElem.attribute("sourcelanguage","en"));
				
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
				srcFile.close();

				QFileInfo srcFileInfo(srcDir.absoluteFilePath(srcFile.fileName()));
				QFile dstFile(dstDir.absoluteFilePath(srcFileInfo.fileName()));
				if (dstFile.open(QFile::WriteOnly|QFile::Truncate))
				{
					dstFile.write(doc.toByteArray());
					dstFile.close();
				}
				else
				{
					qWarning("Failed to open destination file '%s' for write.",dstFile.fileName().toLocal8Bit().constData());
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
