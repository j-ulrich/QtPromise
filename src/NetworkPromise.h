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

/*! \brief
 *
 * Similar to QNetworkReply, NetworkPromise ensures that the resolved() or rejected() signals
 * are emitted
 */
class NetworkPromise : public Promise
{
	Q_OBJECT

public:

	typedef QSharedPointer<NetworkPromise> Ptr;

	static Ptr create(QNetworkReply* reply);

	NetworkDeferred::ReplyData replyData() const;
	NetworkDeferred::Error error() const;

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
