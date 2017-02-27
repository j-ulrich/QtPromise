#include "NetworkPromise.h"

namespace QtPromise
{

NetworkPromise::NetworkPromise(QNetworkReply* reply)
	: Promise(NetworkDeferred::create(reply))
{
	NetworkDeferred::Ptr deferred = m_deferred.staticCast<NetworkDeferred>();
	connect(deferred.data(), &NetworkDeferred::resolved, this, &NetworkPromise::resolved);
	connect(deferred.data(), &NetworkDeferred::rejected, this, &NetworkPromise::rejected);
	connect(deferred.data(), &NetworkDeferred::notified, this, &NetworkPromise::notified);

}

NetworkPromise::Ptr NetworkPromise::create(QNetworkReply* reply)
{
	return Ptr(new NetworkPromise(reply), &QObject::deleteLater);
}

void NetworkPromise::reemitSignals() const
{
	Promise::reemitSignals();
	switch (state())
	{
	case Deferred::Resolved:
	{
		NetworkDeferred::Ptr deferred = m_deferred.staticCast<NetworkDeferred>();
		NetworkDeferred::ReplyData data;
		data.data = deferred->data();
		data.headers = deferred->headers();
		emit resolved(data);
		break;
	}
	case Deferred::Rejected:
	{
		emit rejected(m_deferred->data().value<NetworkDeferred::Error>());
		break;
	}
	default:
	case Deferred::Pending:
		break;
	}
}

} /* namespace QtPromise */
