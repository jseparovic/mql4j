/* JavaJvm.cpp */
#include "stdafx.h"
#include "JavaJvm.h"

mql4j::java::JavaJvm * mql4j::java::JavaJvm::_instance = 0;

mql4j::java::JavaJvm::JavaJvm() {
	this->jvm = NULL;
	this->env = NULL;
	this->nOptions = 0;
}

mql4j::java::JavaJvm::~JavaJvm() {
}

mql4j::java::JavaJvm * mql4j::java::JavaJvm::instance() {
	if(_instance == 0) {
		_instance = new JavaJvm;
		log::trace(__FILE__, __LINE__, "Instance created");
		config::printConfig();
	}
	return _instance;
}

void mql4j::java::JavaJvm::release() {
	if(_instance != 0) {
		delete _instance;
		_instance = 0;
		log::trace(__FILE__, __LINE__, "Instance released");
	}
}

JNIEnv * mql4j::java::JavaJvm::getEnv() {
	if(env == NULL) {
		attach();
	}
	return env;
}

bool mql4j::java::JavaJvm::start() {
	log::info(__FILE__, __LINE__, "JVM start ...");
	if(isRunning()) {
		log::warn(__FILE__, __LINE__, "Found a running JVM thread. Use attach instead of start");
		return false;
	}
	options = new JavaVMOption[JVM_OPTS_MAX];
	addOption("-Djava.class.path=C:\\trading\\jtrader\\libs\\mql4java-all-1.0-SNAPSHOT.jar");
	addOption("-Dlogback.configurationFile=C:\\trading\\jtrader\\etc\\logback.xml");

//	addOption("-Djava.class.path=" + getClassPath());
//	addOption("-Dlogback.configurationFile=" + config::getHomeDir() + "\\logback.xml");
	addOption("-Xmx" + config::getJavaMaxMem());
	if(config::isJavaClassloaderVerbose()) {
		addOption("-verbose:class");
	}
	if(config::isJavaJNIVerbose()) {
		addOption("-verbose:jni");
	}
	JavaVMInitArgs vmArgs;
	vmArgs.version = JVM_JAVA_VERSION_REQ;
	vmArgs.options = options;
	vmArgs.nOptions = nOptions;
	vmArgs.ignoreUnrecognized = JNI_FALSE;
	jint createResult = JNI_CreateJavaVM(&jvm, (void**)&env, &vmArgs);
	delete options;
	if(createResult != JNI_OK) {
		log::error(__FILE__, __LINE__, "JVM start failed: " + getErrorStr(createResult));
		env = NULL;
		jvm = NULL;
		return false;
	}
	JavaException * startException = JavaException::create(env);
	if(startException != NULL) {
		log::warn(__FILE__, __LINE__, "Java exception occurred on startup");
		startException->printMessage();
		delete startException;
	}
	jint verMajor = ((env->GetVersion() >> 16) & 0x0f);
	jint verMinor = (env->GetVersion() & 0x0f);
	log::info(__FILE__, __LINE__, "Successfully started Java(TM) SE Runtime Environment " + to_string(
	              verMajor) + "." + to_string(verMinor));
	return true;
}

bool mql4j::java::JavaJvm::stop() {
	log::info(__FILE__, __LINE__, "JVM stop ...");
	if(!isRunning()) {
		log::error(__FILE__, __LINE__, "JVM stop failed: Not running");
		jvm = NULL;
		env = NULL;
		return false;
	}
	if(getEnv() == NULL) {
		log::error(__FILE__, __LINE__, "JVM stop failed: Invalid JVM environment");
		jvm = NULL;
		env = NULL;
		return false;
	}
	jint destroyResult = jvm->DestroyJavaVM();
	if(destroyResult != JNI_OK) {
		log::error(__FILE__, __LINE__, "JVM stop failed: " + getErrorStr(destroyResult));
		jvm = NULL;
		env = NULL;
		return false;
	}
	log::debug(__FILE__, __LINE__, "Successfully stopped JVM");
	jvm = NULL;
	env = NULL;
	return true;
}

bool mql4j::java::JavaJvm::isRunning() {
	jsize num;
	JNI_GetCreatedJavaVMs(NULL, 0, &num);
	if(num > 0) {
		return true;
	} else {
		return false;
	}
}

