// 
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
// 
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself, 
// and cannot be removed from it.
// 
package simu5g.simulations.NR.mec.trafficlight;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;

// MultiCell_standalone
//
// This network represents the deployment option 3 in 3GPP 38.801
//
network TrafficlightNetwork
{
    parameters:
        int numCarA = default(1);
        int numCarB = default(1);
        int numCarC = default(1);
        int numCarD = default(1);

        int numBgCells = default(0);
        @display("i=block/network2;bgb=1221.6749,756.20996;bgi=background/atos_bg,s");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=49.14,24.57;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=49.14,24.57");
        }
        binder: Binder {
            @display("p=50.504997,13.65;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=49.14,24.57;is=s");
        }
        server: StandardHost {
            @display("p=1152.0599,285.285;is=n;i=device/server");
        }
        router: Router {
            @display("p=1105.65,154.245;i=device/smallrouter");
        }
        upf: Upf {
            @display("p=920.01,155.61");
        }
        iUpf: Upf {
            @display("p=569.205,155.61");
        }
        gnb1: gNodeB {
            @display("p=238.875,353.535;is=vl");
        }
        gnb2: gNodeB {
            @display("p=920.01,353.535;is=vl");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=30.029999,76.439995;is=vl");
        }
        trLight1: NRUe {
            @display("p=349.44,687.95996;i=misc/trafficlight");
        }
        trLight2: NRUe {
            @display("p=363.09,85.994995;i=misc/trafficlight");
        }
        trLight3: NRUe {
            @display("p=806.71497,84.63;i=misc/trafficlight");
        }
        trLight4: NRUe {
            @display("p=806.71497,677.04;i=misc/trafficlight");
        }

        sumo1: NRUe {
            @display("p=297.57,616.98;i=misc/securitycamera");
        }

        sumo2: NRUe {
            @display("p=307.125,23.205;i=misc/securitycamera");
        }

        sumo3: NRUe {
            @display("p=763.035,23.205;i=misc/securitycamera");
        }

        sumo4: NRUe {
            @display("p=748.01996,608.79;i=misc/securitycamera");
        }

        carA[numCarA]: NRUe {
            @display("p=565.11,47.774998;i=misc/car2");
        }
        carB[numCarB]: NRUe {
            @display("p=805.35,353.535;i=misc/car2");
        }
        carC[numCarC]: NRUe {
            @display("p=565.11,651.105;i=misc/car2");
        }
        carD[numCarD]: NRUe {
            @display("p=363.09,378.10498;i=misc/car2");
        }

    connections:

        //# Data Network connections
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> upf.filterGate;

        //# 5G Core Network connections
        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gnb1.ppp;
        iUpf.pppg++ <--> Eth10G <--> gnb2.ppp;

        //# X2 connections
        gnb1.x2++ <--> Eth10G <--> gnb2.x2++;
}
