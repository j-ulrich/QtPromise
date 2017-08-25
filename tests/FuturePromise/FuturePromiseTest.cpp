
#include <QtTest>
#include <QtDebug>
#include <QSignalSpy>
#include <QtConcurrent>
#include <functional>
#include "FuturePromise.h"


namespace QtPromise
{
namespace Tests
{

/*! \brief Unit tests for the FuturePromise class.
 *
 * \author jochen.ulrich
 */
class FuturePromiseTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanup();
	void testBasicFuture();
	void testMultipleResults();
	void testProgressReporting();
	void testCancel();

private:
	struct PromiseSpies
	{
		PromiseSpies(FuturePromise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy baseResolved;
		QSignalSpy rejected;
		QSignalSpy baseRejected;
		QSignalSpy notified;
		QSignalSpy baseNotified;
	};
};


//####### Helpers #######
FuturePromiseTest::PromiseSpies::PromiseSpies(FuturePromise::Ptr promise)
	: resolved(promise.data(), &FuturePromise::resolved),
	  rejected(promise.data(), &FuturePromise::rejected),
	  notified(promise.data(), &FuturePromise::notified),
	  baseResolved(promise.data(), &Promise::resolved),
	  baseRejected(promise.data(), &Promise::rejected),
	  baseNotified(promise.data(), &Promise::notified)
{
}

void FuturePromiseTest::initTestCase()
{
	/* Ensure we have at most one worker thread.
	 * Else it is impossible to have the QFutures report progress in a
	 * predictable manner.
	 */
	QThreadPool::globalInstance()->setMaxThreadCount(2);
}

void FuturePromiseTest::cleanup()
{
	// Let deleteLater be executed to clean up
	QTest::qWait(100);
}


//####### Tests #######
/*! \test Tests a FuturePromise with a QFuture created from QtConcurrent::run().
 */
void FuturePromiseTest::testBasicFuture()
{
	int returnValue = 0;
	QWaitCondition waitCond;
	QFuture<int> future = QtConcurrent::run([&]() -> int {
		QMutex mutex;
		QMutexLocker locker(&mutex);
		waitCond.wait(&mutex);
		return returnValue;
	});

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	QTest::qWait(1 * 1000); // 1s
	QVERIFY(!future.isFinished());
	QCOMPARE(promise->state(), Deferred::Pending);

	returnValue = 42;
	waitCond.wakeAll();

	future.waitForFinished();
	QTRY_COMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(spies.baseResolved.count(), 1);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(promise->results(), spies.resolved.first().first().toList());
	QCOMPARE(promise->results().first().value<int>(), returnValue);
}

void FuturePromiseTest::testMultipleResults()
{
	QList<int> input;
	input << 1 << 2 << 3;

	QMutex mutex;

	std::function<int(const int&)> mapFunction = [&](const int& value) -> int {
		QMutexLocker locker(&mutex);
		return value * 2;
	};

	// Prevent the thread(s) from running before we set up the promise
	mutex.lock();

	QFuture<int> future = QtConcurrent::mapped(input, mapFunction);

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	// Allow the thread(s) to run
	mutex.unlock();

	QTRY_COMPARE(promise->state(), Deferred::Resolved);
	QVariantList expectedResults;
	expectedResults << 2 << 4 << 6;
	QCOMPARE(promise->results(), expectedResults);
	QCOMPARE(spies.baseResolved.count(), 1);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.resolved.first().first().toList(), promise->results());
	QCOMPARE(spies.baseResolved.first().first(), QVariant::fromValue(promise->results()));

}

void FuturePromiseTest::testCancel()
{
	QList<int> input;
	input << 1 << 2 << 3;

	QWaitCondition waitCond;

	std::function<int(const int&)> mapFunction = [&](const int& value) -> int {
		QMutex mutex;
		QMutexLocker locker(&mutex);
		waitCond.wait(&mutex);
		return value * 2;
	};

	QFuture<int> future = QtConcurrent::mapped(input, mapFunction);

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	QTest::qWait(1 * 500);

	waitCond.wakeOne();

	QTRY_COMPARE(future.resultCount(), 1);

	future.cancel();
	waitCond.wakeAll();

	QTRY_COMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(spies.baseRejected.count(), 1);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.rejected.first().first().toList(), promise->results());
	QCOMPARE(spies.baseRejected.first().first(), QVariant::fromValue(promise->results()));
	QCOMPARE(promise->results().count(), 1);
	QCOMPARE(promise->results().first().value<int>(), 2);
}

void FuturePromiseTest::testProgressReporting()
{
	QList<int> input;
	input << 1 << 2 << 3;

	QWaitCondition waitCond;

	std::function<int(const int&)> mapFunction = [&](const int& value) -> int {
		QMutex mutex;
		QMutexLocker locker(&mutex);
		waitCond.wait(&mutex);
		return value * 2;
	};

	QFuture<int> future = QtConcurrent::mapped(input, mapFunction);

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	QCOMPARE(future.progressValue(), 0);
	QCOMPARE(future.progressMinimum(), 0);
	QCOMPARE(future.progressMaximum(), input.length());

	// The first two notifications inform about progress min/max and initial value (0)
	QTRY_COMPARE(spies.notified.count(), 2);
	FutureDeferred::Progress expectedProgress;
	expectedProgress.min = 0;
	expectedProgress.max = input.length();
	expectedProgress.value = -1;
	QCOMPARE(spies.notified.at(0).first().value<FutureDeferred::Progress>(), expectedProgress);

	expectedProgress.min = 0;
	expectedProgress.max = input.length();
	expectedProgress.value = 0;
	QCOMPARE(spies.notified.at(1).first().value<FutureDeferred::Progress>(), expectedProgress);

	waitCond.wakeOne();

	// First real progress
	QTRY_COMPARE(spies.notified.count(), 3);
	expectedProgress.min = 0;
	expectedProgress.max = input.length();
	expectedProgress.value = 1;
	QCOMPARE(spies.notified.at(2).first().value<FutureDeferred::Progress>(), expectedProgress);

	waitCond.wakeOne();

	// Second real progress
	QTRY_COMPARE(spies.notified.count(), 4);
	expectedProgress.min = 0;
	expectedProgress.max = input.length();
	expectedProgress.value = 2;
	QCOMPARE(spies.notified.at(3).first().value<FutureDeferred::Progress>(), expectedProgress);

	waitCond.wakeOne();

	// Finished
	QTRY_COMPARE(promise->state(), Deferred::Resolved);
}


}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::FuturePromiseTest)
#include "FuturePromiseTest.moc"


