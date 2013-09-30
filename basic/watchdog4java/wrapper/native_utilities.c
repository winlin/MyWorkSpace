#include "../common/mit_data_define.h"
#include "../common/mit_log_module.h"
#include "../common/watchdog_comm.h"
#include "native_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <android/log.h>

#define PIPE_EXEC_MAXLINE 1024

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "POSJNIProject", __VA_ARGS__);
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , "POSJNIProject", __VA_ARGS__);
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , "POSJNIProject", __VA_ARGS__);
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , "POSJNIProject", __VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , "POSJNIProject", __VA_ARGS__);

int pipe_exec_cmd(const char *cmd_line)
{
	LOGI("cmd_line will be exec:%s", cmd_line);
	char result_buf[PIPE_EXEC_MAXLINE];
	int rc = 0;
	FILE *fp = popen(cmd_line, "r");
	if(NULL == fp) {
		LOGE("popen()failed:%s", strerror(errno));
		return -1;
	}
	while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
	{
		if('\n' == result_buf[strlen(result_buf)-1]) {
			result_buf[strlen(result_buf)-1] = '\0';
		}
		LOGI("[output]:%s\n", result_buf);
		memset(result_buf, 0, PIPE_EXEC_MAXLINE);
	}
	// wait for the child process return
	rc = pclose(fp);
	if(-1 == rc) {
		LOGE("pclose()failed:%s", strerror(errno));
		return -1;
	}
	return rc;
}

JNIEXPORT jint JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_saveAppInfoConfig
  (JNIEnv *jev, jobject jobj, jint monitorPID, jstring appName, jstring cmdLine, jstring versionStr)
{
	const char *app_name = (*jev)->GetStringUTFChars(jev, appName, NULL);
	LOGI("Get app name :%s\n", app_name);
	// open the log module
	char log_path[MAX_AB_PATH_LEN] = {0};
	sprintf(log_path, "%s%s", LOG_FILE_PATH, app_name);
	LOGI("The %s log path:%s", app_name, log_path);
	MITLogFuncRetValue log_ret = MITLogOpen(app_name, log_path, _IOLBF);
	if (log_ret != MITLOG_RETV_SUCCESS) {
		LOGE("MITLogOpen() failed");
		(*jev)->ReleaseStringUTFChars(jev, appName, app_name);
		return MIT_RETV_FAIL;
	}

	const char *cmd_line = (*jev)->GetStringUTFChars(jev, cmdLine, NULL);
	LOGI("Get cmd line :%s\n", cmd_line);
	const char *version_str = (*jev)->GetStringUTFChars(jev, versionStr, NULL);
	LOGI("Get version str :%s\n", version_str);

	MITFuncRetValue func_ret = save_appinfo_config(monitorPID, app_name, cmd_line, version_str);
	if(func_ret != MIT_RETV_SUCCESS) {
		LOGE("save_appinfo_config() failed:%d", func_ret);
	}

	(*jev)->ReleaseStringUTFChars(jev, appName, app_name);
	(*jev)->ReleaseStringUTFChars(jev, cmdLine, cmd_line);
	(*jev)->ReleaseStringUTFChars(jev, versionStr, version_str);

	return func_ret;
}

JNIEXPORT void JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_freeAppInfoConfig
  (JNIEnv *jev, jobject jobj)
{
	free_appinfo_config();
	MITLogClose();
}

JNIEXPORT jint JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_initUDPSocket
  (JNIEnv *jev, jobject jobj)
{
	int socket_id = init_udp_socket();
	if(socket_id <= 0) {
		LOGE("init_udp_socket() failed");
	}
	return socket_id;
}

JNIEXPORT jint JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_closeUPDClient
  (JNIEnv *jev, jobject jobj, jint socketID)
{
	MITFuncRetValue func_ret = close_udp_socket(socketID);
	if(func_ret != MIT_RETV_SUCCESS) {
		LOGE("close_udp_socket() failed");
	}
	return func_ret;
}

JNIEXPORT jint JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_sendWDRegisterPackage
  (JNIEnv *jev, jobject jobj, jint socketID, jshort period, jint threadID)
{
	MITFuncRetValue func_ret = send_wd_register_package(socketID, period, threadID);
	if(func_ret != MIT_RETV_SUCCESS) {
		LOGE("send_wd_register_package() failed");
	}
	return func_ret;
}

JNIEXPORT jint JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_sendWDFeedPackage
  (JNIEnv *jev, jobject jobj, jint socketID, jshort period, jint threadID)
{
	MITFuncRetValue func_ret = send_wd_feed_package(socketID, period, threadID);
	if(func_ret != MIT_RETV_SUCCESS) {
		LOGE("send_wd_feed_package() failed");
	}
	return func_ret;
}

JNIEXPORT jint JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_sendWDUnregisterPackage
  (JNIEnv *jev, jobject jobj, jint socketID, jint threadID)
{
	MITFuncRetValue func_ret = send_wd_unregister_package(socketID,threadID);
	if(func_ret != MIT_RETV_SUCCESS) {
		LOGE("send_wd_unregister_package() failed");
	}
	return func_ret;
}

JNIEXPORT jint JNICALL Java_com_ipaloma_posjniproject_jni_NativeUtilitiesClass_execCmdLine(JNIEnv *jev, jobject jobj, jstring cmdline)
{
	int ret = -1;
	const char *pcmd = (*jev)->GetStringUTFChars(jev, cmdline, 0);
	LOGI("Get command line:%s\n", pcmd);
	if(strlen(pcmd) > 0) {
		ret = pipe_exec_cmd(pcmd);
	}
	(*jev)->ReleaseStringUTFChars(jev, cmdline, pcmd);
	return ret;
}


