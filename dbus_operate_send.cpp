#include "dbus_operate.h"


uint32_t DbusOperate::sendMessage(char *service, char *method, uint8_t msg_type, char *inargs, int32_t arg_len, char **retval, int32_t *ret_len, int32_t timeout)
{
    LIBDBUS_DEBUG <<" [ send ]start sendMessage" << LIBDBUS_ENDL;
    if(service == NULL || strlen(service) <= 0 || method == NULL || strlen(method) <= 0)
    {
        LIBDBUS_COUT <<" [ send ]service or method is null" << LIBDBUS_ENDL;
        return LIBDBUSRET_EPARAMETER;
    }

    if(msg_type == BUS_NOTICE)//
    {
        LIBDBUS_DEBUG <<" [ send ]msg is NOTICE" << LIBDBUS_ENDL;
        uint32_t ret = sendSingalMsg(service,method,inargs,arg_len,timeout);
        return ret;
    }
    else if(msg_type == BUS_REQUEST)//
    {
        LIBDBUS_DEBUG <<" [ send ]msg is BUS_REQUEST" << LIBDBUS_ENDL;
        uint32_t ret = sendMethodMsg(service,method,inargs,arg_len,retval,ret_len,timeout);
        return ret;
    }
    else
    {
        return LIBDBUSRET_EPARAMETER;
    }
    return LIBDBUSRET_SOK;
}

uint32_t DbusOperate::sendSingalMsg(char *service, char *method, char *inargs, int32_t arg_len, int32_t timeout)
{
    LIBDBUS_DEBUG <<" [ send ]start sendSingalMsg" << LIBDBUS_ENDL;
    DBusMessage *msg;
    DBusMessageIter arg;
    dbus_uint32_t  serial = 0;

    //
    DBUS_INFO bus_info_tmp;
    if(dbus_info_.size() > 0)
        bus_info_tmp = dbus_info_[0];

    //
    std::string dbus_path("/com/LibDbus/");
    dbus_path.append(service);

    LIBDBUS_DEBUG <<" [ send ]send path:" << dbus_path << LIBDBUS_ENDL;

    //
    //
    if((msg = dbus_message_new_signal(dbus_path.c_str(), "com.LibDbus.interface", method))== NULL){
        LIBDBUS_COUT << " [ send ]dbus_message_new_signal is NULL" << LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }
    LIBDBUS_DEBUG <<" [ send ]new signal mesg ok" << LIBDBUS_ENDL;

    do{

        //
        dbus_message_iter_init_append(msg, &arg);

        //
        if(!dbus_message_iter_append_basic(&arg, DBUS_TYPE_STRING, &inargs)){
            LIBDBUS_COUT <<" [ send ]Out OfMemory!" << LIBDBUS_ENDL;
            break;
        }
        if(!dbus_message_iter_append_basic(&arg, DBUS_TYPE_UINT32, &arg_len)){
            LIBDBUS_COUT <<" [ send ]Out OfMemory!" << LIBDBUS_ENDL;
            break;
        }

        //
        if(!dbus_connection_send(connect_send_, msg, &serial)){
            LIBDBUS_COUT <<" [ send ]Out of Memory!" << LIBDBUS_ENDL;
            break;
        }
        LIBDBUS_DEBUG << " dbus_connection_flush1 start" << LIBDBUS_ENDL;
        dbus_connection_flush(connect_send_);
        LIBDBUS_DEBUG << " dbus_connection_flush1 end" << LIBDBUS_ENDL;
        LIBDBUS_DEBUG <<" [ send ]Signal Send" << LIBDBUS_ENDL;
        dbus_message_unref(msg);
        return LIBDBUSRET_SOK;
    }while(0);

    //释放相关的分配的内存。
    dbus_message_unref(msg);
    return LIBDBUSRET_EGENERAL;
}

