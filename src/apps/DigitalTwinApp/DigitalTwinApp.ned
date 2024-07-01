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

package simu5g.apps.DigitalTwinApp;

import inet.applications.contract.IApp;


simple DigitalTwinApp like IApp
{
    parameters:
        int localPort = default(3000);

        int destPort = default(3000);
        string destAddress;

        int responseSize = default(40);

        bool enableVimComputing @mutable = default(false);
        double serviceComplexity @mutable = default(1); // expressed in fractions of MIPs (the actual number of instructions will be rounded to the closest integer)

        int computingType = default(0); // EDGE = 0 , EXTREME_EDGE = 1 
        double extremeEdgeComputingTime @unit("s") = default(200ms);
        bool enableOrchestration = default(false);

        string monitoringDT;
        int monitoringDTindex = default(-1);

        @signal[cbrReqFrameDelaySignal](type="simtime_t");
        @signal[cbrReqJitterSignal](type="simtime_t");
        @signal[cbrReqFrameLossSignal](type="double");
        @signal[cbrReqReceivedThroughtputSignal];
        @signal[cbrReqReceivedBytesSignal];
        @signal[AoIatDTSignal];

        @statistic[cbrReqFrameDelay](title="Cbr Frame Delay"; unit="s"; source="cbrReqFrameDelaySignal"; record=mean,vector);
        @statistic[cbrReqJitter](title="Cbr Playout Loss"; unit="s"; source="cbrReqJitterSignal"; record=mean,vector);
        @statistic[cbrReqFrameLoss](title="Cbr Frame Loss"; unit="ratio"; source="cbrReqFrameLossSignal"; record=mean);
        @statistic[cbrReqReceivedThroughtput](title="Throughput received at the application level"; unit="Bps"; source="cbrReqReceivedThroughtputSignal"; record=timeavg,mean,sum);
        @statistic[cbrReqReceivedBytes](title="Bytes received at the application level"; unit="Bps"; source="cbrReqReceivedBytesSignal"; record=timeavg,mean,vector,sum);
        @statistic[AoIatDTSignal](title="AoI of the sample used for DT update"; unit="s"; source="AoIatDTSignal"; record=mean,vector);


        @signal[cbrReqRcvdPkt];
        @statistic[cbrReqRcvdPkt](title="Received packet ID"; unit=""; source="cbrReqRcvdPkt"; record=vector);

        @display("i=block/broadcast");
    gates:
        output socketOut;
        input socketIn;
}

