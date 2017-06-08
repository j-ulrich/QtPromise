
#include <QtTest>
#include <stdexcept>
#include <string>
#include "Promise.h"

Q_DECLARE_METATYPE(QList<QtPromise::Deferred::Ptr>)
Q_DECLARE_METATYPE(QList<int>)

namespace QtPromise
{
namespace Tests
{

const QString ACTION_RESOLVE = "resolve";
const QString ACTION_REJECT = "reject";
const QString ACTION_NOTIFY = "notify";

/*! Unit tests for the Promise class.
 *
 * \author jochen.ulrich
 */
class PromiseTest : public QObject
{
	Q_OBJECT

private slots:
	void testConstructor();
	void testConstructorWithResolvedDeferred();
	void testConstructorWithRejectedDeferred();
	void testCreateResolvedPromise();
	void testCreateRejectedPromise();
	void testResolve();
	void testReject();
	void testNotify();
	void testThenVoidCallback_data();
	void testThenVoidCallback();
	void testThenVariantCallback_data();
	void testThenVariantCallback();
	void testThenPromiseCallback_data();
	void testThenPromiseCallback();
	void testAlways_data();
	void testAlways();
	void testThreeLevelChain();
	void testAsyncChain();
	void testAll();
	void testAllReject();
	void testAny();
	void testAnyReject();
	void testAllAnySync_data();
	void testAllAnySync();
	void testAllAnyInitializerList();
	void cleanup();

private:
	struct PromiseSpies
	{
		PromiseSpies(Promise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy rejected;
		QSignalSpy notified;
	};

	void callActionOnDeferred(Deferred::Ptr& deferred, const QString& action, const QVariant& data, int repetitions);

};


//####### Helper #######

PromiseTest::PromiseSpies::PromiseSpies(Promise::Ptr promise)
	: resolved(promise.data(), &Promise::resolved),
	  rejected(promise.data(), &Promise::rejected),
	  notified(promise.data(), &Promise::notified)
{
}

/*! Helper method which calls an action on a Deferred.
 *
 * \param deferred The Deferred on which the action should be executed.
 * \param action The action to be executed.
 * \param data The data to be passed to the action.
 * \param repetitions The number of times the action should be called.
 */
void PromiseTest::callActionOnDeferred(Deferred::Ptr& deferred, const QString& action, const QVariant& data, int repetitions)
{
	for (int i=0; i < repetitions; ++i)
	{
		if (action == ACTION_RESOLVE)
			deferred->resolve(data);
		else if (action == ACTION_REJECT)
			deferred->reject(data);
		else if (action == ACTION_NOTIFY)
			deferred->notify(data);
		else
			throw std::invalid_argument(std::string("Unknown action: ") + action.toStdString());
	}
}

void PromiseTest::cleanup()
{
	// Let deleteLater be executed to clean up
	QTest::qWait(100);
}

//####### Tests #######

/*! \test Tests the Promise::create() method.
 */
void PromiseTest::testConstructor()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	QCOMPARE(promise->state(), Deferred::Pending);
	QVERIFY(promise->data().isNull());
}

/*! \test Tests the Promise::create() method with
 * an already resolved Deferred.
 */
void PromiseTest::testConstructorWithResolvedDeferred()
{
	Deferred::Ptr deferred = Deferred::create();
	QString value("string");
	deferred->resolve(value);
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QCOMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(promise->data().toString(), value);

	QVERIFY(spies.resolved.wait());
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
}

/*! \test Tests the Promise::create() method with
 * an already rejected Deferred.
 */
void PromiseTest::testConstructorWithRejectedDeferred()
{
	Deferred::Ptr deferred = Deferred::create();
	QString value("string");
	deferred->reject(value);
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->data().toString(), value);

	QVERIFY(spies.rejected.wait());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
}

/*! \test Tests the Promise::createResolved() method.
 */
void PromiseTest::testCreateResolvedPromise()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createResolved(myString);

	QCOMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(promise->data().toString(), myString);
}

/*! \test Tests the Promise::createRejected() method.
 */
void PromiseTest::testCreateRejectedPromise()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createRejected(myString);

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->data().toString(), myString);
}

/*! \test Tests resolving a Promise.
 */
