const arrBasePoints = [{ n:"cpu_load", o:0 }, 
    {n:"vmsize_kb",o:10}, 
    {n:"obj_count",o:20},
    {n:"req_count",o:30},
    {n:"uptime",o:240},
    {n:"offline_times",o:250},
    {n:"cmdline",o:260},
    {n:"open_files", o:70},
    {n:"working_dir",o:280},
    {n:"rx_bytes",o:90},
    {n:"tx_bytes",o:100},
    {n:"pid",o:110},
    {n:"conn_count", o:40}
]

const arrRouterPoints = arrBasePoints.concat( [
    {n:"max_conn",o:200},
    {n: "sess_time_limit",o:110},
    {n:"max_send_bps",o:220},
    {n:"max_recv_bps",o:230},
    {n:"max_pending_tasks",o:241}
]).sort( (a,b) => { return a.o - b.o } );

const arrAppPoints = arrBasePoints.concat( [
    {n:"resp_count",o:120},
    {n:"failure_count",o:130},
    {n:"pending_tasks",o:200},
    {n:"max_streams_per_session",o:140},
    {n:"max_qps",o:80},
    {n:"cur_qps",o:81},
]).sort( (a,b) => { return a.o - b.o } );

const PtDescProps = {
    value: 0,
    ptype: 1,
    ptflag: 2,
    average: 3,
    unit: 4,
    datatype: 5,
    size: 6,
    retval: 12329
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
        if( app.bRegistered && app.bRegistered === true )
        {
            // keep-alive request to prevent browser from being disconnected
            var oContext = {};
            oContext.oGetPvCb = (oContext, ret, rvalue) => {
                if( oContext.m_iRet < 0 )
                    console.log( "Error, GetPointValue failed with status " + oContext.m_iRet );
            }
            return oProxy.GetPointValue( oContext, "rpcrouter1/cpu_load" ).then((ret)=>{
                    return Promise.resolve( oContext.m_iRet );
                }).catch((error) => {
                    console.log("Error GetPointValue:", error);
                    return Promise.resolve(error)
                });
        }
        var oContext = {};
        app.setpoints = new Map();
        oContext.oRegListenerCb = (oContext, ret) => {
            if( oContext.m_iRet < 0 )
                console.log( "Errror, RegisterListener failed with status " + oContext.m_iRet );
            else
                console.log( "RegisterListener succeeded with status " + oContext.m_iRet );
        };
        return oProxy.RegisterListener( oContext, [app.name] ).then((ret) => {
            app.bRegistered = true;
            oContext.oGetPtDescCb = (oContext, ret, mapPtDescs) => {
                if( oContext.m_iRet < 0 )
                    return
                // Process the point descriptions
                for( const [key, oDesc] of mapPtDescs ) {
                    var ptv = oDesc.GetProperty(PtDescProps.retval);
                    if( ptv & 0x80000000 )
                        continue;
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
            if( app.app_class === "rpcrouter")
                arrPts = arrRouterPoints
            globalThis.curSpModal.arrPts = arrPts;
            var arrPtPath = arrPts.map(pt => `${app.name}/${pt.n}`);
            return oProxy.GetPointDesc( oContext, arrPtPath ).then((ret)=>{
                if( oContext.m_iRet < 0 )
                    return Promise.reject( oContext.m_iRet );
                return Promise.resolve( 0 );
            }).catch((error) => {
                console.log("Error GetPointDesc:", error);
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
