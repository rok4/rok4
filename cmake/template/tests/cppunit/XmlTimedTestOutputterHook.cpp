#include "XmlTimedTestOutputterHook.h"
#include <string>
#include <sstream>
#include <iomanip>

XmlTimedTestOutputterHook::XmlTimedTestOutputterHook(TimedTestListener* _ttlisten): ttlisten(_ttlisten)
{

}

void XmlTimedTestOutputterHook::successfulTestAdded(CppUnit::XmlDocument* document, CppUnit::XmlElement* testElement, CppUnit::Test* test)
{
  std::ostringstream sstream;
  sstream << std::setprecision(4) << std::fixed << ttlisten->getTime(test->getName()); 
  testElement->addElement(new CppUnit::XmlElement( "Duration", sstream.str() ) ) ;
}

void XmlTimedTestOutputterHook::failTestAdded(CppUnit::XmlDocument* document, CppUnit::XmlElement* testElement, CppUnit::Test* test, CppUnit::TestFailure* failure)
{
  std::ostringstream sstream;
  sstream << std::setprecision(4) << std::fixed << ttlisten->getTime(test->getName()); 
  testElement->addElement(new CppUnit::XmlElement( "Duration", sstream.str() ) ) ;
}

void XmlTimedTestOutputterHook::statisticsAdded(CppUnit::XmlDocument* document, CppUnit::XmlElement* statisticsElement)
{
  char timestamp[20];
  tm * putctime;
  time_t unixtime;
  time(&unixtime);
  putctime = gmtime(&unixtime);
  strftime(timestamp,20,"%Y-%m-%dT%H:%M:%S",putctime);
  std::ostringstream sstream;
  sstream << std::setprecision(4) << std::fixed << ttlisten->getTotalTime();
  
  statisticsElement->addElement( new CppUnit::XmlElement( "Duration", sstream.str()));
  statisticsElement->addElement( new CppUnit::XmlElement( "Timestamp", timestamp));
}