void PromiseTest::testResolve()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QString value("myString");
	deferred->resolve(value);

	QCOMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(promise->data().toString(), value);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.resolved.first().first().toString(), value);
}

/*! \test Tests rejecting a Promise.
 */
void PromiseTest::testReject()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QString value("myString");
	deferred->reject(value);

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->data().toString(), value);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.rejected.first().first().toString(), value);
}

/*! \test Tests notifying a Promise.
 */
void PromiseTest::testNotify()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QString firstValue("myString");
	deferred->notify(firstValue);

	QCOMPARE(promise->state(), Deferred::Pending);
	QVERIFY(promise->data().isNull());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 1);
	QCOMPARE(spies.notified.at(0).first().toString(), firstValue);

	int secondValue(7);
	deferred->notify(secondValue);

	QCOMPARE(promise->state(), Deferred::Pending);
	QVERIFY(promise->data().isNull());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 2);
	QCOMPARE(spies.notified.at(1).first().toInt(), secondValue);
}

/*! Provides the data for the testThenVoidCallback() test.
 */
void PromiseTest::testThenVoidCallback_data()
{
	QTest::addColumn<bool>("async");
	QTest::addColumn<QString>("action");
	QTest::addColumn<QVariant>("data");
	QTest::addColumn<Deferred::State>("expectedState");
	QTest::addColumn<QVariant>("expectedData");
	QTest::addColumn<QVariantList>("expectedResolvedCalls");
	QTest::addColumn<QVariantList>("expectedRejectedCalls");
	QTest::addColumn<QVariantList>("expectedNotifiedCalls");

	QVariant data("my string value");
	//                              // async // action         // data // expectedState      // expectedData // expectedResolvedCalls    // expectedRejectedCalls    // expectedNotifiedCalls
	QTest::newRow("sync resolve")  << false  << ACTION_RESOLVE << data << Deferred::Resolved << data         << (QVariantList() << data) << (QVariantList())         << (QVariantList());
	QTest::newRow("sync reject")   << false  << ACTION_REJECT  << data << Deferred::Rejected << data         << (QVariantList())         << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync notify")   << false  << ACTION_NOTIFY  << data << Deferred::Pending  << QVariant()   << (QVariantList())         << (QVariantList())         << (QVariantList());
	QTest::newRow("async resolve") << true   << ACTION_RESOLVE << data << Deferred::Resolved << data         << (QVariantList() << data) << (QVariantList())         << (QVariantList());
	QTest::newRow("async reject")  << true   << ACTION_REJECT  << data << Deferred::Rejected << data         << (QVariantList())         << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async notify")  << true   << ACTION_NOTIFY  << data << Deferred::Pending  << QVariant()   << (QVariantList())         << (QVariantList())         << (QVariantList() << data << data);
}

/*! \test Tests the Promise::then() method with a callback returning void.
 */
void PromiseTest::testThenVoidCallback()
{
	QFETCH(bool, async);
	QFETCH(QString, action);
	QFETCH(QVariant, data);


	Deferred::Ptr deferred = Deferred::create();

	if (!async)
		callActionOnDeferred(deferred, action, data, 2);

	Promise::Ptr promise = Promise::create(deferred);

	QVariantList resolvedCalls;
	QVariantList rejectedCalls;
	QVariantList notifiedCalls;

	Promise::Ptr newPromise = promise->then([&resolvedCalls](const QVariant& value) {
		resolvedCalls.push_back(value);
	}, [&rejectedCalls](const QVariant& reason) {
		rejectedCalls.push_back(reason);
	}, [&notifiedCalls](const QVariant& progress) {
		notifiedCalls.push_back(progress);
	});

	QVariantList chainedNotifiedCalls;

	newPromise->then(noop, noop, [&chainedNotifiedCalls] (const QVariant& progress) {
		chainedNotifiedCalls.push_back(progress);
	});

	if (async)
	{
		QCOMPARE(promise->state(), Deferred::Pending);
		QCOMPARE(newPromise->state(), Deferred::Pending);
		QCOMPARE(resolvedCalls.count(), 0);
		QCOMPARE(rejectedCalls.count(), 0);
		QCOMPARE(notifiedCalls.count(), 0);

		callActionOnDeferred(deferred, action, data, 2);
	}

	QTEST(promise->state(), "expectedState");
	QTEST(newPromise->state(), "expectedState");
	QTEST(promise->data(), "expectedData");
	QTEST(newPromise->data(), "expectedData");
	QTEST(resolvedCalls, "expectedResolvedCalls");
	QTEST(rejectedCalls, "expectedRejectedCalls");
	QTEST(notifiedCalls, "expectedNotifiedCalls");
	QVERIFY(chainedNotifiedCalls.isEmpty());
}

