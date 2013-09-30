package com.ipaloma.posjniproject.jni;

/*
 *   typedef enum MITLogFuncRetValue {
 *   MITLOG_RETV_SUCCESS              = 0,
 *   MITLOG_RETV_FAIL                 = -1,
 *   MITLOG_RETV_PARAM_ERROR          = -100,
 *   MITLOG_RETV_OPEN_FILE_FAIL       = -101,
 *   MITLOG_RETV_ALLOC_MEM_FAIL       = -102,
 *   MITLOG_RETV_HAS_OPENED           = -103
 *	} MITLogFuncRetValue;
 */


public class NativeUtilitiesClass {
	/*
	 * Save the application's information.
	 * If you want to change application's information, you must call this
	 * function and sendWDRegisterPackage() again.
	 *  
	 * @param: pid 			the process identifier
	 * 		   appName		the application's name
	 * 		   cmdLine		the start command line of application
	 * 		   versionStr   the version string of the application, for example "v1.0.1"
	 * @return: please refer enum MITLogFuncRetValue
	 * WARNING: you should only call this function in main thread
	 *			before other thread started.
	 *          Remember to call free_appinfo_config() at the end
	 *          of the main thread.
	 */
	public native int saveAppInfoConfig(int pid, String appName, String cmdLine, String versionStr);
	
	/*
	 * Free the application's information.
	 * This function should be called at the end of the thread. 
	 * WARNING: you should only call this function in main thread
	 *          after other thread ended.
	 */
	public native void freeAppInfoConfig();
	
	/*
	 * Initialize the feeding socket. Every thread so call this function one time.
	 * 
	 * @return: if success the socket identifier will be returned else -1 will be returned.
	 */
	public native int initUDPSocket();
	
	/*
	 * Close the UDP socket.
	 * 
	 * @param: socketID		the socket identifier which you want to close
	 * @return: please refer enum MITLogFuncRetValue
	 */
	public native int closeUPDClient(int socketID);
	
	/*
	 * The monitored application register to the watchdog.
	 * 
	 * @param: socketID		the socket identifier which you want to close
	 * 		   period		the feed period by second
	 * 		   threadID		the monitored thread identifier, you can use the socketID as threadID
	 * 						but you should always use the same threadID in one thread
	 * @return: please refer enum MITLogFuncRetValue
	 */
	public native int sendWDRegisterPackage(int socketID, short period, int threadID);
	
	/*
	 * The monitored application feed to the watchdog.
	 * Every time you can change the period for feeding if you plan to do some heavy work.
	 * 
	 * @param: socketID		the socket identifier which you want to close
	 * 		   period		the feed period by second
	 * 		   threadID		the monitored thread identifier, it must be same with register threadID
	 * @return: please refer enum MITLogFuncRetValue
	 */
	public native int sendWDFeedPackage(int socketID, short period, int threadID);
	
	/*
	 * The monitored application unregister to the watchdog.
	 *  
	 * @param: socketID		the socket identifier which you want to close
	 * 		   threadID		the monitored thread identifier, it must be same with register threadID
	 * @return: please refer enum MITLogFuncRetValue
	 */
	public native int sendWDUnregisterPackage(int socketID, int threadID);
	
    // Declare native method (and make it public to expose it directly)
	// @return On success 0 will be returned else means error.
    public native int execCmdLine(String cmdline);
 
    // Load library
    static {
        System.loadLibrary("native_utilities");
    }
}