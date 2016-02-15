/* Mql4jJava.cpp */
#include "stdafx.h"
#include "Mql4jJava.h"
#include "boost/format.hpp"

MQL4J_API bool mql4j::java::javaStart() {
	if(!java::JavaJvm::instance()->start()) {
		return false;
	}
	JNIEnv * env = JavaJvm::instance()->getEnv();
	return true;
}

MQL4J_API bool mql4j::java::javaStop() {
	log::warn(__FILE__, __LINE__, "Unloading JVM is not possible. You need to restart the terminal");
	return true;
}

MQL4J_API bool mql4j::java::javaStatus() {
	return JavaJvm::instance()->isRunning();
}


#define EXEC_ON_INIT_CLASS "com/jseppa/mql4java/base/mql/MQLExpert"
#define EXEC_ON_INIT_METHOD "execOnInit"
#define EXEC_ON_INIT_SIGNATURE "(JLjava/lang/String;)V"


MQL4J_API void mql4j::java::execOnInit(int64_t chartID, wchar_t *strategyClassName) {
	log::debug(__FILE__, __LINE__, str(boost::format("calling %1%") % EXEC_ON_INIT_METHOD));
	const char* className = EXEC_ON_INIT_CLASS;
	const char* methodName = EXEC_ON_INIT_METHOD;
	const char* signature = EXEC_ON_INIT_SIGNATURE;

	string stratClass = toString(strategyClassName);

	JNIEnv * env = JavaJvm::instance()->getEnv();
	jclass classHandle = JavaExecutor::getJClass(env, className);
	if (classHandle == NULL) return;

	jmethodID methodID = JavaExecutor::getJMethodID(env, classHandle, methodName, signature);
	if (classHandle == NULL) return;

	env->CallStaticVoidMethod(classHandle, methodID, chartID, env->NewStringUTF(stratClass.c_str()));
	log::debug(__FILE__, __LINE__, str(boost::format("Call %1% %2%(%3%,%4%)")
											% className % methodName % chartID % stratClass));
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return true;
}
 