bool mql4j::java::JavaJvm::attach() {
	log::debug(__FILE__, __LINE__, "JVM attach ...");
	jsize num = 0;
	JNI_GetCreatedJavaVMs(NULL, 0, &num);
	log::trace(__FILE__, __LINE__, "Found " + std::to_string((int)num) + " running JVM thread(s)");
	if(num < 1) {
		log::warn(__FILE__, __LINE__, "Cannot attach to JVM thread: No JVM thread found");
		return false;
	}
	JavaVM ** jvms = new JavaVM*[num];
	JNI_GetCreatedJavaVMs(jvms, num, &num);
	this->jvm = jvms[0];
	JNIEnv * tmpEnv;
	log::trace(__FILE__, __LINE__, "Creating temporary JVM environment ...");
	jint tmpEnvResult = this->jvm->GetEnv((void **)&tmpEnv, JVM_JAVA_VERSION_REQ);
	if(tmpEnvResult != JNI_EDETACHED && tmpEnvResult != JNI_OK) {
		log::error(__FILE__, __LINE__, "Failed to get temporary JVM environment: " + getErrorStr(tmpEnvResult));
		this->env = NULL;
		this->jvm = NULL;
		return false;
	}
	log::trace(__FILE__, __LINE__, "Try to attach ...");
	jint attachResult = jvm->AttachCurrentThread((void **)&env, NULL);
	if(env == NULL) {
		log::error(__FILE__, __LINE__, "Failed to attach JVM thread: Got invalid JVM environment");
		this->env = NULL;
		this->jvm = NULL;
		return false;
	}
	if(attachResult != JNI_OK) {
		log::error(__FILE__, __LINE__, "Failed to attach JVM thread: " + getErrorStr(attachResult));
		this->env = NULL;
		this->jvm = NULL;
		return false;
	}
	log::debug(__FILE__, __LINE__, "Successfully attached JVM");
	return true;
}

string mql4j::java::JavaJvm::getClassPath() {
	string cp = "";
	WIN32_FIND_DATA fd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	string homeDir = config::getHomeDir();
	wstring find = mql4j::toWString(homeDir + "/*");
	hFind = FindFirstFile(find.c_str(), &fd);
	if(INVALID_HANDLE_VALUE == hFind) {
		log::warn(__FILE__, __LINE__, "Invalid libraries directory: " + homeDir);
		return cp;
	}
	while(FindNextFile(hFind, &fd) != 0) {
		string fileName = mql4j::toString(fd.cFileName);
		if(fileName.rfind(".jar") == fileName.length() - 4) {
			log::debug(__FILE__, __LINE__, "  (jar) " + fileName);
			if(cp.length() > 0) {
				cp += ";";
			}
			cp += homeDir + "/" + fileName;
		}
	}
	FindClose(hFind);
	if(cp.length() < 2) {
		log::warn(__FILE__, __LINE__, "No .jar file(s) found in " + homeDir);
	}
	replace(cp.begin(), cp.end(), '\\', '/');
	return cp;
}

void mql4j::java::JavaJvm::addOption(const string option) {
	if(nOptions >= JVM_OPTS_MAX) {
		log::warn(__FILE__, __LINE__, "Not added option '" + option + "': Limit exceeded");
		return;
	}
	log::debug(__FILE__, __LINE__, "  (opt) '" + option + "'");
	char * opt = new char[option.length() + 1]();
	memcpy_s(opt, option.length(), option.c_str(), option.length());
	options[nOptions].optionString = opt;
	nOptions++;
}

string mql4j::java::JavaJvm::getErrorStr(const jint rc) {
	string error;
	switch(rc) {
		case JNI_OK:
			return "No error";
		case JNI_EEXIST:
			return "Found already running JVM";
		case JNI_ERR:
			return "Unknown";
		case JNI_EDETACHED:
			return "Thread detached from the VM";
		case JNI_EVERSION:
			return "JNI verion";
		case JNI_ENOMEM:
			return "Not enough memory";
		case JNI_EINVAL:
			return "Invalid arguments";
		default:
			return "";
	}
}
