#include "skin.h"

QString Skin::FPathToSkins = "../../skin";
QString Skin::FSkinName = "default";
QHash<QString,Iconset> Skin::FIconsets;

Skin::Skin()
{

}

Skin::~Skin()
{

}

void Skin::setPathToSkins(const QString &APathToSkins)
{
  FPathToSkins = APathToSkins;
}

const QString &Skin::pathToSkins()
{
  return FPathToSkins;
}

void Skin::setSkin(const QString &ASkinName)
{
  FSkinName = ASkinName;
}

const QString &Skin::skin()
{
  return FSkinName;
}
