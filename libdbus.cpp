#include "libdbus.h"
#include "dbus_operate.h"
#include <iostream>

bool connect_flag = false;

int32_t libDbusConnectMessageBus(char* token, MessageProc msgProc, char* srvList)
{
    LIBDBUS_COUT << "libDbusConnectMessageBus start " << LIBDBUS_ENDL;
#ifdef WIN32
	char buffer[256] = { 0 };
	DWORD length = GetEnvironmentVariableA("DBUS_SESSION_BUS_ADDRESS", buffer, 256);
	if (strcmp(buffer, "tcp:host=localhost,port=8888") != 0)
	{
		SetEnvironmentVariableA("DBUS_SESSION_BUS_ADDRESS", "tcp:host=localhost,port=8888");
		_putenv("DBUS_SESSION_BUS_ADDRESS=tcp:host=localhost,port=8888");
	}
#endif

    ///init dbus 
    if(!DbusOperate::getInstance()->init(token,msgProc,srvList))
    {
        LIBDBUS_COUT << "init failed" <<  LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }
    LIBDBUS_COUT << "init finished " << LIBDBUS_ENDL;

    ///create system config
    if(DbusOperate::getInstance()->creatAllSystemBusConf() != LIBDBUSRET_SOK)
    {
        LIBDBUS_COUT << "creatAllSystemBusConf failed" <<  LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }
    LIBDBUS_COUT << "creatAllSystemBusConf finished " << LIBDBUS_ENDL;

    ///register system dbus
    if(DbusOperate::getInstance()->regSysDbus() != LIBDBUSRET_SOK)
    {
        LIBDBUS_COUT << "regSysDbus failed " <<  LIBDBUS_ENDL;
        DbusOperate::getInstance()->terminate();//stop thread
        return LIBDBUSRET_EGENERAL;
    }
    LIBDBUS_COUT << "regSysDbus finished " << LIBDBUS_ENDL;

    ///create thread
    if(DbusOperate::getInstance()->createListenDbusThread() != LIBDBUSRET_SOK)
    {
        DbusOperate::getInstance()->terminate();
        LIBDBUS_COUT << "createListenDbusThread failed" <<  LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }

    LIBDBUS_COUT << "libDbusConnectMessageBus end " << LIBDBUS_ENDL;
    connect_flag = true;
    return LIBDBUSRET_SOK;
}

void libDbusDisconnect()
{
    if(!connect_flag)
        return;
    LIBDBUS_COUT << "libDbusDisconnect start " << LIBDBUS_ENDL;
    DbusOperate::getInstance()->terminate();
    LIBDBUS_COUT << "libDbusDisconnect end " << LIBDBUS_ENDL;
    return;
}


uint32_t libDbusSendMessage(char* service, char* method, uint8_t msgType, char* inargs, int32_t argLen, char** retval, int32_t* retLen, int32_t timeout)
{
    if(!connect_flag)
        return LIBDBUSRET_ENOTCONNECT;
    return DbusOperate::getInstance()->sendMessage(service,method,msgType,inargs,argLen,retval,retLen,timeout);
}


#ifdef WIN32

#ifdef __cplusplus
extern "C" {
#endif
LIBDBUS_API void* libDbusMalloc(size_t size);
LIBDBUS_API void libDbusFree(void *mem);
#ifdef __cplusplus
}
#endif

void libDbusFree(void *mem)
{
	if (mem != NULL)
	{
		free(mem);
		mem = NULL;
	}
}
void* libDbusMalloc(size_t size)
{
	return (void*)malloc(size);
}
#endif