/*! Provides the data for the testThenVariantCallback() test.
 */
void PromiseTest::testThenVariantCallback_data()
{
	QTest::addColumn<bool>("async");
	QTest::addColumn<QString>("action");
	QTest::addColumn<QVariant>("originalData");
	QTest::addColumn<QVariant>("chainedData");
	QTest::addColumn<Deferred::State>("expectedOriginalState");
	QTest::addColumn<QVariantList>("expectedOriginalNotifiedCalls");
	QTest::addColumn<Deferred::State>("expectedChainedState");
	QTest::addColumn<QVariant>("expectedChainedData");
	QTest::addColumn<QVariantList>("expectedResolvedCalls");
	QTest::addColumn<QVariantList>("expectedRejectedCalls");
	QTest::addColumn<QVariantList>("expectedNotifiedCalls");


	QVariant originalData(42);
	QVariant chainedData("my string value");
	//                             // async // action         // originalData // chainedData // expectedOriginalState // expectedOriginalNotifiedCalls                    // expectedChainedState // expectedChainedData // expectedResolvedCalls           // expectedRejectedCalls // expectedNotifiedCalls
	QTest::newRow("sync resolve")  << false << ACTION_RESOLVE << originalData << chainedData << Deferred::Resolved    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("sync reject")   << false << ACTION_REJECT  << originalData << chainedData << Deferred::Rejected    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("sync notify")   << false << ACTION_NOTIFY  << originalData << chainedData << Deferred::Pending     << (QVariantList())                                 << Deferred::Pending    << QVariant()          << (QVariantList())                << (QVariantList())      << (QVariantList());
	QTest::newRow("async resolve") << true  << ACTION_RESOLVE << originalData << chainedData << Deferred::Resolved    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("async reject")  << true  << ACTION_REJECT  << originalData << chainedData << Deferred::Rejected    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("async notify")  << true  << ACTION_NOTIFY  << originalData << chainedData << Deferred::Pending     << (QVariantList() << originalData << originalData) << Deferred::Pending    << QVariant()          << (QVariantList())                << (QVariantList())      << (QVariantList() << chainedData << chainedData);
}

/*! \test Tests the Promise::then() method with a callback returning QVariant.
 */
void PromiseTest::testThenVariantCallback()
{
	QFETCH(bool, async);
	QFETCH(QString, action);
	QFETCH(QVariant, originalData);
	QFETCH(QVariant, chainedData);


	Deferred::Ptr deferred = Deferred::create();

	if (!async)
		callActionOnDeferred(deferred, action, originalData, 2);

	Promise::Ptr promise = Promise::create(deferred);

	QVariantList originalNotifiedCalls;

	Promise::Ptr newPromise = promise->then([&](const QVariant&) -> QVariant {
		return chainedData;
	}, [&](const QVariant&) -> QVariant {
		return chainedData;
	}, [&](const QVariant& progress) -> QVariant {
		originalNotifiedCalls.push_back(progress);
		return chainedData;
	});

	QVariantList resolvedCalls;
	QVariantList rejectedCalls;
	QVariantList notifiedCalls;

	newPromise->then([&](const QVariant& value) {
		resolvedCalls.push_back(value);
	}, [&](const QVariant& reason) {
		rejectedCalls.push_back(reason);
	}, [&](const QVariant& progress) {
		notifiedCalls.push_back(progress);
	});

	if (async)
	{
		QCOMPARE(promise->state(), Deferred::Pending);
		QCOMPARE(newPromise->state(), Deferred::Pending);

		callActionOnDeferred(deferred, action, originalData, 2);
	}

	QTEST(promise->state(), "expectedOriginalState");
	QTEST(originalNotifiedCalls, "expectedOriginalNotifiedCalls");
	QTEST(newPromise->state(), "expectedChainedState");
	QTEST(newPromise->data(), "expectedChainedData");
	QTEST(resolvedCalls, "expectedResolvedCalls");
	QTEST(rejectedCalls, "expectedRejectedCalls");
	QTEST(notifiedCalls, "expectedNotifiedCalls");
}

