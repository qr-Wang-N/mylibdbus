#include "dbus_operate.h"


void DbusOperate::run(DBusConnection* conn,const std::string& dbus_path)
{
    DBusMessage *msg = NULL;
    while(!terminated_)
    {
        dbus_connection_read_write(conn, 0);       //ok
        msg = dbus_connection_pop_message(conn);    //ok
        if(msg == NULL){
            //LIBDBUS_DEBUG <<" msg is null continue" << LIBDBUS_ENDL;
#ifdef WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
            
            continue;
        }
        LIBDBUS_DEBUG <<" self dbus_path: " << dbus_path << LIBDBUS_ENDL;
        const char* rec_msg_path = dbus_message_get_path(msg);
        if(rec_msg_path == NULL)
        {
            LIBDBUS_DEBUG << " receive mag path is null" << LIBDBUS_ENDL;
            dbus_message_unref(msg);
#ifdef WIN32
			Sleep(100);
#else
			usleep(100000);
#endif
            continue;
        }
        else
            LIBDBUS_DEBUG << " msg path:" << rec_msg_path << LIBDBUS_ENDL;
        if(strcmp(rec_msg_path, dbus_path.c_str()) == 0)
        {
            LIBDBUS_DEBUG <<" receive msg..." << LIBDBUS_ENDL;
            replyToMessage(conn,msg,dbus_message_get_type(msg));
        }
        dbus_message_unref(msg);
    }
    LIBDBUS_COUT << " thread run quit" << LIBDBUS_ENDL;
}


