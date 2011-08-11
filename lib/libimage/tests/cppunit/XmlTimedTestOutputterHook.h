#ifndef XMLTIMEDTESTOUTPUTTERHOOK_H
#define XMLTIMEDTESTOUTPUTTERHOOK_H


#include <cppunit/XmlOutputterHook.h>
#include <cppunit/tools/XmlElement.h>
#include <cppunit/tools/StringTools.h>
#include "TimedTestListener.h"

 class XmlTimedTestOutputterHook : public CppUnit::XmlOutputterHook
 {
 public:
   XmlTimedTestOutputterHook(TimedTestListener *_ttlisten);
   
   virtual void failTestAdded (CppUnit::XmlDocument *document, CppUnit::XmlElement *testElement, CppUnit::Test *test, CppUnit::TestFailure *failure);
   virtual void successfulTestAdded (CppUnit::XmlDocument *document, CppUnit::XmlElement *testElement, CppUnit::Test *test);
   virtual void statisticsAdded (CppUnit::XmlDocument *document, CppUnit::XmlElement *statisticsElement);
   
 protected:
   TimedTestListener * ttlisten;
   
 };
 
#endif //XMLTIMEDTESTOUTPUTTERHOOK_H