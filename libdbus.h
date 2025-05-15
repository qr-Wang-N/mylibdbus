////////////////////////////////////////////////////////////////////////////////
/// @file    libdbus.h
/// @version 1.0
/// @date    2024/03/29
////////////////////////////////////////////////////////////////////////////////
#ifndef __LIBDBUS_H__
#define __LIBDBUS_H__

#include <stdint.h>

#ifdef _WIN32
#ifdef __LIBDBUS_LIBRARY__
#define LIBDBUS_API __declspec(dllexport)
#else
#define LIBDBUS_API __declspec(dllimport)
#endif
#else
#define LIBDBUS_API
#endif

////////////////////////////////////////////////////////////////////////////////
// 返回值定义
#define LIBDBUSRET_SOK          0x00000000 // 成功，返回true
#define LIBDBUSRET_SFALSE       0x00000001 // 成功，返回false

#define LIBDBUSRET_EGENERAL     0x80000001 // 一般错误
#define LIBDBUSRET_EUNKNOWN     0x80000001 // 未知错误
#define LIBDBUSRET_EPARAMETER   0x80000002 // 无效的参数
#define LIBDBUSRET_EPRIVILEGE   0x80000003 // 没有权限
#define LIBDBUSRET_ENOTIMPL     0x80000004 // 功能未实现
#define LIBDBUSRET_ETIMEOUT     0x80000005 // 超时
#define LIBDBUSRET_EBUSY        0x80000006 // 设备忙碌
#define LIBDBUSRET_ENOTEXISTS   0x80000010 // 目标不存在
#define LIBDBUSRET_EEXISTS      0x80000011 // 目标已存在
#define LIBDBUSRET_ENETWORK     0x80000012 // 网络错误
#define LIBDBUSRET_ENOTCONNECT  0x80001001 // 尚未连接到消息总线
#define LIBDBUSRET_ECONNRESET   0x80001002 // 与消息总线的连接已断开


////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

// 在子系统连接到消息引擎时提供此回调函数，当接收到来自消息总线分发的消息时，SDK将调用此
// 回调函数以将消息传递给子系统进行处理。
//注意：
// 1. 如果子系统在调用connectMessageBus时向消息总线注册了自己提供的服务，此处可能会收
// 到“请求”或者”“通知”消息，否则只会收到“应答”消息。
// 2. 当子系统收到一个“请求”消息时（子系统只会收到与自己注册的服务名称相符的“请求”或者“
// 通知”消息），对此消息进行处理，并在retval中返回对应的“应答”消息载荷，通知消息不需要返
// 回消息载荷。
// 3. 此函数是在独立的线程上下文中被调用。
//参数说明:
// [in]  sender ：消息发送者的身份令牌，其类型为UTF8编码的字符串，以’\0’结尾， 具体
// 的内容由基础平台厂商提供。用于唯一标识一个子系统。
// [in]  msgType: 消息类型，1=请求，2=应答，3=响应。
// [in]  service: 服务名称，与消息关联的服务名称，UTF8编码的字符串，以’\0’结尾。
// [in]  method : 方法名称，与消息关联的方法名称，UTF8编码的字符串，以’\0’结尾。
// [in]  inargs : 消息载荷，UTF8编码的JSON对象（字符串），以’\0’结尾。如果消息没有
// 载荷，inargs的值为NULL。
// [in]  argLen : 载荷长度，单位为字节（不计算结尾的’\0’），如果消息没有载荷，
// argLen的值为0。
// [out] retval : 应答载荷（只有当msgType为1时有效），其内容为UTF8编码的JSON对象
// （对请求的处理结果），其内存空间由回调函数分配，SDK将会使用标准C的free函数释放此
// 内存空间。如果消息没有应答载荷，回调函数应该变*retval的值设置为NULL。
// [out] retLen : 应答载荷长度，单位为字节（不计算结尾的’\0’）。如果消息没有应答载荷，
// 回调函数应该吧*retLen的值设置为0。
// 返回值：
// 正常应该返回0，错误返回0x80000001

typedef uint32_t (*MessageProc)(char    * sender ,
                                uint8_t   msgType,
                                char    * service,
                                char    * method ,
                                char    * inargs ,
                                int32_t   argLen ,
                                char   ** retval ,
                                int32_t * retLen );

////////////////////////////////////////////////////////////////////////////////
// 子系统调用此函数以连接到消息总线。
// [in]token  ：子系统身份令牌，其类型为UTF8编码的字符串，以’\0’结尾， 具体的内容由基础
// 平台厂商提供。用于唯一标识一个子系统。
// [in]msgProc：消息回调函数，如果子系统并未提供任何服务此处可以为NULL。
// [in]srvList：子系统提供的服务列表，其类型为UTF8编码的字符串。如果子系统提供了多个服务，
// 每个服务名称之间以‘#’字符分割，如”ServiceName1#ServiceName2#ServiceName3”。
LIBDBUS_API int32_t libDbusConnectMessageBus(char       * token  ,
                                     MessageProc  msgProc,
                                     char       * srvList);

////////////////////////////////////////////////////////////////////////////////
// 断开与消息总线的连接
LIBDBUS_API void libDbusDisconnect();

////////////////////////////////////////////////////////////////////////////////
// 发送一次调用请求，获取应答
// [in]  service：消息关联的服务名称，UTF8编码的字符串，以‘\0’结尾
// [in]  method ：消息关联的方法名称，UTF8编码的字符串，以‘\0’结尾
// [in]  msgType：消息类型，只能是“请求”或者“通知”。
// 如果消息类型为“请求”，sendMessage将会阻塞，直到超时或者接收到应答消息后才会返回；
// 如果消息类型为“通知”，sendMessage将会立即返回
// [in]  inargs ：要发送的消息载荷，UTF8编码的JSON对象，以‘\0’结尾
// [in]  argLen ：要发送的消息载荷长度（字节），不计算结尾‘\0’
// [out] retval ：接收到的应答消息载荷（如果发送的消息类型为“通知”，此值为NULL），
// UTF8编码的JSON对象，以‘\0’结尾。
// 其内存空间由SDK用malloc进行分配，调用者应该使用free释放。
// [out] retLen ：接收到的应答消息载荷长度（单位为字节），不计算结尾的’\0’
// [in]  timeout：等待“应答”消息的超时时间，单位为毫秒
LIBDBUS_API uint32_t libDbusSendMessage(char    * service,
                                char    * method ,
                                uint8_t   msgType,
                                char    * inargs ,
                                int32_t   argLen ,
                                char   ** retval ,
                                int32_t * retLen ,
                                int32_t   timeout);

#ifdef __cplusplus
}
#endif

#endif//__LIBDBUS_H__
