#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPainter>
#include <QVideoFrame>
#include <QFile>
#include <QDateTime>
#include <QSet>
#include <QUrl>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("VAPX Inspector - 图片/文字占位符调试器");
    resize(1420, 900);
    m_player = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_player->setAudioOutput(m_audio);
    m_sink = new QVideoSink(this);
    m_player->setVideoSink(m_sink);
    connect(m_sink, &QVideoSink::videoFrameChanged, this, &MainWindow::onVideoFrame);
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPosition);
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& e){
        m_status->setText("视频解码错误：" + e);
        m_log->append("[VIDEO ERROR] " + e);
    });

    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    auto* toolbar = new QHBoxLayout;
    auto* open = new QPushButton("打开 VAP/MP4");
    auto* json = new QPushButton("打开 vapc JSON");
    auto* play = new QPushButton("播放 / 暂停");
    auto* check = new QPushButton("检查配置");
    toolbar->addWidget(open); toolbar->addWidget(json); toolbar->addWidget(play); toolbar->addWidget(check);
    toolbar->addStretch();
    m_status = new QLabel("请打开 .vap 文件或独立 vapc JSON");
    toolbar->addWidget(m_status);
    root->addLayout(toolbar);

    auto* split = new QSplitter(Qt::Horizontal);
    m_sources = new QTableWidget(0,4);
    m_sources->setHorizontalHeaderLabels({"类型","srcId","srcTag","绑定状态"});
    m_sources->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_sources->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sources->setEditTriggers(QAbstractItemView::NoEditTriggers);
    split->addWidget(m_sources);

    m_preview = new PreviewWidget;
    split->addWidget(m_preview);

    auto* right = new QWidget;
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(new QLabel("占位符替换"));
    rightLayout->addWidget(new QLabel("文字内容（选择 txt 类型资源后应用）："));
    m_text = new QLineEdit;
    m_text->setPlaceholderText("输入要替换的文字");
    rightLayout->addWidget(m_text);
    auto* apply = new QPushButton("应用文字");
    auto* image = new QPushButton("绑定图片");
    auto* bounds = new QPushButton("显示/隐藏边界框");
    rightLayout->addWidget(apply);
    rightLayout->addWidget(image);
    rightLayout->addWidget(bounds);
    rightLayout->addWidget(new QLabel("测试日志"));
    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    rightLayout->addWidget(m_log);
    split->addWidget(right);
    split->setStretchFactor(0,2); split->setStretchFactor(1,5); split->setStretchFactor(2,2);
    root->addWidget(split,1);

    auto* bottom = new QHBoxLayout;
    m_timeline = new QSlider(Qt::Horizontal);
    m_timeline->setRange(0,0);
    bottom->addWidget(new QLabel("时间轴"));
    bottom->addWidget(m_timeline,1);
    root->addLayout(bottom);
    setCentralWidget(central);

    connect(open,&QPushButton::clicked,this,&MainWindow::openVap);
    connect(json,&QPushButton::clicked,this,&MainWindow::openJson);
    connect(play,&QPushButton::clicked,this,&MainWindow::playPause);
    connect(check,&QPushButton::clicked,this,&MainWindow::checkConfig);
    connect(m_sources,&QTableWidget::cellClicked,this,[this](int,int){selectSource();});
    connect(image,&QPushButton::clicked,this,&MainWindow::bindImage);
    connect(apply,&QPushButton::clicked,this,&MainWindow::applyText);
    connect(bounds,&QPushButton::clicked,this,[this]{
        static bool visible=true; visible=!visible; m_preview->setShowBounds(visible);
    });
    connect(m_timeline,&QSlider::valueChanged,this,[this](int v){
        m_preview->setCurrentFrame(v);
        if (m_fps > 0) m_player->setPosition(qint64(v*1000.0/m_fps));
    });
}

