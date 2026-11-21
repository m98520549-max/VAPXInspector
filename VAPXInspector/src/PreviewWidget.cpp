#include "PreviewWidget.h"
#include <QPainter>
#include <QJsonDocument>
#include <QRegularExpression>
#include <algorithm>

PreviewWidget::PreviewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(420, 360);
    setAutoFillBackground(false);
}
void PreviewWidget::setVideoFrame(const QImage& image) { m_video = image; update(); }
void PreviewWidget::setConfig(const QJsonObject& config) { m_config = config; update(); }
void PreviewWidget::setCurrentFrame(int frame) { m_frame = qMax(0, frame); update(); }
void PreviewWidget::setBindings(const QHash<QString, BoundAsset>& b) { m_bindings = b; update(); }
void PreviewWidget::setShowBounds(bool on) { m_showBounds = on; update(); }

QJsonObject PreviewWidget::sourceForId(const QString& id) const {
    const QJsonArray src = m_config.value("src").toArray();
    for (const auto& v : src) {
        const auto o = v.toObject();
        if (o.value("srcId").toVariant().toString() == id) return o;
    }
    return {};
}
QString PreviewWidget::tagForSrcId(const QString& id) const {
    const QString tag = sourceForId(id).value("srcTag").toString();
    QString key = tag;
    if (key.startsWith('[') && key.endsWith(']')) key = key.mid(1, key.size()-2);
    return key;
}

void PreviewWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(38, 42, 50));
    const QJsonObject info = m_config.value("info").toObject();
    const qreal vw = info.value("w").toDouble(m_video.width() > 0 ? m_video.width() : 720);
    const qreal vh = info.value("h").toDouble(m_video.height() > 0 ? m_video.height() : 1280);
    const qreal scale = qMin((width()-24)/qMax<qreal>(1,vw), (height()-24)/qMax<qreal>(1,vh));
    const QSizeF target(vw*scale, vh*scale);
    const QRectF canvas((width()-target.width())/2, (height()-target.height())/2, target.width(), target.height());

    // Checkerboard makes transparent regions visible.
    const int cell = 16;
    for (int y = int(canvas.top()); y < int(canvas.bottom()); y += cell)
        for (int x = int(canvas.left()); x < int(canvas.right()); x += cell)
            p.fillRect(QRect(x,y,cell,cell), ((x/cell+y/cell)&1) ? QColor("#d8d8d8") : QColor("#f4f4f4"));

    if (!m_video.isNull()) p.drawImage(canvas, m_video);

    const QJsonArray frames = m_config.value("frame").toArray();
    QJsonObject current;
    int best = -1;
    for (const auto& fv : frames) {
        const QJsonObject fo = fv.toObject();
        const int idx = fo.value("i").toInt(-1);
        if (idx <= m_frame && idx >= best) { best = idx; current = fo; }
    }
    struct Obj { QJsonObject o; int z; };
    QList<Obj> objects;
    for (const auto& ov : current.value("obj").toArray()) {
        const auto o = ov.toObject();
        objects.push_back({o, o.value("z").toInt(0)});
    }
    std::sort(objects.begin(), objects.end(), [](const Obj& a, const Obj& b){ return a.z < b.z; });

    for (const auto& item : objects) {
        const QJsonObject o = item.o;
        const QString id = o.value("srcId").toVariant().toString();
        const QJsonArray a = o.value("frame").toArray();
        if (a.size() < 4) continue;
        const QRectF r(canvas.left()+a[0].toDouble()*scale, canvas.top()+a[1].toDouble()*scale,
                       a[2].toDouble()*scale, a[3].toDouble()*scale);
        const QString tag = tagForSrcId(id);
        const BoundAsset asset = m_bindings.value(tag);
        if (asset.isSet) {
            if (!asset.image.isNull()) {
                QImage img = asset.image;
                const QString fit = sourceForId(id).value("fitType").toString("fitXY");
                if (fit == "centerCrop") {
                    const QSize wanted(qMax(1,int(r.width())), qMax(1,int(r.height())));
                    img = img.scaled(wanted, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    const int x = qMax(0,(img.width()-wanted.width())/2);
                    const int y = qMax(0,(img.height()-wanted.height())/2);
                    img = img.copy(x,y,wanted.width(),wanted.height());
                } else {
                    img = img.scaled(r.size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                }
                p.drawImage(r, img);
            } else if (!asset.text.isNull()) {
                const QJsonObject src = sourceForId(id);
                QColor color(src.value("color").toString("#FFFFFF"));
                if (!color.isValid()) color = Qt::white;
                QFont font = p.font();
                font.setBold(src.value("style").toString() == "b");
                font.setPixelSize(qMax(10, int(r.height()*0.72)));
                p.setFont(font);
                p.setPen(color);
                p.drawText(r.adjusted(2,0,-2,0), Qt::AlignCenter | Qt::TextWordWrap, asset.text);
            }
        }
        if (m_showBounds) {
            QPen pen(asset.isSet ? QColor("#00C853") : QColor("#FF5252"));
            pen.setWidth(2);
            pen.setStyle(Qt::DashLine);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(r);
            p.setPen(Qt::black);
            p.setBrush(QColor(255,255,255,210));
            p.drawRect(QRectF(r.topLeft(), QSizeF(qMin(140.0,r.width()),18)));
            p.setPen(Qt::black);
            p.drawText(QRectF(r.left()+3,r.top()+1,qMin(136.0,r.width()-4),16), Qt::AlignLeft|Qt::AlignVCenter, tag.isEmpty() ? id : tag);
        }
    }
    p.setPen(Qt::white);
    p.drawText(8, height()-8, QString("Frame: %1").arg(m_frame));
}
