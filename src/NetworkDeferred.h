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

/*! \brief Creates a Deferred for a QNetworkReply.
 *
 * A NetworkDeferred is resolved with a ReplyData object when the QNetworkReply
 * finishes without errors, meaning QNetworkReply::error() returns
 * QNetworkReply::NoError. Else, the deferred is rejected with an Error object.
 * Additionally, the deferred is notified with a ReplyProgress object whenever
 * there is download or upload progress.
 *
 * In most cases, it is not necessary to create a NetworkDeferred directly but instead
 * use the convenience method NetworkPromise::create(QNetworkReply*) which returns a
 * promise on a NetworkDeferred.
 * Creating a NetworkDeferred directly is only needed if the deferred should be
 * resolved/rejected/notified independently of the QNetworkReply, which should be
 * a very rare use case.
 *
 * \author jochen.ulrich
 *
 * \sa NetworkPromise
 */
class NetworkDeferred : public Deferred
{
	Q_OBJECT

public:
	/*! Smart pointer to NetworkDeferred. */
	typedef QSharedPointer<NetworkDeferred> Ptr;

	/*! The struct used to resolve a NetworkDeferred.
	 */
	struct ReplyData
	{
		/*! This is the data contained in the response.
		 */
		QByteArray data;
		/*! Pointer to the original QNetworkReply.
		 * Use this to get additional information about the reply like
		 * response headers, HTTP status code etc.
		 *
		 * \warning Do not delete the QNetworkReply. It is owned by the NetworkDeferred.
		 */
		const QNetworkReply* qReply;

		/*! Default constructor.
		 *
		 * \p data will be empty and \p qReply will be a \c nullptr.
		 */
		ReplyData() : qReply(nullptr) {}
		/*! Creates a ReplyData struct.
		 *
		 * \param data The data as read from the QNetworkReply using QIODevice::readAll().
		 * \param qReply The QNetworkReply itself.
		 */
		ReplyData(QByteArray data, const QNetworkReply* qReply) : data(data), qReply(qReply) {}

		/*! Compares two ReplyData objects for equality.
		 *
		 * \param other The ReplyData object to compare to.
		 * \return \c true if the \p data is equal and the \p qReply is identical for both ReplyData
		 * objects. \c false otherwise.
		 */
		bool operator==(const ReplyData& other) const
		{
			return data == other.data && qReply == other.qReply;
		}
	};

	/*! Represents the progress of a download or upload.
	 */
	struct Progress
	{
		/*! The number of bytes which have already been transferred. */
		qint64 current = -1;
		/*! The total number of bytes to be transferred. */
		qint64 total = -1;

		/*! Compares two Progress objects for equality.
		 *
		 * \param other The Progress object to compare to.
		 * \return \c true if both \p current and \p total are equal for \c this and \p other.
		 * \c false otherwise.
		 */
		bool operator==(const Progress& other) const
		{
			return current == other.current && total == other.total;
		}
	};

	/*! Represents the progress of a network communication.
	 *
	 * It includes both the download and the upload progress.
	 * If the QNetworkReply did not download or upload anything yet,
	 * the values of the corresponding Progress object will be \c -1.
	 */
	struct ReplyProgress
	{
		/*! The download progress. */
		Progress download;
		/*! The upload progress. */
		Progress upload;

		/*! Compares two ReplyProgress objects for equality.
		 *
		 * \param other The ReplyProgress object to compare to.
		 * \return \c true if both \p download and \p upload objects are equal.
		 * \c false otherwise.
		 *
		 * \sa Progress::operator==()
		 */
		bool operator==(const ReplyProgress& other) const
		{
			return download == other.download && upload == other.upload;
		}
	};

	/*! The struct used to reject a NetworkDeferred.
	 */
	struct Error
	{
		/*! The error code.
		 * This is identical to \c replyData.qReply->error() except for the case
		 * when the QNetworkReply is destroyed before it is finished.
		 * In that case, this will be \c -1.
		 */
		QNetworkReply::NetworkError code;
		/*! The error message.
		 * This is identical to \c replyData.qReply->errorString() except for the case
		 * when the QNetworkReply is destroyed before it is finished.
		 * In that case, this will be a "custom" error message.
		 */
		QString message;
		/*! Represents the state of the reply when the error occurred.
		 *
		 * The ReplyData::data contains the downloaded data at the time when the error occurred.
		 * This can be empty or contain the full response data in case of a server error
		 * response (e.g. in case of a QNetworkReply::ContentAccessDenied).
		 *
		 * ReplyData::qReply allows accessing more details (like the response headers).
		 * \warning ReplyData::qReply can be a \c nullptr (for example in case the QNetworkReply
		 * is destroyed before it is finished). So check the pointer before using it.
		 */
		ReplyData replyData;

		/*! Default constructor.
		 * Sets the \p code to QNetworkReply::NoError. The other members are default
		 * constructed.
		 */
		Error() : code(QNetworkReply::NoError) {}
		/*! Creates an Error object for a given ReplyData object.
		 *
		 * \param replyData The ReplyData object representing the state
		 * of the QNetworkReply at the time when the error occurred.
		 */
		Error(const ReplyData& replyData) : replyData(replyData)
		{
			if (replyData.qReply)
			{
				code = replyData.qReply->error();
				message = replyData.qReply->errorString();
			}
			else
			{
				code = -1;
			}

		}

		/*! Compares two Error objects for equality.
		 *
		 * \param other The Error object to compare to.
		 * \return \c true if the \p code, \p message and the \p replyData of both objects
		 * are equal. \c false otherwise.
		 *
		 * \sa ReplyData::operator==()
		 */
		bool operator==(const Error& other) const
		{
			return code == other.code && message == other.message && replyData == other.replyData;
		}
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
	void replyDestroyed();

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
