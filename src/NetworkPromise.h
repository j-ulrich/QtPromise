/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef SRC_NETWORKPROMISE_H_
#define SRC_NETWORKPROMISE_H_

#include "Promise.h"
#include "NetworkDeferred.h"

namespace QtPromise
{

class NetworkPromise : public Promise
{
	Q_OBJECT

public:
	/*!
	 *
	 * \param reply The promise chain takes ownership of the \p reply.
	 * \param parent QObject parent.
	 */
	NetworkPromise(QNetworkReply* reply, QObject* parent = nullptr);


signals:
	void rejected(NetworkDeferred::Error reason) const;
	void notified(NetworkDeferred::NetworkReplyProgress progress) const;

};

} /* namespace QtPromise */


#endif /* SRC_NETWORKPROMISE_H_ */
