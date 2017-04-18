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

signals:
	void resolved(const NetworkDeferred::ReplyData& data) const;
	void rejected(const NetworkDeferred::Error& reason) const;
	void notified(const NetworkDeferred::ReplyProgress& progress) const;

protected:
	/*!
	 *
	 * \param reply The promise chain takes ownership of the \p reply.
	 * \param parent QObject parent.
	 */
	NetworkPromise(QNetworkReply* reply);
};

} /* namespace QtPromise */


#endif /* QTPROMISE_NETWORKPROMISE_H_ */