/*! Provides the data for the testThenPromiseCallback() test.
 */
void PromiseTest::testThenPromiseCallback_data()
{
	QTest::addColumn<bool>("async");
	QTest::addColumn<QString>("action");
	QTest::addColumn<QString>("callbackAction");
	QTest::addColumn<QVariant>("callbackData");
	QTest::addColumn<Deferred::State>("expectedOriginalState");
	QTest::addColumn<Deferred::State>("expectedChainedState");
	QTest::addColumn<QVariant>("expectedChainedData");
	QTest::addColumn<QVariantList>("expectedResolvedCalls");
	QTest::addColumn<QVariantList>("expectedRejectedCalls");


	QVariant data("my string value");
	//                                        // async // action         // callbackAction      // callbackData // expectedOriginalState // expectedChainedState expectedChainedData // expectedResolvedCalls    // expectedRejectedCalls
	QTest::newRow("sync resolve -> reject")   << false << ACTION_RESOLVE << ACTION_REJECT       << data         << Deferred::Resolved    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("sync resolve -> resolve")  << false << ACTION_RESOLVE << ACTION_RESOLVE      << data         << Deferred::Resolved    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync reject -> resolve")   << false << ACTION_REJECT  << ACTION_RESOLVE      << data         << Deferred::Rejected    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync reject -> reject")    << false << ACTION_REJECT  << ACTION_REJECT       << data         << Deferred::Rejected    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("async resolve -> reject")  << true  << ACTION_RESOLVE << ACTION_REJECT       << data         << Deferred::Resolved    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("async resolve -> resolve") << true  << ACTION_RESOLVE << ACTION_RESOLVE      << data         << Deferred::Resolved    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async reject -> resolve")  << true  << ACTION_REJECT  << ACTION_RESOLVE      << data         << Deferred::Rejected    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async reject -> reject")   << true  << ACTION_REJECT  << ACTION_REJECT       << data         << Deferred::Rejected    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
}

/*! \test Tests the Promise::then() method with a callback returning Promise::Ptr.
 */
void PromiseTest::testThenPromiseCallback()
{
	QFETCH(bool, async);
	QFETCH(QString, action);
	QFETCH(QString, callbackAction);
	QFETCH(QVariant, callbackData);


	Deferred::Ptr deferred = Deferred::create();
	QVariant deferredData("initial data");

	if (!async)
		callActionOnDeferred(deferred, action, deferredData, 1);

	Promise::Ptr promise = Promise::create(deferred);

	auto callback = [&](const QVariant&) -> Promise::Ptr {
		if (callbackAction == ACTION_RESOLVE)
			return Promise::createResolved(callbackData);
		else if (callbackAction == ACTION_REJECT)
			return Promise::createRejected(callbackData);
		else
		{
			QTest::qFail("Invalid action", __FILE__, __LINE__);
			return Promise::createRejected(QVariant());
		}
	};

	Promise::Ptr newPromise = promise->then(callback, callback);

	QVariantList resolvedCalls;
	QVariantList rejectedCalls;

	newPromise->then([&](const QVariant& value) {
		resolvedCalls.push_back(value);
	}, [&](const QVariant& reason) {
		rejectedCalls.push_back(reason);
	});

	if (async)
	{
		QCOMPARE(promise->state(), Deferred::Pending);
		QCOMPARE(newPromise->state(), Deferred::Pending);

		callActionOnDeferred(deferred, action, deferredData, 1);
	}

	QTEST(promise->state(), "expectedOriginalState");
	QTEST(newPromise->state(), "expectedChainedState");
	QTEST(newPromise->data(), "expectedChainedData");
	QTEST(resolvedCalls, "expectedResolvedCalls");
	QTEST(rejectedCalls, "expectedRejectedCalls");
}

