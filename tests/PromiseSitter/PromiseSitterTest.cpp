
#include <QtTest>
#include <QtDebug>
#include <QPointer>
#include <QScopedPointer>
#include "PromiseSitter.h"


namespace QtPromise
{
namespace Tests
{

const QString ACTION_RESOLVE = "resolve";
const QString ACTION_REJECT = "reject";
const QString ACTION_NOTIFY = "notify";

/*! Unit tests for the PromiseSitter class.
 *
 * \author jochen.ulrich
 */
class PromiseSitterTest : public QObject
{
	Q_OBJECT

private slots:
	void testAddContainsRemove();
	void testPromiseLifetime_data();
	void testPromiseLifetime();
	void testGlobalInstance_data();
	void testGlobalInstance();
	void cleanup();

private:
	struct PromiseSpies
	{
		PromiseSpies(Promise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy rejected;
		QSignalSpy notified;
	};

};


//####### Helper #######

PromiseSitterTest::PromiseSpies::PromiseSpies(Promise::Ptr promise)
	: resolved(promise.data(), &Promise::resolved),
	  rejected(promise.data(), &Promise::rejected),
	  notified(promise.data(), &Promise::notified)
{
}

void PromiseSitterTest::cleanup()
{
	// Let deleteLater be executed to clean up
	QTest::qWait(100);
}

//####### Tests #######

/*! \test Tests the PromiseSitter::add(), PromiseSitter::contains()
 * and PromiseSitter::remove() methods.
 */
void PromiseSitterTest::testAddContainsRemove()
{
	PromiseSitter sitter;

	Deferred::Ptr deferred = Deferred::create();
	QPointer<Promise> promisePointer;

	Promise::Ptr promise = Promise::create(deferred);
	promisePointer = promise.data();

	sitter.add(promise);
	QVERIFY(sitter.contains(promise));
	QVERIFY(sitter.remove(promise));
	QVERIFY(!sitter.contains(promise));
	QVERIFY(!sitter.remove(promise));

	sitter.add(promise);
	Promise::Ptr samePromise = promise;
	QVERIFY(sitter.contains(samePromise));

	deferred->resolve();

	QVERIFY(!sitter.contains(promise));
	QVERIFY(!sitter.contains(samePromise));
	QVERIFY(!promisePointer.isNull());
}

/*! Provides the data for the testPromiseLifetime() test.
 */
void PromiseSitterTest::testPromiseLifetime_data()
{
	QTest::addColumn<QString>("action");

	QTest::newRow("resolve") << ACTION_RESOLVE;
	QTest::newRow("reject") << ACTION_REJECT;
	QTest::newRow("notify") << ACTION_NOTIFY;
}

/*! \test Tests the releasing of Promises from the PromiseSitter.
 */
void PromiseSitterTest::testPromiseLifetime()
{
	QFETCH(QString, action);

	PromiseSitter sitter;

	Deferred::Ptr deferred = Deferred::create();
	QScopedPointer<PromiseSpies> spies;
	QPointer<Promise> promisePointer;
	bool chainedActionTriggered = false;
	{
		Promise::Ptr promise = Promise::create(deferred);
		sitter.add(promise);
		promisePointer = promise.data();
		spies.reset(new PromiseSpies{promise});

		sitter.add(promise->always([&chainedActionTriggered](const QVariant&) {
			chainedActionTriggered = true;
		}));

		// Our Promise::Ptr goes out of scope
	}

	// Allow a potential deleteLater to be executed
	QTest::qWait(100);
	QVERIFY2(!promisePointer.isNull(), "Promise was destroyed although added to the sitter");

	QString data = "foo bar";
	if (action == ACTION_RESOLVE)
		deferred->resolve(data);
	else if (action == ACTION_REJECT)
		deferred->reject(data);
	else if (action == ACTION_NOTIFY)
		deferred->notify(data);
	else
		QFAIL("Invalid value for \"action\"");

	// Give the sitter the chance to drop its reference.
	QTest::qWait(100);

	if (action == ACTION_NOTIFY)
		QVERIFY(!promisePointer.isNull());
	else
		QVERIFY(promisePointer.isNull());

	// Verify actions have been triggered
	if (action == ACTION_RESOLVE)
	{
		QCOMPARE(spies->resolved.count(), 1);
		QCOMPARE(spies->resolved.at(0).at(0).toString(), data);
	}
	else if (action == ACTION_REJECT)
	{
		QCOMPARE(spies->rejected.count(), 1);
		QCOMPARE(spies->rejected.at(0).at(0).toString(), data);
	}
	if (action == ACTION_NOTIFY)
	{
		QCOMPARE(spies->notified.count(), 1);
		QCOMPARE(spies->notified.at(0).at(0).toString(), data);
	}
	else
		QVERIFY(chainedActionTriggered);
}

/*! Provides the data for the testGlobalInstance() test.
 */
void PromiseSitterTest::testGlobalInstance_data()
{
	QTest::addColumn<Deferred::Ptr>("deferred");
	QTest::addColumn<Promise::Ptr>("promise");

	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);
	QTest::newRow("testGlobalInstance") << deferred << promise;
	PromiseSitter::instance()->add(promise);
}

/*! \test Tests the global instance of the PromiseSitter.
 */
void PromiseSitterTest::testGlobalInstance()
{
	QFETCH(Deferred::Ptr, deferred);
	QFETCH(Promise::Ptr, promise);

	QVERIFY(PromiseSitter::instance()->contains(promise));

	deferred->resolve("data");
	QTRY_VERIFY(!PromiseSitter::instance()->contains(promise));
}


}  // namespace Tests
}  // namespace QtPromise


QTEST_MAIN(QtPromise::Tests::PromiseSitterTest)
#include "PromiseSitterTest.moc"


