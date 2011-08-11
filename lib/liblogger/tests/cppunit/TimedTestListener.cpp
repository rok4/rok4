#include "TimedTestListener.h"
#include <cppunit/Test.h>
#include <cppunit/TestResult.h>
#include <map>
#include <time.h>

TimedTestListener::TimedTestListener()
{
}

// Test Listener
void TimedTestListener::startTest(CppUnit::Test *test)
{
    setStartTime(test);;
}

void TimedTestListener::endTest(CppUnit::Test *test)
{
    setEndTime(test);
}

void TimedTestListener::startTestRun(CppUnit::Test* test, CppUnit::TestResult* eventManager)
{
    gettimeofday(&testrunstart, NULL);
}

void TimedTestListener::endTestRun(CppUnit::Test* test, CppUnit::TestResult* eventManager)
{
    gettimeofday(&testrunend, NULL);
}

// Time
double TimedTestListener::getTotalTime()
{
    double time = testrunend.tv_sec - testrunstart.tv_sec + (testrunend.tv_usec - testrunstart.tv_usec)/1000000.;
    return time;
}

double TimedTestListener::getTime(std::string testName)
{
    timeval teststart,testend ;
    teststart = testStartTime[testName];
    testend = testEndTime[testName];
    double time = testend.tv_sec - teststart.tv_sec + (testend.tv_usec - teststart.tv_usec)/1000000.;
    return time;

}

void TimedTestListener::setStartTime(CppUnit::Test *test)
{
    timeval teststart;
    gettimeofday(&teststart, NULL);
    testStartTime[test->getName()]=teststart;
}

void TimedTestListener::setEndTime(CppUnit::Test *test)
{
    timeval testend;
    gettimeofday(&testend, NULL);
    testEndTime[test->getName()]=testend;
}
