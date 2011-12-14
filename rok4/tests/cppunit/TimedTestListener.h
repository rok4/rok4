#ifndef TIMEDTESTRESULTCOLLECTOR_H
#define TIMEDTESTRESULTCOLLECTOR_H

#include <cppunit/TestListener.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/Test.h>
#include <sys/time.h>
#include <string>
#include <map>

class TimedTestListener : public CppUnit::TestResultCollector

{
public:
  TimedTestListener();

  //TestListener implementation
  virtual void startTest( CppUnit::Test *test );
  virtual void endTest( CppUnit::Test *test );
  virtual void startTestRun (CppUnit::Test *test, CppUnit::TestResult *eventManager);
  virtual void endTestRun (CppUnit::Test *test, CppUnit::TestResult *eventManager);
  
  //Time function
  virtual double getTotalTime();
  virtual double getTime( std::string testName );

protected:
  inline void setStartTime(CppUnit::Test *test);
  inline void setEndTime(CppUnit::Test *test);
  std::map<std::string,timeval> testStartTime;
  std::map<std::string,timeval> testEndTime;
  timeval testrunstart;
  timeval testrunend;
};
#endif //TIMEDTESTRESULTCOLLECTOR_H
