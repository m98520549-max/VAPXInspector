#pragma once
#include <QWidget>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QVariant>

struct BoundAsset {
    QString type;
    QString text;
    QImage image;
    bool isSet = false;
};

class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);
    void setVideoFrame(const QImage& image);
    void setConfig(const QJsonObject& config);
    void setCurrentFrame(int frame);
    void setBindings(const QHash<QString, BoundAsset>& bindings);
    void setShowBounds(bool on);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QImage m_video;
    QJsonObject m_config;
    QHash<QString, BoundAsset> m_bindings;
    int m_frame = 0;
    bool m_showBounds = true;
    QString tagForSrcId(const QString& id) const;
    QJsonObject sourceForId(const QString& id) const;
};
