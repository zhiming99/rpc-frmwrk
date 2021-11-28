package org.rpcf.tests.iftest;

import java.util.HashMap;
import java.util.concurrent.TimeUnit;
import org.rpcf.rpcbase.JRetVal;
import org.rpcf.rpcbase.JavaRpcContext;
import org.rpcf.rpcbase.RC;
import org.rpcf.rpcbase.rpcbase;

public class maincli {
    public static JavaRpcContext m_oCtx;

    public maincli() {
    }

    public static void main(String[] args) {
        m_oCtx = JavaRpcContext.createProxy();
        if (m_oCtx != null) {
            IfTestcli oSvcCli = new IfTestcli(m_oCtx.getIoMgr(), "./iftestdesc.json", "IfTest");
            if (!RC.ERROR(oSvcCli.getError())) {
                int ret = oSvcCli.start();
                if (!RC.ERROR(ret)) {
                    while(oSvcCli.getState() == RC.stateRecovery) {
                        try {
                            TimeUnit.SECONDS.sleep(1L);
                        } catch (InterruptedException var5) {
                        }
                    }

                    if (oSvcCli.getState() != RC.stateConnected) {
                        ret = RC.ERROR_STATE;
                    } else {
                        GlobalFeatureList i0 = new GlobalFeatureList();
                        i0.extendUserFeature = new HashMap();
                        i0.extendUserFeature.put(2, "extendUserFeature");
                        i0.userFeature = new HashMap();
                        i0.userFeature.put(1, "userFeature");
                        i0.userOnlineFeature = new HashMap();
                        i0.userOnlineFeature.put("hello", "userOnlineFeature");
                        i0.userHistoricalStatisticsFeature = new HashMap();
                        i0.userHistoricalStatisticsFeature.put(3, 5);
                        i0.sceneFeature = new HashMap();
                        i0.sceneFeature.put(6, "sceneFeature");
                        i0.formL24Exposure = new HashMap();
                        i0.formL24Exposure.put("formL24Exposure", 7);
                        i0.industryL24Exposure = new HashMap();
                        i0.industryL24Exposure.put("industryL24Exposure", 8);
                        i0.flightId = "sz2680";
                        i0.requestId = "haha9527";
                        i0.time = 23456L;
                        i0.flag = 9L;
                        i0.body = new GlobalFeature[]{new GlobalFeature(), new GlobalFeature()};
                        i0.body[0].adFeature = new HashMap();
                        i0.body[0].adFeature.put(11, "adFeature");
                        i0.body[0].extendAdFeature = new HashMap();
                        i0.body[0].extendAdFeature.put(11, "extendAdFeature");
                        i0.body[0].adHistoricalStatisticsFeature = new HashMap();
                        i0.body[0].adHistoricalStatisticsFeature.put(12, 13);
                        i0.body[0].userRealtimeStatisticsFeature = new HashMap();
                        i0.body[0].userRealtimeStatisticsFeature.put(14, 15);
                        i0.body[0].extenduserRealtimeStatisticsFeature = new HashMap();
                        i0.body[0].extenduserRealtimeStatisticsFeature.put(16, 17);
                        i0.body[0].adRealtimeStatisticsFeature = new HashMap();
                        i0.body[0].adRealtimeStatisticsFeature.put(18, 19);
                        i0.body[0].adFormId = "bobo";
                        i0.body[0].materialId = "steel";
                        i0.body[0].creativeId = "cici";
                        i0.body[0].sponsorId = "dudu";
                        i0.body[0].adFeature = new HashMap();
                        i0.body[0].adFeature.put(11, "adFeature");
                        i0.body[0].extendAdFeature = new HashMap();
                        i0.body[0].extendAdFeature.put(11, "extendAdFeature");
                        i0.body[0].adHistoricalStatisticsFeature = new HashMap();
                        i0.body[0].adHistoricalStatisticsFeature.put(12, 13);
                        i0.body[0].userRealtimeStatisticsFeature = new HashMap();
                        i0.body[0].userRealtimeStatisticsFeature.put(14, 15);
                        i0.body[0].extenduserRealtimeStatisticsFeature = new HashMap();
                        i0.body[0].extenduserRealtimeStatisticsFeature.put(16, 17);
                        i0.body[0].adRealtimeStatisticsFeature = new HashMap();
                        i0.body[0].adRealtimeStatisticsFeature.put(18, 19);
                        i0.body[0].adFormId = "bobo";
                        i0.body[0].materialId = "steel";
                        i0.body[0].creativeId = "cici";
                        i0.body[0].sponsorId = "dudu";
                        i0.body[1].adFeature = new HashMap();
                        i0.body[1].adFeature.put(11, "adFeature");
                        i0.body[1].extendAdFeature = new HashMap();
                        i0.body[1].extendAdFeature.put(11, "extendAdFeature");
                        i0.body[1].adHistoricalStatisticsFeature = new HashMap();
                        i0.body[1].adHistoricalStatisticsFeature.put(12, 13);
                        i0.body[1].userRealtimeStatisticsFeature = new HashMap();
                        i0.body[1].userRealtimeStatisticsFeature.put(14, 15);
                        i0.body[1].extenduserRealtimeStatisticsFeature = new HashMap();
                        i0.body[1].extenduserRealtimeStatisticsFeature.put(16, 17);
                        i0.body[1].adRealtimeStatisticsFeature = new HashMap();
                        i0.body[1].adRealtimeStatisticsFeature.put(18, 19);
                        i0.body[1].adFormId = "foofoo";
                        i0.body[1].materialId = "plastic";
                        i0.body[1].creativeId = "giogio";
                        i0.body[1].sponsorId = "jojo";
                        JRetVal jret = oSvcCli.Echo(i0);
                        if (jret.ERROR()) {
                            rpcbase.JavaOutputMsg("Echo request failed");
                        } else {
                            rpcbase.JavaOutputMsg("Echo request completed");
                        }
                    }

                    oSvcCli.stop();
                    m_oCtx.stop();
                }
            }
        }
    }
}

