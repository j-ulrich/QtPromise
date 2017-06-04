/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_NETWORKPROMISE_H_
#define QTPROMISE_NETWORKPROMISE_H_

#include "Promise.h"
#include "NetworkDeferred.h"

namespace QtPromise
{

/*! \brief A promise to a NetworkDeferred.
 *
 * \sa NetworkDeferred
 */
class NetworkPromise : public Promise
{
	Q_OBJECT

public:

	/*! Smart pointer to a NetworkPromise */
	typedef QSharedPointer<NetworkPromise> Ptr;

	/*! Creates a NetworkPromise for a QNetworkReply
	 *
	 * \param reply The QNetworkReply performing the transmission.
	 * \return A NetworkPromise to a new, pending NetworkDeferred for the given
	 * \p reply.
	 */
	static Ptr create(QNetworkReply* reply);
	static Ptr create(NetworkDeferred::Ptr deferred);

	NetworkDeferred::ReplyData replyData() const;
	NetworkDeferred::Error error() const;

signals:
	/*! \copydoc NetworkDeferred::resolved()
	 * \sa NetworkDeferred::resolved()
	 */
	void resolved(const NetworkDeferred::ReplyData& data) const;
	/*! \copydoc NetworkDeferred::rejected()
	 * \sa NetworkDeferred::rejected()
	 */
	void rejected(const NetworkDeferred::Error& reason) const;
	/*! \copydoc NetworkDeferred::notified()
	 * \sa NetworkDeferred::notified()
	 */
	void notified(const NetworkDeferred::ReplyProgress& progress) const;

protected:
	/*! Creates a NetworkDeferred for a QNetworkReply and
	 * then create a NetworkPromise for that new NetworkDeferred.
	 *
	 * \sa NetworkDeferred()
	 */
	NetworkPromise(QNetworkReply* reply);
	NetworkPromise(NetworkDeferred::Ptr deferred);
};

} /* namespace QtPromise */


#endif /* QTPROMISE_NETWORKPROMISE_H_ */
