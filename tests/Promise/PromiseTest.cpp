
#include <QtTest>
#include <stdexcept>
#include <string>
#include "Promise.h"

namespace QtPromise
{
namespace Tests
{

/*! Unit tests for the Promise class.
 *
 * \author jochen.ulrich
 */
class PromiseTest : public QObject
{
	Q_OBJECT

private slots:
	void constructorTest();
	void constructorWithResolvedDeferredTest();
	void constructorWithRejectedDeferredTest();
	void createResolvedPromiseTest();
	void createRejectedPromiseTest();
	void resolveTest();
	void rejectTest();
	void notifyTest();
	void thenVoidCallbackTest_data();
	void thenVoidCallbackTest();
	void thenVariantCallbackTest_data();
	void thenVariantCallbackTest();
	void thenPromiseCallbackTest_data();
	void thenPromiseCallbackTest();
	void alwaysTest_data();
	void alwaysTest();
	void threeLevelChainTest();
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

void PromiseTest::callActionOnDeferred(Deferred::Ptr& deferred, const QString& action, const QVariant& data, int repetitions)
{
	for (int i=0; i < repetitions; ++i)
	{
		if (action == "resolve")
			deferred->resolve(data);
		else if (action == "reject")
			deferred->reject(data);
		else if (action == "notify")
			deferred->notify(data);
		else
			throw std::invalid_argument(std::string("Unknown action: ") + action.toStdString());
	}
}

void PromiseTest::cleanup()
{
	// Let deleteLater be executed to clean up
	QTestEventLoop().enterLoopMSecs(100);
}

//####### Tests #######

void PromiseTest::constructorTest()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	QCOMPARE(promise->state(), Deferred::Pending);
	QVERIFY(promise->data().isNull());
}

void PromiseTest::constructorWithResolvedDeferredTest()
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

void PromiseTest::constructorWithRejectedDeferredTest()
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

void PromiseTest::createResolvedPromiseTest()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createResolved(myString);

	QCOMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(promise->data().toString(), myString);
}

void PromiseTest::createRejectedPromiseTest()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createRejected(myString);

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->data().toString(), myString);
}

void PromiseTest::resolveTest()
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

void PromiseTest::rejectTest()
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

void PromiseTest::notifyTest()
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

void PromiseTest::thenVoidCallbackTest_data()
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
	QTest::newRow("sync resolve") << false << "resolve" << data << Deferred::Resolved << data << (QVariantList() << data) << (QVariantList()) << (QVariantList());
	QTest::newRow("sync reject") << false << "reject" << data << Deferred::Rejected << data << (QVariantList()) << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync notify") << false << "notify" << data << Deferred::Pending << QVariant() << (QVariantList()) << (QVariantList()) << (QVariantList());
	QTest::newRow("async resolve") << true << "resolve" << data << Deferred::Resolved << data << (QVariantList() << data) << (QVariantList()) << (QVariantList());
	QTest::newRow("async reject") << true << "reject" << data << Deferred::Rejected << data << (QVariantList()) << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async notify") << true << "notify" << data << Deferred::Pending << QVariant() << (QVariantList()) << (QVariantList()) << (QVariantList() << data << data);
}

void PromiseTest::thenVoidCallbackTest()
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

void PromiseTest::thenVariantCallbackTest_data()
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
	QTest::newRow("sync resolve") << false << "resolve" << originalData << chainedData << Deferred::Resolved << (QVariantList()) << Deferred::Resolved << chainedData << (QVariantList() << chainedData) << (QVariantList()) << (QVariantList());
	QTest::newRow("sync reject") << false << "reject" << originalData << chainedData << Deferred::Rejected << (QVariantList()) << Deferred::Resolved << chainedData << (QVariantList() << chainedData) << (QVariantList()) << (QVariantList());
	QTest::newRow("sync notify") << false << "notify" << originalData << chainedData << Deferred::Pending << (QVariantList()) << Deferred::Pending << QVariant() << (QVariantList()) << (QVariantList()) << (QVariantList());
	QTest::newRow("async resolve") << true << "resolve" << originalData << chainedData << Deferred::Resolved << (QVariantList()) << Deferred::Resolved << chainedData << (QVariantList() << chainedData) << (QVariantList()) << (QVariantList());
	QTest::newRow("async reject") << true << "reject" << originalData << chainedData << Deferred::Rejected << (QVariantList()) << Deferred::Resolved << chainedData << (QVariantList() << chainedData) << (QVariantList()) << (QVariantList());
	QTest::newRow("async notify") << true << "notify" << originalData << chainedData << Deferred::Pending << (QVariantList() << originalData << originalData) << Deferred::Pending << QVariant() << (QVariantList()) << (QVariantList()) << (QVariantList() << chainedData << chainedData);
}

void PromiseTest::thenVariantCallbackTest()
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

	Promise::Ptr newPromise = promise->then([&](const QVariant& value) -> QVariant {
		return chainedData;
	}, [&](const QVariant& reason) -> QVariant {
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

void PromiseTest::thenPromiseCallbackTest_data()
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
	//                                        // async // action    // callbackAction // callbackData // expectedOriginalState // expectedChainedState expectedChainedData // expectedResolvedCalls    // expectedRejectedCalls
	QTest::newRow("sync resolve -> reject")   << false << "resolve" << "reject"       << data         << Deferred::Resolved    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("sync resolve -> resolve")  << false << "resolve" << "resolve"      << data         << Deferred::Resolved    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync reject -> resolve")   << false << "reject"  << "resolve"      << data         << Deferred::Rejected    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync reject -> reject")    << false << "reject"  << "reject"       << data         << Deferred::Rejected    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("async resolve -> reject")  << true  << "resolve" << "reject"       << data         << Deferred::Resolved    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("async resolve -> resolve") << true  << "resolve" << "resolve"      << data         << Deferred::Resolved    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async reject -> resolve")  << true  << "reject"  << "resolve"      << data         << Deferred::Rejected    << Deferred::Resolved   << data             << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async reject -> reject")   << true  << "reject"  << "reject"       << data         << Deferred::Rejected    << Deferred::Rejected   << data             << (QVariantList())         << (QVariantList() << data);
}

void PromiseTest::thenPromiseCallbackTest()
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

	auto callback = [&](const QVariant& value) -> Promise::Ptr {
		if (callbackAction == "resolve")
			return Promise::createResolved(callbackData);
		else if (callbackAction == "reject")
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

void PromiseTest::alwaysTest_data()
{
	QTest::addColumn<QString>("action");

	QTest::newRow("resolve") << "resolve";
	QTest::newRow("reject") << "reject";
}

void PromiseTest::alwaysTest()
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

/*! \test Tests if a Promise chain survives deletion of intermediate
 * Promises.
 */
void PromiseTest::threeLevelChainTest()
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
	QTestEventLoop().enterLoopMSecs(100);

	QVariant data("my data");
	callActionOnDeferred(deferred, "resolve", data, 1);
	QCOMPARE(callbackCalls, QVariantList() << data << data);
}

}  // namespace Tests
}  // namespace QtPromise


QTEST_MAIN(QtPromise::Tests::PromiseTest)
#include "PromiseTest.moc"