void MainWindow::openVap() {
    const QString path = QFileDialog::getOpenFileName(this,"打开 VAP 文件",{}, "VAP/MP4 (*.vap *.mp4);;所有文件 (*.*)");
    if(path.isEmpty()) return;
    auto result = VapParser::parseFile(path);
    if(!result.ok) {
        QMessageBox::warning(this,"VAP 解析失败",result.error);
        m_log->append("[PARSE ERROR] "+result.error);
        return;
    }
    m_mediaPath = path;
    loadDocument(result.document.object(), path);
    m_player->setSource(QUrl::fromLocalFile(path));
    m_status->setText(QString("已载入：%1（vapc 偏移 %2）").arg(QFileInfo(path).fileName()).arg(result.boxOffset));
    m_log->append("[OK] vapc JSON 解析成功");
}

void MainWindow::openJson() {
    const QString path = QFileDialog::getOpenFileName(this,"打开 vapc JSON",{}, "JSON (*.json);;所有文件 (*.*)");
    if(path.isEmpty()) return;
    auto result = VapParser::parseJsonFile(path);
    if(!result.ok) { QMessageBox::warning(this,"JSON 解析失败",result.error); return; }
    m_mediaPath.clear();
    m_player->stop();
    loadDocument(result.document.object(), path);
    m_status->setText("已载入独立配置 JSON（预览使用占位符画布）");
    m_log->append("[OK] 独立 JSON 解析成功");
}

void MainWindow::loadDocument(const QJsonObject& config, const QString&) {
    m_config = config;
    const QJsonObject info = config.value("info").toObject();
    m_fps = info.value("fps").toDouble(20.0);
    m_totalFrames = info.value("f").toInt(0);
    m_timeline->blockSignals(true);
    m_timeline->setRange(0,qMax(0,m_totalFrames-1));
    m_timeline->setValue(0);
    m_timeline->blockSignals(false);
    m_preview->setConfig(m_config);
    m_preview->setCurrentFrame(0);
    populateSources();
    m_log->append(QString("[INFO] FPS=%1, frames=%2, canvas=%3x%4")
        .arg(m_fps).arg(m_totalFrames).arg(info.value("w").toInt()).arg(info.value("h").toInt()));
}

void MainWindow::populateSources() {
    const QJsonArray src = m_config.value("src").toArray();
    m_sources->setRowCount(src.size());
    for(int i=0;i<src.size();++i) {
        const auto o=src[i].toObject();
        const QString tag=normalizeTag(o.value("srcTag").toString());
        const QString id=o.value("srcId").toVariant().toString();
        const bool loaded=m_bindings.value(tag).isSet;
        m_sources->setItem(i,0,new QTableWidgetItem(o.value("srcType").toString()));
        m_sources->setItem(i,1,new QTableWidgetItem(id));
        m_sources->setItem(i,2,new QTableWidgetItem(o.value("srcTag").toString()));
        m_sources->setItem(i,3,new QTableWidgetItem(loaded?"已绑定":"未绑定"));
    }
    m_preview->setBindings(m_bindings);
}

