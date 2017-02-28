/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef SRC_NETWORKDEFERRED_H_
#define SRC_NETWORKDEFERRED_H_

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
		QList<QNetworkReply::RawHeaderPair> headers;

		bool operator==(const ReplyData& other) const { return data == other.data && headers == other.headers; }
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
		QNetworkReply::NetworkError code = QNetworkReply::NoError;
		QString message;

		bool operator==(const Error& other) const { return code == other.code && message == other.message; }
	};

	static Ptr create(QNetworkReply* reply);

	QByteArray data() const { QReadLocker locker(&m_lock); return m_buffer; }
	QList<QNetworkReply::RawHeaderPair> headers() const { QReadLocker lokcer(&m_lock); return m_reply->rawHeaderPairs(); }

signals:
	void resolved(const QtPromise::NetworkDeferred::ReplyData& data) const;
	void rejected(const QtPromise::NetworkDeferred::Error& reason) const;
	void notified(const QtPromise::NetworkDeferred::ReplyProgress& progress) const;

protected slots:
	void replyFinished();
	void replyReadyRead();
	void replyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void replyUploadProgress(qint64 bytesSent, qint64 bytesTotal);

protected:
	NetworkDeferred(QNetworkReply* reply);

	QNetworkReply* m_reply;
	QByteArray m_buffer;
	ReplyProgress m_progress;

private:
	void registerMetaTypes() const;
};

}  // namespace QtPromise

Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::ReplyData)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::Progress)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::ReplyProgress)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::Error)


#endif /* SRC_NETWORKDEFERRED_H_ */