uint32_t DbusOperate::sendMethodMsg(char *service, char *method, char *inargs, int32_t arg_len, char **retval, int32_t *ret_len, int32_t timeout)
{

    DBusMessage *msg;
    DBusMessageIter arg;
    DBusPendingCall *pending;
    char *ret_msg = NULL;
    int32_t ret_msg_len = 0;
    int32_t nret_stat = 0;

    //
    std::string dbus_path("/com/LibDbus/");
    dbus_path.append(service);
    std::string dbus_dest_name("com.LibDbus.");
    dbus_dest_name.append(service);

    std::string dbus_interface("com.LibDbus.interface");

    //
    LIBDBUS_DEBUG <<" -------------------------------- [ send ]Message method:" << method << LIBDBUS_ENDL;
    msg = dbus_message_new_method_call(dbus_dest_name.c_str(), dbus_path.c_str(), dbus_interface.c_str(), method);
    if(msg == NULL){
        LIBDBUS_COUT <<" [ send ]Message NULL" << LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }

    //
    dbus_message_iter_init_append(msg, &arg);
    if(!dbus_message_iter_append_basic(&arg, DBUS_TYPE_STRING, &inargs))
    {
        LIBDBUS_COUT <<" [ send ]Out of Memory!" << LIBDBUS_ENDL;
        dbus_message_unref(msg);
        return LIBDBUSRET_EGENERAL;
    }
    if(!dbus_message_iter_append_basic(&arg, DBUS_TYPE_UINT32, &arg_len)){
        LIBDBUS_COUT <<" [ send ]Out of Memory!" << LIBDBUS_ENDL;
        dbus_message_unref(msg);
        return LIBDBUSRET_EGENERAL;
    }


    LIBDBUS_DEBUG << " dbus_connection_send_with_reply start" << LIBDBUS_ENDL;
    //
    if(!dbus_connection_send_with_reply(connect_send_, msg, &pending, timeout)){
        LIBDBUS_COUT <<" [ send ]Out of Memory!" << LIBDBUS_ENDL;
        dbus_message_unref(msg);
        return LIBDBUSRET_EGENERAL;
    }

    if(pending == NULL){
        LIBDBUS_COUT <<" [ send ]Pending Call NULL: connection is disconnected " << LIBDBUS_ENDL;
        dbus_message_unref(msg);
        return LIBDBUSRET_ECONNRESET;
    }

    LIBDBUS_DEBUG << " dbus_connection_flush2 start" << LIBDBUS_ENDL;
    dbus_connection_flush(connect_send_);
    LIBDBUS_DEBUG << " dbus_connection_flush2 end" << LIBDBUS_ENDL;
    dbus_message_unref(msg);

    //waiting a reply
    LIBDBUS_DEBUG <<" [ send ]start wait return msg..." << LIBDBUS_ENDL;
    dbus_pending_call_block(pending);
    msg = dbus_pending_call_steal_reply(pending);
    if (msg == NULL) {
        LIBDBUS_COUT <<" [ send ]Reply Null" << LIBDBUS_ENDL;
        dbus_pending_call_unref(pending);
        return LIBDBUSRET_EGENERAL;
    }

    LIBDBUS_DEBUG <<" [ send ]get return msg finished" << LIBDBUS_ENDL;
    dbus_pending_call_unref(pending);

    do{
        // read the parameters
        if(!dbus_message_iter_init(msg, &arg))
        {
            LIBDBUS_COUT <<" [ send ]Message has no arguments!" << LIBDBUS_ENDL;
            break;
        }
        else if (dbus_message_iter_get_arg_type(&arg) != DBUS_TYPE_STRING)
        {
            LIBDBUS_COUT <<" [ send ]Argument is not string!" << LIBDBUS_ENDL;
            break;
        }
        else
            dbus_message_iter_get_basic(&arg, &ret_msg);

        LIBDBUS_DEBUG <<" [ send ] ret meg content:" << ret_msg << LIBDBUS_ENDL;
        if (!dbus_message_iter_next(&arg))
        {
            LIBDBUS_COUT <<" [ send ]Message has too few arguments!" <<LIBDBUS_ENDL;
            break;
        }
        else if (dbus_message_iter_get_arg_type(&arg) != DBUS_TYPE_UINT32 )
        {
            LIBDBUS_COUT <<" [ send ]Argument is not int!" << LIBDBUS_ENDL;
            break;
        }
        else
            dbus_message_iter_get_basic(&arg, &ret_msg_len);

        if (!dbus_message_iter_next(&arg))
        {
            LIBDBUS_COUT <<" [ send ]Message has too few arguments!" <<LIBDBUS_ENDL;
            break;
        }
        else if (dbus_message_iter_get_arg_type(&arg) != DBUS_TYPE_UINT32 )
        {
            LIBDBUS_COUT <<" [ send ]Argument is not int!" << LIBDBUS_ENDL;
            break;
        }
        else
            dbus_message_iter_get_basic(&arg, &nret_stat);
        LIBDBUS_DEBUG <<" [ send ] ret stat:" << nret_stat << LIBDBUS_ENDL;
        
        //
        int len = 0; //
        if(retval != NULL)
        {
            if(ret_msg[ret_msg_len-1] == '\0')
                len = ret_msg_len;
            else
                len = ret_msg_len+1;
            *retval = (char *)malloc(len);
            memset(*retval, 0, len);
            strncpy(*retval, ret_msg,ret_msg_len);
        }
        if(ret_len != NULL)
        {
            *ret_len = (int32_t)(len);
        }
        LIBDBUS_DEBUG <<" [ send ]ret msg len:" << ret_msg_len << LIBDBUS_ENDL;
        dbus_message_unref(msg);

        return nret_stat;
    }while(0);

    dbus_message_unref(msg);
    return LIBDBUSRET_EGENERAL;
}
