#include <zookeeper/zookeeper.h>
#include <naemon/naemon.h>

NEB_API_VERSION(CURRENT_NEB_API_VERSION);
nebmodule *neb_handle;

static zhandle_t *zh;

void watcher(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx) {
    nm_log(NSLOG_INFO_MESSAGE, "ZooKeeper event");
}

int nebmodule_init (int flags, char *arg, nebmodule *handle) {

	event_broker_options = ~0; /* force settings to "everything" */
    /*
    neb_register_callback(NEBCALLBACK_PROCESS_DATA, neb_handle, 0, iobroker_initializer);
    neb_register_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, neb_handle, 0, external_command_handle);
    neb_register_callback(NEBCALLBACK_HOST_STATUS_DATA, dm_neb_handle, 0, test_func);
    neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA, dm_neb_handle, 0, test_func);
    */

    zh = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);

	return 0;
}

int nebmodule_deinit (__attribute__((unused)) int flags, __attribute__((unused)) int reason)
{
    zookeeper_close(zh);
	return 0;
}
