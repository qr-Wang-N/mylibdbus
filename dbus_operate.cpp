#include "dbus_operate.h"


typedef struct thread_params
{
    DbusOperate * dbus_operate_;
    DBUS_INFO dbus_info_;
}THREAD_PARAMS;

DbusOperate*  DbusOperate::instance_;
bool  DbusOperate::terminated_;
MessageProc DbusOperate::call_proc_;
DBusConnection * DbusOperate::connect_send_;
pthread_t DbusOperate::rpcclient_thread_id_;

DbusOperate::DbusOperate()
{
    call_proc_ = NULL;
    srv_list_name_ = "";
    terminated_ = false;
    connect_send_ = NULL;
    self_exe_name_ = "";
    rpcclient_thread_id_ = 0;
}

DbusOperate *DbusOperate::getInstance()
{
    if(instance_ == NULL)
    {
        instance_ = new DbusOperate();
    }
    return instance_;
}
#ifdef WIN32
DWORD WINAPI DbusOperate::worker(LPVOID arg)
#else
void *DbusOperate::worker(void *arg)
#endif
{
    THREAD_PARAMS* pThread = (THREAD_PARAMS*)arg;
    LIBDBUS_COUT <<" worker start" << LIBDBUS_ENDL;
    pThread->dbus_operate_->run(pThread->dbus_info_.connect_,pThread->dbus_info_.dbus_path_);
    LIBDBUS_COUT <<" worker run end" << LIBDBUS_ENDL;
#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
    
}

void DbusOperate::terminate()
{
    terminated_ = true;
    if (rpcclient_thread_id_ != 0)
    {
        LIBDBUS_COUT <<" wait thread quit" << LIBDBUS_ENDL;
#ifdef WIN32
		WaitForSingleObject(rpcclient_thread_id_, INFINITE);
		CloseHandle(rpcclient_thread_id_);
#else
		pthread_join(rpcclient_thread_id_, NULL);
#endif
        LIBDBUS_COUT <<" thread quit" << LIBDBUS_ENDL;
        rpcclient_thread_id_ = 0;
    }

    if(dbus_info_.size() > 0 && dbus_info_[0].connect_ != NULL)
    {
        LIBDBUS_COUT << "start release rec dbus name" << LIBDBUS_ENDL;
        int ret = dbus_bus_release_name(dbus_info_[0].connect_,dbus_info_[0].dbus_name_.c_str(),NULL);
        LIBDBUS_COUT << "rec dbus_bus_release_name ret:" << ret << LIBDBUS_ENDL;
        dbus_connection_close(dbus_info_[0].connect_);
        dbus_connection_unref(dbus_info_[0].connect_);
        dbus_info_[0].connect_ = NULL;
        LIBDBUS_COUT << "release end" << LIBDBUS_ENDL;
    }

    if(connect_send_ != NULL && dbus_info_.size() > 0)
    {
        LIBDBUS_COUT << "start release send dbus name" << LIBDBUS_ENDL;
        std::string send_name = dbus_info_[0].dbus_name_;
        send_name.append("_send");
        int ret = dbus_bus_release_name(connect_send_,send_name.c_str(),NULL);
        LIBDBUS_COUT << "send dbus_bus_release_name ret:" << ret << LIBDBUS_ENDL;
        dbus_connection_close(connect_send_);
        dbus_connection_unref(connect_send_);
        connect_send_ = NULL;
        LIBDBUS_COUT << "release end" << LIBDBUS_ENDL;
    }
}

bool DbusOperate::init(char* token,MessageProc proc,char *srv_list)
{
    if(srv_list != NULL)
        srv_list_name_ = std::string(srv_list,strlen(srv_list));
    if(srv_list_name_.empty())
        srv_list_name_ = BaseFun::getInstance()->getSelfProcessName();
    call_proc_ = proc;
    self_exe_name_ = BaseFun::getInstance()->getSelfProcessName();
    LIBDBUS_COUT <<" dbus name: " << srv_list_name_ <<LIBDBUS_ENDL;

    if(srv_list_name_.empty())
    {
        LIBDBUS_COUT <<" dbus name is empty" <<LIBDBUS_ENDL;
        return false;
    }

    terminated_ = false;
    dbus_info_.clear();

    std::vector<std::string> list_name = BaseFun::getInstance()->sepString(srv_list_name_,"#");
    for(int i =0; i < list_name.size(); i++)
    {
        DBUS_INFO info_tmp;
        info_tmp.connect_ = NULL;
        info_tmp.dbus_name_ = "com.LibDbus.";
        info_tmp.dbus_name_.append(list_name[i]);
        info_tmp.dbus_path_ = "/com/LibDbus/";
        info_tmp.dbus_path_.append(list_name[i]);
        dbus_info_.push_back(info_tmp);
        LIBDBUS_DEBUG <<" set dbus name:" << info_tmp.dbus_name_ << " | dbus path:" << info_tmp.dbus_path_ << LIBDBUS_ENDL;
    }

    LIBDBUS_COUT <<" lidbus.so: init end dbus count:" << dbus_info_.size() << LIBDBUS_ENDL;
    return true;
}

