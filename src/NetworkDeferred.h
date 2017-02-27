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

	struct Progress
	{
		qint64 current;
		qint64 total;
	};

	struct NetworkReplyProgress
	{
		Progress download;
		Progress upload;
	};

	struct Error
	{
		QNetworkReply::NetworkError error;
		QString errorString;
	};

	static Ptr create(QNetworkReply* reply);


signals:
	void resolved(const QByteArray& data) const;
	void rejected(Error reason) const;
	void notified(NetworkReplyProgress progress) const;

protected slots:
	void replyFinished();
	void replyReadyRead();
	void replyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void replyUploadProgress(qint64 bytesSent, qint64 bytesTotal);

protected:
	NetworkDeferred(QNetworkReply* reply);

	QNetworkReply* m_reply;
	QByteArray m_buffer;
	NetworkReplyProgress m_progress;
};

}  // namespace QtPromise

Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::Progress)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::NetworkReplyProgress)
Q_DECLARE_METATYPE(QtPromise::NetworkDeferred::Error)


#endif /* SRC_NETWORKDEFERRED_H_ */
