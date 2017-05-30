#include <zookeeper/zookeeper.h>
#include <naemon/naemon.h>

NEB_API_VERSION(CURRENT_NEB_API_VERSION);
nebmodule *neb_handle;

static zhandle_t *zh;
const char app_name[] = "distman";
const char cluster_name[] = "master";

void watcher (zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx) {
    if (type == ZOO_CREATED_EVENT) {
        zoo_exists(zh, "/robin", 1, NULL);
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Created event!");
    } else if (type == ZOO_DELETED_EVENT) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Deleted event!");
    } else if (type == ZOO_CHANGED_EVENT) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Changed event!");
    } else if (type == ZOO_CHILD_EVENT) {
        /* Add a new watch so we can detect disconnects etc */
        zoo_get_children(zh, "/", 1, NULL);
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Child event!");

    } else if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            nm_log(NSLOG_INFO_MESSAGE, "DistMan: Connected to ZooKeeper server!");
        } else if (state == ZOO_CONNECTING_STATE) {
            nm_log(NSLOG_INFO_MESSAGE, "DistMan: Trying to connect to ZooKeeper server...");
        }
    } else if (type == ZOO_NOTWATCHING_EVENT) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Notwatching event!");
    } else {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Unknown ZooKeeper event!");
    }

    if (state == ZOO_EXPIRED_SESSION_STATE) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Expired session state");
    } else if (state == ZOO_AUTH_FAILED_STATE) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Auth failed state");
    } else if (state == ZOO_CONNECTING_STATE) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Connecting state");
    } else if (state == ZOO_ASSOCIATING_STATE) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Associating state");
    } else if (state == ZOO_CONNECTED_STATE) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Connected state");
    } else {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Unknown ZooKeeper state! %d", state);
    }
}

int handle_host_check (int event_type, void *data) {
    nebstruct_host_check_data *ds = (nebstruct_host_check_data *)data;
    if (ds->type == NEBTYPE_HOSTCHECK_INITIATE) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Overriding host check");
        return NEBERROR_CALLBACKOVERRIDE;
    }
    return NEB_OK;
}

int handle_service_check (int event_type, void *data) {
    nebstruct_service_check_data *ds = (nebstruct_service_check_data *)data;
    if (ds->type == NEBTYPE_SERVICECHECK_INITIATE) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Overriding service check");
        return NEBERROR_CALLBACKOVERRIDE;
    }
    return NEB_OK;
}

int nebmodule_init (int flags, char *arg, nebmodule *handle) {

    char *election_path = NULL;
    char buffer[1024];
    char p[2048];
    char *cert = 0;
    char appId[64];
    int rc = 0;
    int size = 512;

    neb_handle = handle;

    event_broker_options = ~0; /* force settings to "everything" */
    neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA, neb_handle, 0, handle_service_check);
    neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA, neb_handle, 0, handle_host_check);

    zh = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);

    // 1. Create necessary paths
    sprintf(buffer, "/%s", app_name);
    rc = zoo_create(zh, buffer, "value", 5, &ZOO_OPEN_ACL_UNSAFE, 0, buffer, sizeof(buffer)-1);
    switch (rc) {
    case ZOK:
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Created app path OK!");
        break;
    case ZNODEEXISTS:
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Failed to create zNode /%s because it already exists", app_name);
        break;
    case ZNONODE:
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Parent node / does not exist!");
        return -1;
    case ZNOAUTH:
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Authorization required to create zNode /%s", app_name);
        return -1;
    case ZNOCHILDRENFOREPHEMERALS:
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Cannot create children of ephemeral nodes");
        return -1;
    case ZBADARGUMENTS:
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Bad arguments to create zNode!");
        return -1;
    default:
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Some error %d!", rc);
        return -1;
    }

    // 2. Determine if I am the leader
    // 3. If yes, set leader to true
    // 4. If no, create a watch on the node I created-1

    rc = zoo_create(zh,"/xyz","value", 5, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, buffer, sizeof(buffer)-1);
    if (rc != ZOK) {
        nm_log(NSLOG_INFO_MESSAGE, "DistMan: Failed to create ZK node!");
    }

    //zoo_get_children(zh, "/", 1, NULL);
    //struct Stat stat;
    //rc = zoo_get(zh, "/test", 1, buffer, &size, NULL);
    //nm_log(NSLOG_INFO_MESSAGE, "DistMan: RC IS %d", rc);
    zoo_exists(zh, "/robin", 1, NULL);
    return 0;
}

int nebmodule_deinit (int flags, int reason)
{
    zookeeper_close(zh);
    return 0;
}