uint32_t DbusOperate::creatAllSystemBusConf()
{
    if(dbus_info_.size() <= 0)
    {
        LIBDBUS_COUT << "dbus info is empty" << LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }
    //
    if(creatSystemBusConf(dbus_info_[0].dbus_name_) != LIBDBUSRET_SOK)
    {
        LIBDBUS_COUT << "create bus conf fail" << LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }
    //
    std::string dbus_send_name = dbus_info_[0].dbus_name_;
    dbus_send_name.append("_send");
    if(creatSystemBusConf(dbus_send_name) != LIBDBUSRET_SOK)
    {
        LIBDBUS_COUT << "create send bus conf fail" << LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }
    return LIBDBUSRET_SOK;
}

////etc/dbus-1/system.d/
///com.LibDbus.[srv_list_name_].conf
uint32_t DbusOperate::creatSystemBusConf(const std::string& dbus_name)
{
#ifndef WIN32
	if (dbus_name.empty())
		return LIBDBUSRET_EGENERAL;

	if (getuid() != 0)//
	{
		LIBDBUS_COUT << "this process is normal user" << LIBDBUS_ENDL;
		return LIBDBUSRET_SOK;
	}

	TiXmlDocument doc;

	//
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", "");
	doc.LinkEndChild(decl);

	//
	TiXmlElement* root = new TiXmlElement("busconfig");
	doc.LinkEndChild(root);

	{
		//
		TiXmlComment* comment = new TiXmlComment();
		comment->SetValue("Only root can own the service");
		root->LinkEndChild(comment);
	}

	{
		//
		TiXmlElement* policy = new TiXmlElement("policy");
		policy->SetAttribute("user", "root");
		//
		TiXmlElement* allow1 = new TiXmlElement("allow");
		allow1->SetAttribute("own", dbus_name.c_str());
		TiXmlElement* allow2 = new TiXmlElement("allow");
		allow2->SetAttribute("send_interface", "com.Kylin.interface");
		policy->LinkEndChild(allow1);
		policy->LinkEndChild(allow2);
		root->LinkEndChild(policy);
	}

	{
		//
		TiXmlComment* comment = new TiXmlComment();
		comment->SetValue("Allow anyone to invoke methods on the interfaces");
		root->LinkEndChild(comment);
	}
	{
		//
		TiXmlElement* policy = new TiXmlElement("policy");
		policy->SetAttribute("context", "default");
		//
		TiXmlElement* allow1 = new TiXmlElement("allow");
		allow1->SetAttribute("send_interface", "com.Kylin.interface");
		TiXmlElement* allow2 = new TiXmlElement("allow");
		allow2->SetAttribute("send_destination", "com.Kylin.interface");
		allow2->SetAttribute("send_interface", "org.freedesktop.DBus.Properties");
		TiXmlElement* allow3 = new TiXmlElement("allow");
		allow3->SetAttribute("send_destination", "com.Kylin.interface");
		allow3->SetAttribute("send_interface", "org.freedesktop.DBus.Introspectable");
		policy->LinkEndChild(allow1);
		policy->LinkEndChild(allow2);
		policy->LinkEndChild(allow3);
		root->LinkEndChild(policy);
	}

	std::string savefile_tmp = "/etc/dbus-1/system.d/";
	savefile_tmp.append(dbus_name).append(".conf");
	if (doc.SaveFile(savefile_tmp.c_str())) //
	{
		//
		std::string cmd = "sed -i '2i \\";
		cmd.append("<!DOCTYPE busconfig PUBLIC \\")
			.append("\n\"-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN\"\\")
			.append("\n\"http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd\">' ")
			.append(savefile_tmp);
		system(cmd.c_str());
	}
#endif
    
    return LIBDBUSRET_SOK;
}

