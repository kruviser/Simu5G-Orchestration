//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.simulations.NR.industrial_scenario;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.ethernet.EthernetSwitch;
import ned.IdealChannel;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MECHost;
import simu5g.nodes.mec.bgMecAppManager.BgMecAppManager;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;

import simu5g.nodes.mec.MECOrchestrator.MecOrchestrator;
import simu5g.nodes.mec.UALCMP.UALCMP;

channel CloudConnection extends Eth10G
{
    delay = 100ms;
}

network industrial_network
{
    parameters:
        @display("i=block/network2;bgb=1018.93396,615.24396;bgi=background/pisa");

        int numUes = default(0);
        int numExtremeEdgeResources = default(0);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)

    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=877.6425,122.64;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=968.34503,122.64;is=s");
        }
        channelControl: LteChannelControl {
            @display("p=876.365,24.272501;is=s");
        }
        binder: Binder {
            @display("p=976.01,200.5675;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=877.6425,209.51;is=s");
        }
        upf: Upf {
            @display("p=537.8275,249.1125");
        }
        iUpf: Upf {
            @display("p=352.92624,249.77875");
        }
        gNodeB1: gNodeB {
            @display("p=130.816,249.368;is=vl");
        }
        //# MEC module
        MECnode: MECHost {
            @display("p=352.59,387.0825;is=l");
        }
        ControlRoom: MECHost {
            @display("p=352.59,123.661995;is=l");
        }

        mecOrchestrator: MecOrchestrator {
            @display("p=542.9375,45.99;is=l");
        }

        ualcmp: UALCMP {
            @display("p=544.215,123.9175;i=device/smallrouter");
        }

        bgMecAppManager: BgMecAppManager {
            @display("p=974.7325,30.66");
        }

        CloudServer: StandardHost {
            @display("p=743.505,62.5975;is=n;i=device/router");
        }
        router: Router {
            @display("p=743.505,249.1125;i=misc/cloud");
        }


        PA: NRUe {
            @display("p=52.3775,387.0825;i=block/cogwheel");
        }
        localDTnode: NRUe {
            @display("p=186.515,387.0825;i=old/telnet");
        }
        HumanOperator: NRUe {
            @display("p=190.3475,514.8325;i=misc/person3");
        }
    connections allowunconnected:
        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        CloudServer.pppg++ <--> Eth10G <--> router.pppg++;
        ualcmp.toMecOrchestrator --> mecOrchestrator.fromUALCMP;
        ualcmp.fromMecOrchestrator <-- mecOrchestrator.toUALCMP;

        MECnode.ppp++ <--> Eth10G <--> iUpf.pppg++;
        ControlRoom.ppp++ <--> Eth10G <--> iUpf.pppg++;

        router.pppg++ <--> Eth10G <--> upf.filterGate;
        router.pppg++ <--> Eth10G <--> ualcmp.ppp++;
}