/*! Provides the data for the testAlways() test.
 */
void PromiseTest::testAlways_data()
{
	QTest::addColumn<QString>("action");

	//                       // action
	QTest::newRow("resolve") << ACTION_RESOLVE;
	QTest::newRow("reject")  << ACTION_REJECT;
}

/*! \test Tests the Promise::always() method.
 */
void PromiseTest::testAlways()
{
	QFETCH(QString, action);

	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	QVariantList callbackCalls;

	Promise::Ptr newPromise = promise->always([&](const QVariant& data) {
		callbackCalls.push_back(data);
	});

	QVariant originalData("initial data");
	callActionOnDeferred(deferred, action, originalData, 1);

	QVERIFY(promise->state() == newPromise->state());
	QCOMPARE(callbackCalls, QVariantList() << originalData);
}

/*! \test Tests if a Promise chain survives deletion of intermediate Promises.
 */
void PromiseTest::testThreeLevelChain()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr originalPromise = Promise::create(deferred);

	QVariantList callbackCalls;

	Promise::Ptr finalPromise = originalPromise->then([&](const QVariant& data) {
		callbackCalls.push_back(data);
	})->then([&](const QVariant& data) {
		callbackCalls.push_back(data);
	});

	// Ensure deleteLater can be called on intermediate Promise
	QTest::qWait(100);

	QVariant data("my data");
	callActionOnDeferred(deferred, ACTION_RESOLVE, data, 1);
	QCOMPARE(callbackCalls, QVariantList() << data << data);
}

/*! \test Tests chaining multiple asynchronous operations.
 */
void PromiseTest::testAsyncChain()
{
	Deferred::Ptr deferred = Deferred::create();
	Deferred::Ptr transmitDeferred = Deferred::create();
	Promise::Ptr originalPromise = Promise::create(deferred);

	Promise::Ptr finalPromise = originalPromise
			->then([&](const QVariant&) -> Promise::Ptr {
		Deferred::Ptr secondDeferred = Deferred::create();
		QObject::connect(transmitDeferred.data(), &Deferred::resolved,
		                 secondDeferred.data(), &Deferred::resolve);
		return Promise::create(secondDeferred);
	});

	// Ensure deleteLater can be called on intermediate objects
	QTest::qWait(100);

	QVariant data("my data");
	callActionOnDeferred(deferred, ACTION_RESOLVE, data, 1);

	QTest::qWait(100);

	QCOMPARE(finalPromise->state(), Deferred::Pending);

	QVariant secondData("second data");
	qDebug("Resolve transmitDeferred");
	callActionOnDeferred(transmitDeferred, ACTION_RESOLVE, secondData, 1);

	QTest::qWait(100);

	QCOMPARE(finalPromise->state(), Deferred::Resolved);
	QCOMPARE(finalPromise->data(), secondData);
}

/*! \test Tests resolving a combined Promise created with Promise::all().
 */
void PromiseTest::testAll()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::all(promises);

	PromiseSpies spies(combinedPromise);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	QList<QVariant> results;
	results << "My string" << 15 << QVariant::fromValue(QList<int>() << 7 << 13);

	deferreds[0]->resolve(results[0]);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	deferreds[2]->resolve(results[2]);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	deferreds[1]->resolve(results[1]);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.resolved.first().first(), QVariant::fromValue(results));
}

/*! \test Tests rejecting a combined Promise created with Promise::all().
 */
void PromiseTest::testAllReject()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::all(promises);

	PromiseSpies spies(combinedPromise);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	QVariant rejectReason = "Error string";

	deferreds[0]->resolve(13);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	deferreds[1]->reject(rejectReason);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.rejected.first().first(), rejectReason);
}

/*! \test Tests resolving a combined Promise created with Promise::any().
 */
void PromiseTest::testAny()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::any(promises);

	PromiseSpies spies(combinedPromise);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	QVariant rejectReason = "Error string";
	QVariant result = 13;

	deferreds[0]->reject(rejectReason);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	deferreds[1]->resolve(result);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.resolved.first().first(), QVariant::fromValue(result));
}

/*! \test Tests rejecting a combined Promise created with Promise::any().
 */
