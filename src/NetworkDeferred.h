/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_NETWORKDEFERRED_H_
#define QTPROMISE_NETWORKDEFERRED_H_

#include <QNetworkReply>
#include "Deferred.h"


namespace QtPromise {

class NetworkDeferred : public Deferred
{
	Q_OBJECT

public:

	typedef QSharedPointer<NetworkDeferred> Ptr;

	struct ReplyData
	{
		QByteArray data;
		const QNetworkReply* qReply;

		ReplyData() : qReply(nullptr) {}
		ReplyData(QByteArray data, const QNetworkReply* qReply) : data(data), qReply(qReply) {}

		bool operator==(const ReplyData& other) const { return data == other.data && qReply == other.qReply; }
	};

	struct Progress
	{
		qint64 current = -1;
		qint64 total = -1;

		bool operator==(const Progress& other) const { return current == other.current && total == other.total; }
	};

	struct ReplyProgress
	{
		Progress download;
		Progress upload;

		bool operator==(const ReplyProgress& other) const { return download == other.download && upload == other.upload; }
	};

	struct Error
	{
		QNetworkReply::NetworkError code;
		QString message;
		ReplyData replyData;

		Error() : code(QNetworkReply::NoError) {}
		Error(const ReplyData& replyData) : code(replyData.qReply->error()), message(replyData.qReply->errorString()), replyData(replyData) {}

		bool operator==(const Error& other) const { return code == other.code && message == other.message; }
	};

	static Ptr create(QNetworkReply* reply);

	ReplyData replyData() const { QMutexLocker locker(&m_lock); return ReplyData(m_buffer, m_reply); }
	Error error() const { QMutexLocker locker(&m_lock); return Error(ReplyData(m_buffer, m_reply)); }

signals:
	void resolved(const QtPromise::NetworkDeferred::ReplyData& data) const;
	void rejected(const QtPromise::NetworkDeferred::Error& reason) const;
	void notified(const QtPromise::NetworkDeferred::ReplyProgress& progress) const;

protected:
	NetworkDeferred(QNetworkReply* reply);

private slots:
	void replyFinished();
	void replyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void replyUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
	mutable QMutex m_lock;
	QNetworkReply* m_reply;
	QByteArray m_buffer;
	ReplyProgress m_progress;

	void registerMetaTypes() const;
};

}  // namespace QtPromise

Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::ReplyData)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::Progress)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::ReplyProgress)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::Error)


#endif /* QTPROMISE_NETWORKDEFERRED_H_ */