QString MainWindow::normalizeTag(QString tag) const {
    tag=tag.trimmed();
    if(tag.startsWith('[')&&tag.endsWith(']')) tag=tag.mid(1,tag.size()-2);
    return tag;
}
QString MainWindow::currentTag() const {
    const int row=m_sources->currentRow();
    if(row<0) return {};
    return normalizeTag(m_sources->item(row,2)->text());
}
void MainWindow::selectSource() {
    const QString tag=currentTag();
    if(tag.isEmpty()) return;
    const auto asset=m_bindings.value(tag);
    m_text->setText(asset.text);
    m_log->append("[SELECT] "+tag);
}
void MainWindow::bindImage() {
    const int row=m_sources->currentRow();
    if(row<0) { QMessageBox::information(this,"提示","请先选择一个图片占位符。"); return; }
    if(m_sources->item(row,0)->text()!="img") { QMessageBox::information(this,"类型不匹配","当前资源不是图片类型。"); return; }
    const QString path=QFileDialog::getOpenFileName(this,"选择替换图片",{}, "图片 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if(path.isEmpty()) return;
    QImage img(path);
    if(img.isNull()) { QMessageBox::warning(this,"图片错误","图片无法解码。"); return; }
    const QString tag=currentTag();
    BoundAsset a; a.type="img"; a.image=img; a.isSet=true;
    m_bindings.insert(tag,a);
    m_preview->setBindings(m_bindings);
    populateSources();
    m_log->append(QString("[OK] %1 <- %2 (%3x%4)").arg(tag,path).arg(img.width()).arg(img.height()));
}
void MainWindow::applyText() {
    const int row=m_sources->currentRow();
    if(row<0) { QMessageBox::information(this,"提示","请先选择一个文字占位符。"); return; }
    if(m_sources->item(row,0)->text()!="txt") { QMessageBox::information(this,"类型不匹配","当前资源不是文字类型。"); return; }
    const QString tag=currentTag();
    BoundAsset a; a.type="txt"; a.text=m_text->text(); a.isSet=true;
    m_bindings.insert(tag,a);
    m_preview->setBindings(m_bindings);
    populateSources();
    m_log->append(QString("[OK] %1 <- \"%2\"").arg(tag,a.text));
}
void MainWindow::playPause() {
    if(m_mediaPath.isEmpty()) { QMessageBox::information(this,"提示","当前只有 JSON 配置，没有视频文件。"); return; }
    if(m_player->playbackState()==QMediaPlayer::PlayingState) m_player->pause();
    else m_player->play();
}
void MainWindow::onVideoFrame(const QVideoFrame& frame) {
    QVideoFrame f(frame);
    if(!f.map(QVideoFrame::ReadOnly)) return;
    QImage img=f.toImage();
    f.unmap();
    if(!img.isNull()) m_preview->setVideoFrame(img);
}
void MainWindow::onPosition(qint64 position) {
    const int frame=int(position*m_fps/1000.0);
    m_timeline->blockSignals(true);
    m_timeline->setValue(qBound(0,frame,m_timeline->maximum()));
    m_timeline->blockSignals(false);
    m_preview->setCurrentFrame(frame);
}
void MainWindow::checkConfig() {
    QStringList issues;
    const QJsonArray src=m_config.value("src").toArray();
    const QJsonArray frames=m_config.value("frame").toArray();
    QSet<QString> ids, referenced;
    for(const auto& v:src) {
        const auto o=v.toObject();
        const QString id=o.value("srcId").toVariant().toString();
        const QString tag=normalizeTag(o.value("srcTag").toString());
        if(ids.contains(id)) issues<<"重复 srcId："+id;
        ids.insert(id);
        if(tag.isEmpty()) issues<<"资源 "+id+" 缺少 srcTag";
        if(!m_bindings.value(tag).isSet) issues<<"占位符未绑定："+tag;
    }
    for(const auto& fv:frames) for(const auto& ov:fv.toObject().value("obj").toArray()) {
        const QString id=ov.toObject().value("srcId").toVariant().toString();
        referenced.insert(id);
        if(!ids.contains(id)) issues<<"frame 引用了不存在的 srcId："+id;
    }
    for(const auto& v:src) {
        const QString id=v.toObject().value("srcId").toVariant().toString();
        if(!referenced.contains(id)) issues<<"未被任何 frame 引用的资源："+id;
    }
    if(issues.isEmpty()) {
        m_log->append("[PASS] 配置引用检查通过；请继续目视检查位置、遮罩和渲染。");
        m_status->setText("基础检查通过");
    } else {
        m_log->append("[CHECK] 发现 "+QString::number(issues.size())+" 个问题：");
        for(const auto& s:issues) m_log->append("  - "+s);
        m_status->setText(QString("发现 %1 个配置/绑定问题").arg(issues.size()));
    }
}