void PromiseTest::testAnyReject()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::any(promises);

	PromiseSpies spies(combinedPromise);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	QList<QVariant> rejectReasons;
	rejectReasons << "My string" << 15 << QVariant::fromValue(QList<int>() << 7 << 13);

	deferreds[0]->reject(rejectReasons[0]);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	deferreds[2]->reject(rejectReasons[2]);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);

	deferreds[1]->reject(rejectReasons[1]);

	QTest::qWait(100);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.rejected.first().first(), QVariant::fromValue(rejectReasons));
}

/*! Provides the data for the testAllAnySync() test.
 */
void PromiseTest::testAllAnySync_data()
{
	QTest::addColumn<QList<Deferred::Ptr>>("deferreds");
	QTest::addColumn<QList<int>>("expectedAllSignalCounts");
	QTest::addColumn<QList<int>>("expectedAnySignalCounts");

	QList<Deferred::Ptr> oneResolvedDeferreds;
	oneResolvedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	oneResolvedDeferreds[0]->resolve("foo");
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("one resolved") << oneResolvedDeferreds << (QList<int>() << 0 << 0 << 0) << (QList<int>() << 1 << 0 << 0);

	QList<Deferred::Ptr> allResolvedDeferreds;
	allResolvedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	allResolvedDeferreds[0]->resolve("foo");
	allResolvedDeferreds[1]->resolve(17);
	allResolvedDeferreds[2]->resolve(true);
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("all resolved") << allResolvedDeferreds << (QList<int>() << 1 << 0 << 0) << (QList<int>() << 1 << 0 << 0);

	QList<Deferred::Ptr> oneRejectedDeferreds;
	oneRejectedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	oneRejectedDeferreds[0]->reject("foo");
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("one rejected") << oneRejectedDeferreds << (QList<int>() << 0 << 1 << 0) << (QList<int>() << 0 << 0 << 0);

	QList<Deferred::Ptr> allRejectedDeferreds;
	allRejectedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	allRejectedDeferreds[0]->reject("foo");
	allRejectedDeferreds[1]->reject(21);
	allRejectedDeferreds[2]->reject(false);
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("all rejected") << allRejectedDeferreds << (QList<int>() << 0 << 1 << 0) << (QList<int>() << 0 << 1 << 0);
}

/*! \test Tests Promise::all() and Promise::any()
 * when created with already resolved promises.
 */
void PromiseTest::testAllAnySync()
{
	QFETCH(QList<Deferred::Ptr>, deferreds);
	QFETCH(QList<int>, expectedAllSignalCounts);
	QFETCH(QList<int>, expectedAnySignalCounts);

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);
	
	Promise::Ptr allPromise = Promise::all(promises);
	Promise::Ptr anyPromise = Promise::any(promises);
	
	PromiseSpies allSpies(allPromise);
	PromiseSpies anySpies(anyPromise);
	
	QTest::qWait(100);
	QCOMPARE(allSpies.resolved.count(), expectedAllSignalCounts[0]);
	QCOMPARE(allSpies.rejected.count(), expectedAllSignalCounts[1]);
	QCOMPARE(allSpies.notified.count(), expectedAllSignalCounts[2]);
	QCOMPARE(anySpies.resolved.count(), expectedAnySignalCounts[0]);
	QCOMPARE(anySpies.rejected.count(), expectedAnySignalCounts[1]);
	QCOMPARE(anySpies.notified.count(), expectedAnySignalCounts[2]);
}

/*! \test Tests Promise::all() and Promise::any()
 * with initializer lists.
 */
void PromiseTest::testAllAnyInitializerList()
{
	Promise::Ptr firstPromise = Promise::createResolved(17);
	Promise::Ptr secondPromise = Promise::createResolved(4);

	Promise::Ptr allPromise = Promise::all({firstPromise, secondPromise});
	Promise::Ptr anyPromise = Promise::any({firstPromise, secondPromise});

	QTest::qWait(100);
	QCOMPARE(allPromise->state(), Deferred::Resolved);
	QCOMPARE(anyPromise->state(), Deferred::Resolved);
}



}  // namespace Tests
}  // namespace QtPromise


QTEST_MAIN(QtPromise::Tests::PromiseTest)
#include "PromiseTest.moc"


