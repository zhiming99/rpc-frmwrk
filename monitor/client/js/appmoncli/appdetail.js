const arrBasePoints = ["cpu_load", 
    "vmsize_kb", "obj_count", "req_count",
    "uptime", "offline_times", "cmdline",
    "open_files", "working_dir"
]

const arrRouterPoints = arrBasePoints.concat( [
    "max_conn_count", "rx_bytes", "tx_bytes",
    "session_time_limit", "max_send_bps", "max_recv_bps",
    "max_streams_per_session"
]);

const arrAppPoints = arrBasePoints.concat( [
    "resp_count", "failure_count", "pending_tasks",
])

function fetchAppDetails()
{
    if( !globalThis.curSpModal || ! globalThis.oProxy )
        return Promise.reject(-14);
    try{
        const app = globalThis.curSpModal.currentApp;
        // Fetch and display app details for the selected app
        const oProxy = globalThis.oProxy;
        if( app.setpoints && app.setpoints.length > 0 )
            return Promise.resolve(0);
        var oContext = {};
        return oProxy.RegisterListener( oContext, [app.name] ).then((ret) => {
            if( ret >= 0x80000000 )
                return Promise.reject(ret)
            if( app.name === "rpcrouter1 ")
                return oProxy.GetPointValues( oContext, app.name, arrRouterPoints )
            oContext.oGetPvsCb = (oContext, ret, arrKeyVals) => {
                if( ret >= 0x80000000 )
                    return Promise.reject(ret)
                // Process the retrieved point values
                app.setpoints = arrKeyVals.map(item => {
                    return {
                        name: item.strKey,
                        value: item.oValue.m_ival
                    };
                });
            };
            return oProxy.GetPointValues( oContext, app.name, arrAppPoints )
        }).catch((error) => {
            console.log("Error RegisterListener:", error);
            return Promise.reject(-14);
        })

    }catch(e){
        console.error("Error fetching app details:", e);
        return Promise.reject(-14);
    }
}

globalThis.fetchAppDetails = fetchAppDetails;
