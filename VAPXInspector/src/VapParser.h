#pragma once
#include <QByteArray>
#include <QJsonDocument>
#include <QString>

struct VapParseResult {
    bool ok = false;
    QString error;
    QByteArray jsonBytes;
    QJsonDocument document;
    qint64 boxOffset = -1;
};

class VapParser {
public:
    static VapParseResult parseFile(const QString& path);
    static VapParseResult parseJsonFile(const QString& path);
private:
    static bool findVapcBox(const QByteArray& data, QByteArray& payload, qint64& offset, QString& error);
};
