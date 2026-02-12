const arrBasePoints = [{ n:"cpu_load", o:10 }, 
    {n:"vmsize_kb",o:11}, 
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
    {n:"conn_count", o:40},
    {n:"display_name", o:1000}
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

const arrTimerPoints = arrBasePoints.concat( [
    {n:"interval1",o:51},
    {n:"interval2",o:52},
    {n:"interval3",o:53},
    {n:"interval4",o:54},
    {n:"schedule1",o:55},
]).sort( (a,b) => { return a.o - b.o } );

const arrLoggerPoints = arrBasePoints.concat( [
    {n:"lines",o:31},
]).sort( (a,b) => { return a.o - b.o } );

const PtDescProps = {
    value: 0,
    ptype: 1,
    ptflag: 2,
    average: 3,
    unit: 4,
    datatype: 5,
    size: 6,
    haslog:7,
    avgalgo:8,
    retval: 12329
}

const AvgAlgo = {
    algoDiff: 0,
    algoAvg: 1,
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
        case "int" :
        case "uint32": return 3;
        case "qword": return 4;
        case "float": return 5;
        case "double": return 6;
        case "string": return 7;
        case 'blob': return 10;
        default: break;
    }
    return 0
}

globalThis.fetchAppDetails = () =>
{
    if( !globalThis.curSpModal || ! globalThis.curProxy )
        return Promise.resolve(0)
    try{
        const app = globalThis.curSpModal.currentApp;
        // Fetch and display app details for the selected app
        const oProxy = globalThis.curProxy;
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
            var arrFeaturePts = [];
            oContext.oGetLpvCb = (oContext, ret, rvalue) => {
                if( oContext.m_iRet < 0 )
                    console.log( "Error, no display points available with status " + oContext.m_iRet );
                else {
                    const decoder = new TextDecoder('utf-8')
                    let text=decoder.decode(rvalue);
                    var ptList = JSON.parse(text);
                    if( !ptList || ptList.length === 0 )
                        return;
                    var ordinal = 0
                    ptList.forEach(element => {
                        arrFeaturePts.push( { n: element, o: ordinal++ });
                    });
                }
            }
            return oProxy.GetLargePointValue( oContext, `${app.name}/display_points` ).then((ret) => {
                var arrPts = null
                oContext.oGetPtDescCb = (oContext, ret, mapPtDescs) => {
                    if( oContext.m_iRet < 0 )
                        return
                    // Process the point descriptions
                    for( const [key, oDesc] of mapPtDescs ) {
                        var ptv = oDesc.GetProperty(PtDescProps.retval);
                        if( ptv & 0x80000000 )
                            continue;
                        var ptv = oDesc.GetProperty(PtDescProps.value);
                        attrs = { v: ptv === undefined || ptv === null ? null : ptv,
                            t:type2str(oDesc.GetProperty(PtDescProps.datatype)),
                            cnt : 0
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
                        if( oVal !== undefined && oVal !== null )
                            attrs.average = oVal;

                        oVal = oDesc.GetProperty(PtDescProps.unit);
                        if( oVal !== undefined && oVal !== null )
                            attrs.unit = i18nHelper.t(oVal);

                        oVal = oDesc.GetProperty(PtDescProps.size);
                        if( oVal !== undefined && oVal !== null )
                            attrs.size = oVal;

                        oVal = oDesc.GetProperty(PtDescProps.haslog);
                        if( oVal !== undefined && oVal !== null )
                            attrs.haslog = true;
                        else
                            attrs.haslog = false;
                        oVal = oDesc.GetProperty(PtDescProps.avgalgo);
                        if( oVal !== undefined && oVal !== null )
                            attrs.avgalgo = oVal;

                        var arrComps = key.split('/')
                        if( arrComps.length < 2 )
                            continue
                        app.setpoints.set( arrComps[1], attrs );
                    }
                    return
                };
                if( app.app_class === "rpcrouter")
                    arrPts = arrRouterPoints
                else if( app.app_class === "timer")
                    arrPts = arrTimerPoints
                else if( app.app_class === "logger")
                    arrPts = arrLoggerPoints
                else
                    arrPts = arrAppPoints
                if( arrFeaturePts.length > 0 )
                {
                    arrPts = [].concat( arrFeaturePts, arrPts ).sort( (a,b) => { return a.o - b.o } );
                }

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
                console.log("Error GetDisplayPoints:", error);
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

globalThis.parseLogLines = ( logData, avgalgo, timeRangeSec ) =>
{
    try{
        if( !logData || logData.length === 0 )
            return [];
        var dwOffset = 0
        var dwMagic = logData.readUint32BE(dwOffset);
        if( dwMagic !== 0x706c6f67 )
            return [];
        dwOffset += 4;
        var dwCounter = logData.readUint32BE(dwOffset);
        if( dwCounter == 0 )
            return [];
        var arrTimeSeries = []
        dwOffset += 4;
        var wTypeId = logData.readUint16BE( dwOffset );
        dwOffset += 2;
        var wRecSize = logData.readUint16BE( dwOffset )
        dwOffset += 6;
        var dwNumRec = Math.floor( ( logData.length - dwOffset ) / wRecSize );
        if( dwNumRec < 10 )
            return [];

        dwNumRec = Math.min( dwNumRec, dwCounter )

        var dwTimeStamp, val = null, preval=0
        var dwInterval =
            logData.readUint32BE( dwOffset + wRecSize) -
            logData.readUint32BE( dwOffset )
        var dwRecToRead = dwNumRec
        if( timeRangeSec > 0 && dwInterval >= 1 )
        {
            dwRecToRead = Math.floor( timeRangeSec / dwInterval )
            if( dwRecToRead < dwNumRec)
                dwOffset += (dwNumRec - dwRecToRead) * wRecSize
            else
                dwRecToRead = dwNumRec
        }

        var valMax = 0, valMin = 0, distArr = []
        for( var i = 0; i < dwRecToRead; i++ )
        {
            dwTimeStamp = logData.readUint32BE( dwOffset )
            dataoff = dwOffset + 4
            const strType = type2str( wTypeId )
            switch( strType )
            {
            case "byte": 
                val = logData.readUint8( dataoff )
                break
            case "word":
                val = logData.readUint16BE( dataoff )
                break
            case "int":
                val = logData.readUint32BE( dataoff )
                break
            case "qword":
                val = Number(logData.readBigUInt64BE( dataoff ))
                break
            case "float":
                val = logData.readFloatBE( dataoff )
                break
            case "double":
                val = logData.readDoubleBE( dataoff )
                break
            case "string":
                var size = logData.readUint32BE( dataoff )
                val = logData.toString('utf-8', dataoff, dataoff + size)
                break
            default:
                val = null
                break
            }
            if( val === null )
                throw new Error( `Error ${strType} not supported`)

            if( val > valMax )
                valMax = val

            if( val < valMin )
                valMin = val

            if( avgalgo == AvgAlgo.algoDiff)
            {
                if( i > 0 )
                    arrTimeSeries.push([dwTimeStamp,val - preval ])
                preval = val
            }
            else
                arrTimeSeries.push([dwTimeStamp,val ])
            dwOffset += wRecSize
        }

        for( var i = -10; i < 10; i++ )
            distArr.push( {val: valMin + ( valMax - valMin ) * i / 20, count: 0 } )
        distArr.push( {val: valMax, count: 0 } )

        for( var i = 0; i < arrTimeSeries.length; i++ )
        {
            var val = arrTimeSeries[i][1]
            for( var j = 0; j < distArr.length; j++ )
            {
                if( val <= distArr[j].val )
                {
                    distArr[j].count++
                    break;
                }
            }
        }
        var countMin = 0
        var countMax = 0

        var lowIdx = 0;
        for( var i = 0; i <= 10; i++ )
        {
            if( countMin / arrTimeSeries.length > 0.02 )
                break
            lowIdx = i
            countMin += distArr[i].count
        }
        var lowThreshold = distArr[lowIdx].val

        var highIdx = distArr.length - 1
        for( var i = distArr.length - 1; i > 10; i-- )
        {
            if( countMax / arrTimeSeries.length > 0.02 )
                break
            highIdx = i
            countMax += distArr[i].count
        }
        var highThreshold = distArr[highIdx].val

        arrTimeSeries = arrTimeSeries.filter(
            item => item[1] >= lowThreshold && item[1] <= highThreshold )
        return arrTimeSeries
    }
    catch(e){
        console.error("Error parsing log lines:", e);
        return [];
    }
}

globalThis.downloadPointLog = ( oContext, ptPath, logName, avgalgo, timeRangeSec ) =>
{
    globalThis.oStmCtx = oContext
    return new Promise( ( resolve, reject ) =>{
        if( !globalThis.curProxy )
            return reject( new Error("No proxy available") );
        var oProxy = globalThis.curProxy
        // oContext.m_oResolve is used by GetPtLog callbacks,
        // so we use m_oResolve2 here
        oContext.m_oResolve2 = resolve;
        oContext.m_oReject2 = reject;
        oContext.m_ptPath = ptPath;
        oContext.m_logName = logName;
        oContext.m_dwSize = 0
        oContext.m_dwOffset = 0
        oContext.m_avgAlgo = avgalgo
        oContext.m_timeRangeSec = timeRangeSec
        oContext.m_hStream = 0
        oContext.m_iRet = 0
        oContext.m_arrLogData = globalThis.AllocBuffer(0)
        return oProxy.m_funcOpenStream( oContext ).then((oContext)=>{
            if( oContext.m_iRet < 0 )
                return oContext.m_oReject2( new Error("OpenStreamLocal failed with status " + oContext.m_iRet) );
            if( oContext.m_hStream === 0 )
                return oContext.m_oReject2( new Error("OpenStreamLocal failed: invalid stream handle") );

            oContext.oGetPtLogCb = (( oContext, ret ) => {
                var This = globalThis.oStmCtx
                if( This.m_iRet < 0 )
                    oProxy.m_funcCloseStream( This.m_hStream)
                return;
            })

            oContext.OnDataReceived=(( hStream, oBuf ) => {
                var This = globalThis.oStmCtx
                if( hStream !== This.m_hStream )
                    return 0
                if( This.m_dwSize === 0 )
                {
                    This.m_dwSize = oBuf.readUint32BE(0);
                    This.m_arrLogData = oBuf.slice(4)
                    This.m_dwOffset += oBuf.length - 4;
                }
                else
                {
                    This.m_arrLogData = globalThis.ConcatBuffer(
                        This.m_arrLogData, oBuf )
                    This.m_dwOffset += oBuf.length;
                }
                if( This.m_dwOffset === This.m_dwSize )
                {
                    This.m_iRet = 0
                    oProxy.m_funcCloseStream( This.m_hStream )
                    This.m_hStream = null
                    This.m_oResolve2(0)
                }
                else if( This.m_dwOffset > This.m_dwSize )
                {
                    This.m_iRet = -34
                    oProxy.m_funcCloseStream( This.m_hStream )
                    This.m_hStream = null
                    This.m_oReject2( new Error("GetPtLog internal error " + (-34)))
                }
                return 0
            })

            oContext.OnStmClosed = ((hstream) =>{
                var This = globalThis.oStmCtx
                if( hstream !== This.m_hStream )
                    return
                if( This.m_dwSize > 0 )
                    return This.m_oResolve2(-34)
                return This.m_oResolve2(0)
            })

            var oInfo = globalThis.NewCfgDb()
            var oSize = { t: str2type("int"), v: 0 }
            oInfo.Push(oSize)
            return oProxy.GetPtLog( oContext, oContext.m_ptPath, oContext.m_hStream, oContext.m_logName, oInfo ).then((e)=>{
                if( oContext.m_iRet < 0 ) 
                    return oContext.m_oReject2( new Error("GetPtLog failed with status " + oContext.m_iRet) );
            }).catch((e)=>{
                return oContext.m_oReject2( new Error("GetPtLog failed with status " + oContext.m_iRet) );
            })
        }).catch((e)=>{
            return oContext.m_oReject2( new Error("OpenStreamLocal exception: " + e) );
        });
    }).then((ret)=>{
        return Promise.resolve(0);
    }).catch((e)=>{
        return Promise.reject(e);
    })
}

