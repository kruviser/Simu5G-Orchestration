//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.simulations.NR.mec.orchestration;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.bgMecAppManager.BgMecAppManager;

import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;



network mec_network_large
{
    parameters:
        @display("i=block/network2;bgb=811.0225,507.6475;bgi=background/pisa");

        int numUes = default(0);
        int numExtremeEdgeResources = default(0);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)

        @statistic[serviceLatency](source=ue[0].app[0].cbrReqServiceTimeSignal; record=figure; targetFigure=serviceLatencyGauge; checkSignals=false);
        @figure[serviceLatencyGauge](type=gauge; pos=700,10; size=100,100; minValue=0; maxValue=0.1; tickSize=0.01; label="App service latency");

        //@statistic[serviceLatency2](source=ue[0].app[0].cbrReqServiceTimeSignal*1000; record=figure; targetFigure=serviceLatencyGauge2; checkSignals=false);
        //@figure[serviceLatencyGauge2](type=plot; pos=700,140; size=400,200; timeWindow=20; maxY=100; yTickSize=10; label="App service latency");

        @statistic[numMecAppSignalVisual1](source=mecHost1.vim.numMecAppSignal; record=figure; targetFigure=appThermometer1);
        @figure[appThermometer1](type=thermometer; pos=380,30; size=30,150; minValue=0; maxValue=15; tickSize=1; label="number of MEC apps 1");

        @statistic[numMecAppSignalVisual2](source=mecHost2.vim.numMecAppSignal; record=figure; targetFigure=appThermometer2);
        @figure[appThermometer2](type=thermometer; pos=610,30; size=30,150; minValue=0; maxValue=15; tickSize=1; label="number of MEC apps 2");




    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=753.214,23.505999;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=747.082,23.505999;is=s");
        }
        channelControl: LteChannelControl {
            @display("p=752.19196,23.505999;is=s");
        }
        binder: Binder {
            @display("p=747.082,23.505999;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=753.214,23.505999;is=s");
        }
        upf: Upf {
            @display("p=574.364,249.368");
        }
        iUpf: Upf {
            @display("p=440.482,249.368");
        }
        gNodeB1: gNodeB {
            @display("p=140,250;is=vl");
        }
        ue[numUes]: NRUe {
            @display("p=94.023994,178.84999");
        }
        ueExtremeEdge[numExtremeEdgeResources]: NRUe {
            @display("p=78.694,317.84198;i=device/card");
        }
        //# MEC module
        mecHost1: MECHost {
            @display("p=440.482,112.42;is=l");
        }
        mecHost2: MECHost {
            @display("p=574.364,112.42;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=752.19196,249.368;is=l");
        }

        ualcmp: UALCMP {
            @display("p=667.36597,248.346;i=device/smallrouter");
        }

        bgMecAppManager: BgMecAppManager {
            @display("p=746.06,22.484");
        }


        gNodeB2: gNodeB {
            @display("p=257.544,91.979996;is=vl");
        }
        gNodeB3: gNodeB {
            @display("p=263.676,415.95398;is=vl");
        }
    connections allowunconnected:

        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        iUpf.pppg++ <--> Eth10G <--> gNodeB2.ppp;
        iUpf.pppg++ <--> Eth10G <--> gNodeB3.ppp;
        ualcmp.ppp++ <--> Eth10G <--> upf.filterGate;

        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        mecHost1.ppp++ <--> Eth10G <--> iUpf.pppg++;
        mecHost2.ppp++ <--> Eth10G <--> iUpf.pppg++;

}
