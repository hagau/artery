package artery.inet;

import artery.application.StationaryMiddleware;
import artery.application.RadioManager;
import artery.application.IRadioModule;
import artery.application.RadioModule;
import artery.application.McoStrategy;
import artery.nic.IRadioDriver;
import inet.node.inet.INetworkNode;
import inet.linklayer.contract.IWirelessNic;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;

module RSU like INetworkNode
{
    parameters:
        @display("i=block/wrxtx;is=vs");
        @networkNode;
        @labels(node,ethernet-node,wireless-node);
        int numRadios = default(1);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        mobility.visualRepresentation = "^";

    gates:
        input radioIn[numRadios] @directIn;
        inout pppg[] @labels(PPPFrame-conn);
        inout ethg[] @labels(EtherFrame-conn);

    submodules:
        interfaceTable: InterfaceTable {
            parameters:
                @display("p=53,300;is=s");
        }

        wlan[numRadios]: <default("VanetNic")> like IWirelessNic {
            parameters:
                @display("p=216,406,row,60;q=queue");
        }

        mobility: <default("inet.mobility.static.StationaryMobility")> like IMobility {
            parameters:
                @display("p=53,200");
        }

        radioDriver[numRadios]: InetRadioDriver {
            parameters:
                macModule = ".wlan[" + string(index) + "].mac";
        }

        middleware: StationaryMiddleware {
            parameters:
                mobilityModule = ".mobility";
                traciCoreModule = "traci.core";
                numRadios = numRadios;
        }

        radioModule[numRadios]: <default("artery.application.ItsRadioModule")> like IRadioModule {
            parameters:
                @display("p=350,200");
                radioDriverModule = ".radioDriver[" + string(index) + "]";
        }

        radioManager: RadioManager {
            parameters:
                @display("p=350,200");
        }

        mcoStrategy: <default("artery.application.McoStaticStrategy")> like McoStrategy {
            parameters:
                @display("p=400,200");
        }

    connections:
        for i=0..sizeof(radioIn)-1 {
            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;

            wlan[i].upperLayerOut --> radioDriver[i].lowerLayerIn;
            wlan[i].upperLayerIn <-- radioDriver[i].lowerLayerOut;

            radioModule[i].radioDriverIn <-- radioDriver[i].middleware$o;
            radioModule[i].radioDriverOut --> radioDriver[i].middleware$i;
        }
}