//
void DbusOperate::replyToMessage(DBusConnection* conn, DBusMessage *msg, int msg_type)
{
    //dbus_uint32_t serial = 0;
    if(msg_type != DBUS_MESSAGE_TYPE_SIGNAL && msg_type != DBUS_MESSAGE_TYPE_METHOD_CALL)
    {
        LIBDBUS_COUT <<" [ receive ]msg type is: " << msg_type  << " not singal or method_call!"<< LIBDBUS_ENDL;
        return;
    }

    //
    const char* sender = dbus_message_get_sender(msg);
    if(sender == NULL)
    {
        LIBDBUS_COUT <<" [ receive ]sender is null" << LIBDBUS_ENDL;
        return;
    }
    LIBDBUS_DEBUG <<" [ receive ]sender:" << sender << LIBDBUS_ENDL;

    //
    const char* method_name = dbus_message_get_member(msg);
    if(method_name == NULL)
    {
        LIBDBUS_COUT <<" [ receive ]method_name is null" << LIBDBUS_ENDL;
        return;
    }
    LIBDBUS_DEBUG <<" [ receive ]method_name:" << method_name << LIBDBUS_ENDL;

    //
    char service_name[128] = {0};
    const char* service_path = dbus_message_get_path(msg);
    if(service_path == NULL)
    {
        LIBDBUS_COUT <<" [ receive ]service_path is null" << LIBDBUS_ENDL;
        return;
    }
    std::string str_tmp(service_path,strlen(service_path));
    str_tmp = str_tmp.substr(str_tmp.rfind('/')+1);
    snprintf(service_name,128,"%s",str_tmp.c_str());
    LIBDBUS_DEBUG <<" [ receive ]service_name:" << service_name << LIBDBUS_ENDL;

    //
    char *param = NULL;
    DBusMessageIter arg;
    int msg_content_len = 0;
    if(!dbus_message_iter_init(msg, &arg))
    {
        LIBDBUS_DEBUG <<" [ receive ]Message has noargs" << LIBDBUS_ENDL;
        return;
    }
    else if(dbus_message_iter_get_arg_type(&arg) != DBUS_TYPE_STRING)
    {
        LIBDBUS_DEBUG <<" [ receive ]arg type is not string" << LIBDBUS_ENDL;
        return;
    }
    else
    {
        dbus_message_iter_get_basic(&arg, &param);
    }

    //
    if (!dbus_message_iter_next(&arg))
    {
        LIBDBUS_DEBUG <<" [ receive ]Message hastoo few arguments!" << LIBDBUS_ENDL;
        return;
    }
    else if (dbus_message_iter_get_arg_type(&arg) != DBUS_TYPE_UINT32 )
    {
        LIBDBUS_DEBUG <<" [ receive ]Argument is not int!" << LIBDBUS_ENDL;
        return;
    }
    else
        dbus_message_iter_get_basic(&arg, &msg_content_len);


    LIBDBUS_DEBUG <<" [ receive ]sender:" << sender << " | msg_type:" << msg_type << " | method_name:" << method_name << " | dbus path:" << service_path
                << " | msg len:" << msg_content_len << " | msg content:" << param << LIBDBUS_ENDL;


    DBusMessage *reply = NULL;
    dbus_uint32_t serial = 0;
    //
    char* return_msg = NULL;
    int return_msg_len = 0;
    int n_call_ret = 0;
    if(call_proc_ != NULL)
    {
        int retlen = 0;
        char* ret_value = NULL;
        if(msg_type == DBUS_MESSAGE_TYPE_SIGNAL)//
        {
            LIBDBUS_DEBUG <<" [ receive ]mesg is DBUS_MESSAGE_TYPE_SIGNAL" << LIBDBUS_ENDL;
            call_proc_((char*)sender,2,service_name,(char*)method_name,param,msg_content_len,NULL,&retlen);
            LIBDBUS_DEBUG <<" [ receive ]mesg is DBUS_MESSAGE_TYPE_SIGNAL proc end" << LIBDBUS_ENDL;
            return;
        }
        else
        {
            LIBDBUS_DEBUG <<" [ receive ]mesg is DBUS_MESSAGE_TYPE_METHOD_CALL" << LIBDBUS_ENDL;
            n_call_ret = call_proc_((char*)sender,1,service_name,(char*)method_name,param,msg_content_len,&ret_value,&retlen);
            LIBDBUS_DEBUG <<" [ receive ]mesg is DBUS_MESSAGE_TYPE_METHOD_CALL end" << LIBDBUS_ENDL;
        }

        if(retlen > 0)
        {
            return_msg = (char*)malloc(retlen+1);
            memset(return_msg,0,retlen+1);
            return_msg_len = retlen;
            strncpy(return_msg,ret_value,retlen);
            if(ret_value)
                free(ret_value);
        }
        else
        {//////////////
            return_msg = (char*)malloc(15);
            memset(return_msg,0,15);
            strncpy(return_msg,"get info fail!",15);
            return_msg_len = strlen("get info fail!");
            LIBDBUS_DEBUG <<" [ receive ]return_msg:" << return_msg << LIBDBUS_ENDL;
        }
        LIBDBUS_DEBUG <<" [ receive ] call proc ret len:" << retlen << " |ret content: " << return_msg << " |ret stat: " << n_call_ret <<LIBDBUS_ENDL;
    }
    else
        return;

    //
    reply = dbus_message_new_method_return(msg);
    do{
        if(reply == NULL)
        {
            LIBDBUS_COUT << " [ receive ] dbus_message_new_method_return false" << LIBDBUS_ENDL;
            break;
        }
        //
        dbus_message_iter_init_append(reply, &arg);
        if(!dbus_message_iter_append_basic(&arg, DBUS_TYPE_STRING, &return_msg)){
            LIBDBUS_COUT <<" [ receive ]Out ofMemory!" << LIBDBUS_ENDL;
            break;
        }
        if(!dbus_message_iter_append_basic(&arg, DBUS_TYPE_UINT32, &return_msg_len)){
            LIBDBUS_COUT <<" [ receive ]Out ofMemory!" << LIBDBUS_ENDL;
            break;
        }
        if(!dbus_message_iter_append_basic(&arg, DBUS_TYPE_UINT32, &n_call_ret)){
            LIBDBUS_COUT <<" [ receive ]Out ofMemory!" << LIBDBUS_ENDL;
            break;
        }

        //
        if(!dbus_connection_send(conn, reply, &serial)){
            LIBDBUS_COUT <<" [ receive ]Out of Memory" << LIBDBUS_ENDL;
            break;
        }

        LIBDBUS_DEBUG << " [ receive ]dbus_connection_flush0 start" << LIBDBUS_ENDL;
        dbus_connection_flush(conn);
        LIBDBUS_DEBUG <<" [ receive ]dbus_connection_flush0 end" << LIBDBUS_ENDL;

    }while(0);

    if(reply != NULL)
        dbus_message_unref(reply);
    if(return_msg != NULL)
    {
        free(return_msg);
        return_msg = NULL;
    }
    LIBDBUS_DEBUG <<" [ receive ]replyToMessage end" << LIBDBUS_ENDL;
    return;
}
