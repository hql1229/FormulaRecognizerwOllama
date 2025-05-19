#ifndef OLLAMACLIENT_H
#define OLLAMACLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QString>

class OllamaClient : public QObject
{
    Q_OBJECT
public:
    explicit OllamaClient(QObject *parent = nullptr);

    // 设置API URL
    void setOllamaUrl(const QString &url);
    // 设置模型名称
    void setModelName(const QString &modelName);

    // 更新API URL和模型名称
    void updateSettings(const QString &url, const QString &modelName);

    // 识别公式
    void recognizeFormula(const QPixmap &pixmap);

signals:
    void recognitionSuccess(const QString &markdownFormula);
    void recognitionError(const QString &errorString);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QString ollamaApiUrl;
    QString currentModelName;

    void sendRequest(const QString &base64Image);
};

#endif // OLLAMACLIENT_H
