
#include <QtTest>
#include <QtDebug>
#include <QSignalSpy>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include "NetworkPromise.h"


namespace QtPromise
{
namespace Tests
{

/*! \brief Unit tests for the NetworkPromise class.
 *
 * \author jochen.ulrich
 */
class NetworkPromiseTest : public QObject
{
	Q_OBJECT

private slots:
	void testSuccess();
	void testFail();
	void testHttp();
	void testFinishedReply_data();
	void testFinishedReply();
	void testDestroyReply();
	void testCachedData();

private:
	struct PromiseSpies
	{
		PromiseSpies(NetworkPromise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy baseResolved;
		QSignalSpy rejected;
		QSignalSpy baseRejected;
		QSignalSpy notified;
		QSignalSpy baseNotified;
	};
};


//####### Helpers #######
NetworkPromiseTest::PromiseSpies::PromiseSpies(NetworkPromise::Ptr promise)
	: resolved(promise.data(), &NetworkPromise::resolved),
	  rejected(promise.data(), &NetworkPromise::rejected),
	  notified(promise.data(), &NetworkPromise::notified),
	  baseResolved(promise.data(), &Promise::resolved),
	  baseRejected(promise.data(), &Promise::rejected),
	  baseNotified(promise.data(), &Promise::notified)
{
}

//####### Tests #######
/*! \test Tests a successful request with a NetworkPromise.
 */
void NetworkPromiseTest::testSuccess()
{
	QString dataPath = QFINDTESTDATA("data/DummyData.txt");
	QFile dataFile(dataPath);
	dataFile.open(QIODevice::ReadOnly);
	QByteArray expectedData = dataFile.readAll();
	dataFile.close();

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	QVERIFY(spies.resolved.wait());
	QCOMPARE(spies.resolved.count(), 1);
	NetworkDeferred::ReplyData replyData = spies.resolved.first().first().value<NetworkDeferred::ReplyData>();
	QCOMPARE(replyData.data, expectedData);
	QCOMPARE(spies.baseResolved.count(), 1);
	QCOMPARE(spies.baseResolved.first().first().value<NetworkDeferred::ReplyData>(), replyData);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.baseRejected.count(), 0);
	QVERIFY(spies.notified.count() > 0);
	NetworkDeferred::ReplyProgress progress = spies.notified.first().first().value<NetworkDeferred::ReplyProgress>();
	QVERIFY(progress.download.current > 0);
	QVERIFY(progress.download.total > 0);
	QVERIFY(spies.baseNotified.count() > 0);
	QCOMPARE(spies.baseNotified.first().first(), QVariant::fromValue(progress));
}

/*! \test Tests a failed request with a NetworkPromise.
 */
void NetworkPromiseTest::testFail()
{
	QString dataPath("A_File_that_doesnt_exist_9831874375377535764532134848337483.txt");

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	QVERIFY(spies.rejected.wait());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.baseResolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	NetworkDeferred::Error error = spies.rejected.first().first().value<NetworkDeferred::Error>();
	QCOMPARE(error.code, QNetworkReply::ContentNotFoundError);
	qDebug() << "Error code:" << error.code;
	qDebug() << "Error message:" << error.message;
	QVERIFY(!error.message.isEmpty());
	QCOMPARE(spies.baseRejected.count(), 1);
	QCOMPARE(spies.baseRejected.first().first().value<NetworkDeferred::Error>(), error);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.baseNotified.count(), 0);
}

/*! \test Tests a HTTP request with a NetworkPromise.
 */
void NetworkPromiseTest::testHttp()
{
	QNetworkAccessManager qnam;
	if(qnam.networkAccessible() == QNetworkAccessManager::NotAccessible)
		QSKIP("Network not accessible");

	QNetworkRequest request(QUrl("http://www.google.com"));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);
	spies.resolved.wait();
	if (reply->error() == QNetworkReply::NoError)
		QCOMPARE(spies.resolved.count(), 1);
	else
		QCOMPARE(spies.rejected.count(), 1);
}

/*! Provides the data for the testFinishedReply() test.
 */
void NetworkPromiseTest::testFinishedReply_data()
{
	QTest::addColumn<QString>("dataPath");
	QTest::addColumn<bool>("expectResolve");

	QTest::newRow("success") << QFINDTESTDATA("data/DummyData.txt") << true;
	QTest::newRow("fail") << "Non_existent_File_45389045765.txt" << false;
}

/*! \test Tests the NetworkPromise with a QNetworkReply which has finished
 * and emitted it's events before the NetworkPromise is created.
 */
void NetworkPromiseTest::testFinishedReply()
{
	QFETCH(QString, dataPath);
	QFETCH(bool, expectResolve);

	QByteArray expectedData;
	QFile dataFile(dataPath);
	if (dataFile.open(QIODevice::ReadOnly))
	{
		expectedData = dataFile.readAll();
		dataFile.close();
	}

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	QCoreApplication::processEvents(QEventLoop::AllEvents, 500);
	QVERIFY(reply->isFinished());

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	bool resolvedCalled = false;
	QVariant resolvedData;
	Promise::Ptr newPromise = promise->then([&](const QVariant& data) {
		resolvedCalled = true;
		resolvedData = data;
	});

	if (expectResolve)
		QVERIFY(spies.resolved.wait(500));
	else
		QVERIFY(spies.rejected.wait(500));
	QCOMPARE(spies.resolved.count(), expectResolve? 1 : 0);
	QCOMPARE(spies.baseResolved.count(), expectResolve? 1 : 0);
	QCOMPARE(spies.rejected.count(), expectResolve? 0 : 1);
	QCOMPARE(spies.baseRejected.count(), expectResolve? 0 : 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.baseNotified.count(), 0);
	QCOMPARE(resolvedCalled, expectResolve);
	QCOMPARE(resolvedData.value<NetworkDeferred::ReplyData>().data, expectedData);
}

/*! \test Tests destroying a QNetworkReply while it is attached to a NetworkPromise.
 */
void NetworkPromiseTest::testDestroyReply()
{
	QNetworkAccessManager qnam;
	if(qnam.networkAccessible() == QNetworkAccessManager::NotAccessible)
		QSKIP("Network not accessible");

	QNetworkRequest request(QUrl("http://www.google.com"));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	QCOMPARE(promise->state(), Deferred::Pending);

	delete reply;

	QCOMPARE(promise->state(), Deferred::Rejected);
}

/*! \test Tests the NetworkPromise with cached data.
 */
void NetworkPromiseTest::testCachedData()
{
	QNetworkAccessManager qnam;
	QNetworkDiskCache diskCache(&qnam);
	diskCache.setCacheDirectory(QDir(qApp->applicationDirPath()).filePath("cache"));
	qnam.setCache(&diskCache);
	
	// Load the data for the first time to cache it
	QString dataPath = QFINDTESTDATA("data/DummyData.txt");
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* firstReply = qnam.get(request);
	QTRY_VERIFY(firstReply->isFinished());
	
	// Load the data for the second time.
	QNetworkReply* secondReply = qnam.get(request);
	NetworkPromise::Ptr promise = NetworkPromise::create(secondReply);
	QTRY_VERIFY(secondReply->isFinished());
	QTRY_COMPARE(promise->state(), Deferred::Resolved);
}


}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::NetworkPromiseTest)
#include "NetworkPromiseTest.moc"


