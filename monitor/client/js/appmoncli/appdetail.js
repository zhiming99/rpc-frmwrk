const arrBasePoints = ["cpu_load", 
    "vmsize_kb", "obj_count", "req_count",
    "uptime", "offline_times", "cmdline",
    "open_files", "working_dir",
    "rx_bytes", "tx_bytes", "pid", "conn_count"
]

const arrRouterPoints = arrBasePoints.concat( [
    "max_conn", "sess_time_limit", "max_send_bps", "max_recv_bps", "max_pending_tasks"
]);

const arrAppPoints = arrBasePoints.concat( [
    "resp_count", "failure_count", "pending_tasks",
    "max_streams_per_session", "max_qps", "cur_qps"
])

const PtDescProps = {
    value: 0,
    ptype: 1,
    ptflag: 2,
    average: 3,
    unit: 4,
    datatype: 5,
    size: 6
}

function type2str( iType )
{
    switch(iType)
    {
        case 1: return "byte";
        case 2: return "word";
        case 3: return "int";
        case 4: return "qword";
        case 5: return "float";
        case 6: return "double";
        case 7: return "string";
        case 10: return 'blob'
        default: break
    }
    return 'none'
}

function str2type( strType )
{
    switch(strType)
    {
        case "byte": return 1;
        case "word": return 2;
        case "int": return 3;
        case "qword": return 4;
        case "float": return 5;
        case "double": return 6;
        case "string": return 7;
        case 'blob': return 10;
        default: break;
    }
    return 0
}

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
            oContext.oGetPtDescCb = (oContext, ret, mapPtDescs) => {
                if( oContext.m_iRet < 0 )
                    return
                // Process the point descriptions
                for( const [key, oDesc] of mapPtDescs ) {

                    var ptv = oDesc.GetProperty(PtDescProps.value);
                    attrs = { v: ptv,
                        t:type2str(oDesc.GetProperty(PtDescProps.datatype))
                    }
                    var oVal = oDesc.GetProperty(PtDescProps.ptype);
                    if( typeof oVal === "number" )
                    {
                        if( oVal === 0 ) 
                            attrs.ptype = "output"
                        else if( oVal === 1 )
                            attrs.ptype = "input"
                        else if( oVal === 2 )
                            attrs.ptype = "setpoint"
                    }
                    oVal = oDesc.GetProperty(PtDescProps.average);
                    if( oVal )
                        attrs.average = oVal;

                    oVal = oDesc.GetProperty(PtDescProps.unit);
                    if( oVal )
                        attrs.unit = i18nHelper.t(oVal);

                    oVal = oDesc.GetProperty(PtDescProps.size);
                    if( oVal )
                        attrs.size = oVal;
                    var arrComps = key.split('/')
                    if( arrComps.length < 2 )
                        continue
                    app.setpoints.set( arrComps[1], attrs );
                }
                return
            };
            var arrPts = arrAppPoints
            if( app.name === "rpcrouter1")
                arrPts = arrRouterPoints
            var arrPtPath = arrPts.map(pt => `${app.name}/${pt}`);
            return oProxy.GetPointDesc( oContext, arrPtPath ).then((ret)=>{
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
