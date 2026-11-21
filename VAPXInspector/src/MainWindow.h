#pragma once
#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QTableWidget>
#include <QSlider>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QHash>
#include "VapParser.h"
#include "PreviewWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
private slots:
    void openVap();
    void openJson();
    void selectSource();
    void bindImage();
    void applyText();
    void playPause();
    void checkConfig();
    void onVideoFrame(const QVideoFrame& frame);
    void onPosition(qint64 position);
private:
    void loadDocument(const QJsonObject& config, const QString& sourcePath);
    void populateSources();
    QString currentTag() const;
    QString normalizeTag(QString tag) const;
    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audio = nullptr;
    QVideoSink* m_sink = nullptr;
    PreviewWidget* m_preview = nullptr;
    QTableWidget* m_sources = nullptr;
    QSlider* m_timeline = nullptr;
    QLineEdit* m_text = nullptr;
    QLabel* m_status = nullptr;
    QTextEdit* m_log = nullptr;
    QJsonObject m_config;
    QHash<QString, BoundAsset> m_bindings;
    QString m_mediaPath;
    int m_totalFrames = 0;
    double m_fps = 20.0;
};
