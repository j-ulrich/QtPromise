#include "NetworkDeferred.h"
#include <QTimer>

namespace QtPromise {

NetworkDeferred::NetworkDeferred(QNetworkReply* reply)
	: Deferred(), m_reply(reply), m_lock(QMutex::Recursive)
{
	registerMetaTypes();

	m_reply->setParent(this);

	/* In case the reply is already finished, we cannot rely on the
	 * QNetworkReply::finished() signal as it could have been fired already.
	 * Therefore, we call replyFinished() asynchronously. So no matter if
	 * QNetworkReply::finished() has already fired or not, the NetworkDeferred
	 * will be resolved/rejected when control returns to the event loop.
	 */
	if (reply->isFinished())
		QTimer::singleShot(0, this, &NetworkDeferred::replyFinished);
	else
		connect(m_reply, &QNetworkReply::finished, this, &NetworkDeferred::replyFinished);

	/* For the progress signals, we stay in line with QNetworkReply:
	 * if they have been fired before the caller connected slots to them,
	 * the caller cannot get progress notifications anymore.
	 *
	 * In case the reply is already finished but the progress signals are still in the
	 * event queue, the notifications will be triggered before the deferred is resolved/reject
	 * since the call to NetworkDeferred::replyFinished() comes later in the event queue.
	 * Hence, the notifications will work as expected.
	 */
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

void NetworkDeferred::replyFinished()
{
	QMutexLocker locker(&m_lock);
	// Save reply data since it will be removed from QNetworkReply when calling readAll()
	m_buffer = m_reply->readAll();
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
	QMutexLocker locker(&m_lock);
	m_progress.download.current = bytesReceived;
	m_progress.download.total = bytesTotal;
	if (this->notify(QVariant::fromValue(m_progress)))
		emit notified(m_progress);
}

void NetworkDeferred::replyUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
	QMutexLocker locker(&m_lock);
	m_progress.upload.current = bytesSent;
	m_progress.upload.total = bytesTotal;
	if (this->notify(QVariant::fromValue(m_progress)))
		emit notified(m_progress);
}



}  // namespace QtPromise
