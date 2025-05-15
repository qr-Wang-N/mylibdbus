#ifndef DBUSOPERATE_H
#define DBUSOPERATE_H
#include "libdbus.h"
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include "dbus/dbus.h"
#include "base_fun.h"
#include "global_config.h"
#include <time.h>
#ifdef WIN32
#include <windows.h>
#define pthread_t HANDLE
#else
#include <pthread.h>
#include <unistd.h>
#include "./third/tinyxml/tinyxml.h"
#endif

typedef struct dbus_info
{
    DBusConnection * connect_;
    std::string dbus_name_;
    std::string dbus_path_;
}DBUS_INFO;

#if 0
#define USED_COMMON_LOG
#endif

#ifndef USED_COMMON_LOG
#define LIBDBUS_COUT std::cout << "[" <<__FILE__ << "]["<< __FUNCTION__ << "]["<< __LINE__ << "] "<< time(NULL)
#define LIBDBUS_DEBUG LIBDBUS_COUT
#define LIBDBUS_ENDL std::endl
#else
#include "../common/zf_log.h"
#define LIBDBUS_COUT std::cout
#define LIBDBUS_DEBUG std::cout
#define LIBDBUS_ENDL std::endl
#endif

class DbusOperate
{
public:
    DbusOperate();
    static DbusOperate* getInstance();
#ifdef WIN32
	static DWORD WINAPI worker(LPVOID arg);
#else
	static void *worker(void *arg);
#endif
    void terminate();
    void run(DBusConnection *conn, const std::string& dbus_path);
    bool init(char* token,MessageProc proc,char *srv_list);

    uint32_t creatAllSystemBusConf();
    uint32_t regSysDbus();
    uint32_t createListenDbusThread();
    uint32_t sendMessage(char* service, char* method, uint8_t msg_type, char* inargs, int32_t arg_len, char** retval, int32_t* ret_len, int32_t timeout);

private:
    void replyToMessage(DBusConnection *conn, DBusMessage *msg,int msg_type);
    uint32_t sendSingalMsg(char* service,char* method,char* inargs,int32_t arg_len,int32_t timeout);
    uint32_t sendMethodMsg(char* service, char* method, char* inargs, int32_t arg_len, char** retval, int32_t* ret_len, int32_t timeout);
    uint32_t creatSystemBusConf(const std::string& dbus_name);
    uint32_t initDbusConnection(DBusConnection **conn, const std::string&dbus_name);

private:
    static DbusOperate*   instance_;
    static bool terminated_;
	static pthread_t rpcclient_thread_id_;
    static DBusConnection * connect_send_;

    static MessageProc call_proc_;  //callback ptr
    std::string srv_list_name_; //input servername list: server1#server2#server3
    std::vector<DBUS_INFO> dbus_info_; //base info
    std::string self_exe_name_;
};

#endif // DBUSOPERATE_H
