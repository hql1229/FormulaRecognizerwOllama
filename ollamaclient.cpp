#include "ollamaclient.h"
#include <QBuffer>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QMimeDatabase> // For guessing MIME type, though PNG is good.

OllamaClient::OllamaClient(QObject *parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this))
{
    // Default values
    ollamaApiUrl = "http://localhost:11434/api/generate";
    currentModelName = "qwen2.5vl:7b";
}

void OllamaClient::setOllamaUrl(const QString &url) {
    ollamaApiUrl = url;
    qDebug() << "ollamaApiUrl:"<< ollamaApiUrl;
}

void OllamaClient::setModelName(const QString &modelName) {
    currentModelName = modelName;
    qDebug() << "currentModelName:"<< currentModelName;
}

void OllamaClient::updateSettings(const QString &url, const QString &modelName) {
    setOllamaUrl(url);
    setModelName(modelName);
}

void OllamaClient::recognizeFormula(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        emit recognitionError("Input image is empty.");
        return;
    }

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    if (!pixmap.save(&buffer, "PNG")) { // Save pixmap as PNG into byte array
        emit recognitionError("Failed to convert QPixmap to PNG byte array.");
        return;
    }
    QString base64Image = QString::fromLatin1(byteArray.toBase64().data());

    QJsonObject jsonPayload;
    jsonPayload["model"] = currentModelName;
    // IMPORTANT: Adjust the prompt to get Markdown.
    // This prompt is a suggestion. You might need to experiment for best results.
    jsonPayload["prompt"] = "focusing on any mathematical formulas in this image. Present the formulas in Markdown format (e.g., $...$ for inline, $$...$$ for display). output formulas only";
    jsonPayload["stream"] = false; // Get response in one go

    QJsonArray imagesArray;
    imagesArray.append(base64Image);
    jsonPayload["images"] = imagesArray;

    QJsonDocument doc(jsonPayload);
    QByteArray jsonData = doc.toJson();

    QNetworkRequest request;
    request.setUrl(QUrl(ollamaApiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void OllamaClient::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("response")) {
            QString formula = jsonObj["response"].toString();
            emit recognitionSuccess(formula);
        } else if (jsonObj.contains("error")) {
            emit recognitionError("Ollama API Error: " + jsonObj["error"].toString());
        }
        else {
            emit recognitionError("Failed to parse Ollama response or 'response' field missing. Response: " + QString(responseData));
        }
    } else {
        emit recognitionError("Network Error: " + reply->errorString() + " | Details: " + reply->readAll());
    }
    reply->deleteLater();
}
