#include "NetworkDeferred.h"

namespace QtPromise {

NetworkDeferred::NetworkDeferred(QNetworkReply* reply)
	: m_reply(reply)
{
	registerMetaTypes();

	m_reply->setParent(this);
	connect(m_reply, &QNetworkReply::readyRead, this, &NetworkDeferred::replyReadyRead);
	connect(m_reply, &QNetworkReply::finished, this, &NetworkDeferred::replyFinished);
	connect(m_reply, &QNetworkReply::downloadProgress, this, &NetworkDeferred::replyDownloadProgress);
	connect(m_reply, &QNetworkReply::uploadProgress, this, &NetworkDeferred::replyUploadProgress);
}

NetworkDeferred::Ptr NetworkDeferred::create(QNetworkReply* reply)
{
	return Ptr(new NetworkDeferred(reply), &QObject::deleteLater);
}

void NetworkDeferred::registerMetaTypes() const
{
	qRegisterMetaType<ReplyData>();
	qRegisterMetaType<ReplyData>("NetworkDeferred::ReplyData");
	qRegisterMetaType<ReplyData>("QtPromise::NetworkDeferred::ReplyData");
	qRegisterMetaType<Error>();
	qRegisterMetaType<Error>("NetworkDeferred::Error");
	qRegisterMetaType<Error>("QtPromise::NetworkDeferred::Error");
	qRegisterMetaType<Progress>();
	qRegisterMetaType<Progress>("NetworkDeferred::Progress");
	qRegisterMetaType<Progress>("QtPromise::NetworkDeferred::Progress");
	qRegisterMetaType<ReplyProgress>();
	qRegisterMetaType<ReplyProgress>("NetworkDeferred::ReplyProgress");
	qRegisterMetaType<ReplyProgress>("QtPromise::NetworkDeferred::ReplyProgress");

}

void NetworkDeferred::replyReadyRead()
{
	QWriteLocker locker(&m_lock);
	m_buffer += m_reply->readAll();
}

void NetworkDeferred::replyFinished()
{
	QWriteLocker locker(&m_lock);
	if (m_reply->error() != QNetworkReply::NoError)
	{
		Error reason;
		reason.code = m_reply->error();
		reason.message = m_reply->errorString();
		if (this->reject(QVariant::fromValue(reason)))
			emit rejected(reason);
	}
	else
	{
		ReplyData data;
		data.data = m_buffer;
		data.headers = m_reply->rawHeaderPairs();
		if (this->resolve(QVariant::fromValue(data)))
			emit resolved(data);
	}
}

void NetworkDeferred::replyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QWriteLocker locker(&m_lock);
	m_progress.download.current = bytesReceived;
	m_progress.download.total = bytesTotal;
	if (this->notify(QVariant::fromValue(m_progress)))
		emit notified(m_progress);
}

void NetworkDeferred::replyUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
	QWriteLocker locker(&m_lock);
	m_progress.upload.current = bytesSent;
	m_progress.upload.total = bytesTotal;
	if (this->notify(QVariant::fromValue(m_progress)))
		emit notified(m_progress);
}



}  // namespace QtPromise
