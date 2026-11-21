#include "VapParser.h"
#include <QFile>
#include <QJsonParseError>
#include <QtEndian>
#include <cstring>

static quint32 readU32(const char* p) {
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(p));
}
static quint64 readU64(const char* p) {
    return qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(p));
}

bool VapParser::findVapcBox(const QByteArray& data, QByteArray& payload, qint64& offset, QString& error) {
    // MP4 BOX: size(32), type(4), optional largesize(64). Scan top-level boxes.
    // vapc is normally stored as a top-level custom box in VAP files.
    qint64 pos = 0;
    while (pos + 8 <= data.size()) {
        quint64 size = readU32(data.constData() + pos);
        const QByteArray type = data.mid(pos + 4, 4);
        qint64 header = 8;
        if (size == 1) {
            if (pos + 16 > data.size()) break;
            size = readU64(data.constData() + pos + 8);
            header = 16;
        } else if (size == 0) {
            size = quint64(data.size() - pos);
        }
        if (size < quint64(header) || size > quint64(data.size() - pos)) {
            error = QString("MP4 BOX 长度异常，偏移 %1，类型 %2").arg(pos).arg(QString::fromLatin1(type));
            return false;
        }
        if (type == "vapc") {
            payload = data.mid(pos + header, qint64(size) - header);
            offset = pos;
            return true;
        }
        pos += qint64(size);
    }
    error = "未在 MP4 顶层 BOX 中找到 vapc。请确认文件是 VAP/VAPX，或尝试载入独立 vapc JSON。";
    return false;
}

VapParseResult VapParser::parseFile(const QString& path) {
    VapParseResult r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        r.error = "无法打开文件：" + f.errorString();
        return r;
    }
    QByteArray bytes = f.readAll();
    if (bytes.size() < 8) {
        r.error = "文件太小，不是有效的 MP4/VAP 文件。";
        return r;
    }
    if (!findVapcBox(bytes, r.jsonBytes, r.boxOffset, r.error)) return r;
    // Some tool versions store a UTF-8 JSON string directly; trim padding/NUL.
    r.jsonBytes = r.jsonBytes.trimmed();
    while (!r.jsonBytes.isEmpty() && r.jsonBytes.endsWith('\0')) r.jsonBytes.chop(1);
    QJsonParseError pe;
    r.document = QJsonDocument::fromJson(r.jsonBytes, &pe);
    if (pe.error != QJsonParseError::NoError) {
        r.error = QString("找到 vapc BOX（偏移 %1），但 JSON 解析失败：%2").arg(r.boxOffset).arg(pe.errorString());
        return r;
    }
    if (!r.document.isObject()) {
        r.error = "vapc JSON 根节点不是对象。";
        return r;
    }
    r.ok = true;
    return r;
}

VapParseResult VapParser::parseJsonFile(const QString& path) {
    VapParseResult r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        r.error = "无法打开 JSON：" + f.errorString();
        return r;
    }
    r.jsonBytes = f.readAll();
    QJsonParseError pe;
    r.document = QJsonDocument::fromJson(r.jsonBytes, &pe);
    if (pe.error != QJsonParseError::NoError || !r.document.isObject()) {
        r.error = "JSON 解析失败：" + pe.errorString();
        return r;
    }
    r.ok = true;
    return r;
}
