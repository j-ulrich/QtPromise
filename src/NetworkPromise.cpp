#include "NetworkPromise.h"

namespace QtPromise
{

NetworkPromise::NetworkPromise(QNetworkReply* reply, QObject* parent = nullptr)
	: Promise(NetworkDeferred::create(reply), parent)
{

}

} /* namespace QtPromise */