uint32_t DbusOperate::regSysDbus()
{
    LIBDBUS_COUT <<" regSysDbus start" << LIBDBUS_ENDL;

    if(dbus_info_.size() <= 0)
    {
        LIBDBUS_COUT << "dbus info is empty" << LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }

    
    //
    std::string dbus_send_name = dbus_info_[0].dbus_name_;
    dbus_send_name.append("_send");
    LIBDBUS_COUT << "dbus send name: " << dbus_send_name << LIBDBUS_ENDL;
    if(initDbusConnection(&connect_send_,dbus_send_name) != LIBDBUSRET_SOK)
        return LIBDBUSRET_EGENERAL;
	
    //
    if(initDbusConnection(&(dbus_info_[0].connect_),dbus_info_[0].dbus_name_) != LIBDBUSRET_SOK)
        return LIBDBUSRET_EGENERAL;

    //
    DBusError dbus_err;
    dbus_error_init(&dbus_err);
    dbus_bus_add_match(dbus_info_[0].connect_, "type='signal', interface='com.Kylin.interface'", &dbus_err);
    if(dbus_error_is_set(&dbus_err)){
        LIBDBUS_COUT <<" Match Error:" << dbus_err.message << LIBDBUS_ENDL;
        dbus_error_free(&dbus_err);
        terminate();
        return LIBDBUSRET_EGENERAL;
    }
    dbus_error_free(&dbus_err);
    dbus_connection_flush(dbus_info_[0].connect_);
    LIBDBUS_COUT <<" regSysDbus end" << LIBDBUS_ENDL;
	
    return LIBDBUSRET_SOK;
}

uint32_t DbusOperate::createListenDbusThread()
{
    if(dbus_info_.size() <= 0)
    {
        LIBDBUS_COUT << "dbus info is empty" << LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }

    //
    static THREAD_PARAMS thread_param;
    thread_param.dbus_operate_ = this;
    thread_param.dbus_info_.connect_ = dbus_info_[0].connect_;
    thread_param.dbus_info_.dbus_name_ = dbus_info_[0].dbus_name_;
    thread_param.dbus_info_.dbus_path_ = dbus_info_[0].dbus_path_;
    LIBDBUS_COUT <<" thread_param.dbus_info_.dbus_name_:" << thread_param.dbus_info_.dbus_name_.c_str() << LIBDBUS_ENDL;
#ifdef WIN32
	rpcclient_thread_id_ = CreateThread(NULL, 0, worker, &thread_param, 0, NULL);
#else
	if (pthread_create(&rpcclient_thread_id_, NULL, worker, &thread_param) != 0)
	{
		return LIBDBUSRET_EGENERAL;
	}
#endif
    
    LIBDBUS_COUT <<" thread_param.dbus_info_.dbus_name_ end:" << thread_param.dbus_info_.dbus_name_.c_str() << LIBDBUS_ENDL;
    return LIBDBUSRET_SOK;
}

uint32_t DbusOperate::initDbusConnection(DBusConnection **conn, const std::string &dbus_name)
{
    if(*conn != NULL)
    {
        LIBDBUS_COUT << "dbus already connection" << LIBDBUS_ENDL;
        return LIBDBUSRET_SOK;
    }

    DBusError dbus_err;
    dbus_error_init(&dbus_err);

    //
    LIBDBUS_COUT << "start dbus_bus_get_private" << LIBDBUS_ENDL;
#ifdef WIN32
	*conn = dbus_bus_get_private(DBUS_BUS_SESSION, &dbus_err);
#else
	*conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &dbus_err);
#endif
    
    if(dbus_error_is_set(&dbus_err)){
        LIBDBUS_COUT <<" dbus_bus_get_private error: " << dbus_err.message << LIBDBUS_ENDL;
        dbus_error_free(&dbus_err);
        return LIBDBUSRET_EGENERAL;
    }
    if(*conn == NULL)
    {
        LIBDBUS_COUT <<" connect is null " <<LIBDBUS_ENDL;
        return LIBDBUSRET_EGENERAL;
    }

    ///
    int has_owner = dbus_bus_name_has_owner(*conn,dbus_name.c_str(),NULL);
    if(has_owner)
    {
        LIBDBUS_COUT << "dbus name has an owner." << LIBDBUS_ENDL;
    }
    else
    {
        LIBDBUS_COUT << "dbus name does not have an owner." << LIBDBUS_ENDL;
    }

    int ret = dbus_bus_request_name(*conn, dbus_name.c_str(),
                                DBUS_NAME_FLAG_REPLACE_EXISTING, &dbus_err);

    LIBDBUS_COUT << " dbus_bus_request_name ret: " << ret << LIBDBUS_ENDL;

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER
            && ret != DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER)
    {
        if (dbus_error_is_set(&dbus_err))
            LIBDBUS_COUT << "dbus_bus_request_name Error: " << dbus_err.message << LIBDBUS_ENDL;

        //
        ret = dbus_bus_release_name(*conn,dbus_name.c_str(),NULL);
        LIBDBUS_COUT << "dbus_bus_release_name ret:" << ret << LIBDBUS_ENDL;
        dbus_error_free(&dbus_err);
        dbus_connection_close(*conn);
        dbus_connection_unref(*conn);
        *conn = NULL;
        return LIBDBUSRET_EGENERAL;
    }
    dbus_error_free(&dbus_err);
    LIBDBUS_COUT << "connect send bus name:" << dbus_bus_get_unique_name(*conn) << LIBDBUS_ENDL;
    return LIBDBUSRET_SOK;
}


