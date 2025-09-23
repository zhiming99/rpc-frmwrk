const arrBasePoints = ["cpu_load", 
    "vmsize_kb", "obj_count", "req_count",
    "uptime", "offline_times", "cmdline",
    "open_files", "working_dir",
    "rx_bytes", "tx_bytes",
]

const arrRouterPoints = arrBasePoints.concat( [
    "max_conn", "sess_time_limit", "max_send_bps", "max_recv_bps",
]);

const arrAppPoints = arrBasePoints.concat( [
    "resp_count", "failure_count", "pending_tasks",
    "max_streams_per_session"
])

function fetchAppDetails()
{
    if( !globalThis.curSpModal || ! globalThis.oProxy )
        return Promise.resolve(0)
    try{
        const app = globalThis.curSpModal.currentApp;
        // Fetch and display app details for the selected app
        const oProxy = globalThis.oProxy;
        if( app.setpoints && app.setpoints.size > 0 )
            return Promise.resolve(0);
        var oContext = {};
        app.setpoints = new Map();
        oContext.oRegListenerCb = (oContext, ret) => {
            if( oContext.m_iRet < 0 )
                console.log( "Errror, RegisterListener failed with status " + oContext.m_iRet );
            else
                console.log( "RegisterListener succeeded with status " + oContext.m_iRet );
        };
        return oProxy.RegisterListener( oContext, [app.name] ).then((ret) => {
            oContext.oGetPvsCb = (oContext, ret, arrKeyVals) => {
                if( oContext.m_iRet < 0 )
                    return 
                // Process the retrieved point values
                for( var i = 0; i < arrKeyVals.length; i++ ){
                    const item = arrKeyVals[i];
                    app.setpoints.set( item.strKey, item.oValue.m_val );
                }
                return
            };
            var arrPts = arrAppPoints
            if( app.name === "rpcrouter1")
                arrPts = arrRouterPoints
            return oProxy.GetPointValues( oContext, app.name, arrPts ).then((ret)=>{
                if( oContext.m_iRet < 0 )
                    return Promise.reject( oContext.m_iRet );
                return Promise.resolve( 0 );
            }).catch((error) => {
                console.log("Error GetPointValues:", error);
                return Promise.resolve(error);
            });
        }).catch((error) => {
            console.log("Error RegisterListener:", error);
            return Promise.resolve(error);
        })

    }catch(e){
        console.error("Error fetching app details:", e);
        return Promise.resolve(e);
    }
}

globalThis.fetchAppDetails = fetchAppDetails;
