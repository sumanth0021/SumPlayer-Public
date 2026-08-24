#pragma once

#include <QCoreApplication>
#include <QString>

inline QString iconPath(const QString &fileName)
{
    return QCoreApplication::applicationDirPath() + "/assets/icons/" + fileName;
}