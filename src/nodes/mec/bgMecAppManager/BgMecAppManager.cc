/*
 * bgMecAppManager.cc
 *
 *  Created on: Jul 15, 2022
 *      Author: linofex
 */


#include "BgMecAppManager.h"
#include "nodes/mec/bgMecAppManager/timer/BgTimer_m.h"
#include "nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.h"
#include "apps/CbrRequestResponse/CbrRequester.h"
#include "apps/CbrRequestResponse/CbrResponder.h"

using namespace omnetpp;


Define_Module(BgMecAppManager);



simsignal_t BgMecAppManager::activationDecisionSignal_ = registerSignal("activationDecisionSignal");
simsignal_t BgMecAppManager::totalAppSignal_ = registerSignal("totalAppSignal");



BgMecAppManager::~BgMecAppManager()
{
    for (auto& bgMecApp: bgMecApps_)
    {
        cancelAndDelete(bgMecApp.second.timer);
        // the modules are deleted by the omnet platform? I hope
    }

    bgMecApps_.clear();

    cancelAndDelete(deltaMsg_);
}

BgMecAppManager::BgMecAppManager()
{
    deltaMsg_ = nullptr;
    snapMsg_ = nullptr;
    orchestratedApp_ = nullptr;
    orchestratedResponder_ = nullptr;
}


void BgMecAppManager::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    bgAppsVector_.setName("MecBgApps");

    fromTraceFile_ = par("fromTraceFile").boolValue();
    lastMecHostActivated_ = -1;
    mecHostActivationTime_ = par("mecHostActivation");
    maxBgMecApp_ = par("maxBgMecApps");
    minBgMecApp_ = par("minBgMecApps");

    admissionControl_ = par("admissionControl");
    currentBgMecApps_ = 0;
    readMecHosts();

    enablePeriodicLoadBalancing_ = par("enablePeriodicLoadBalancing").boolValue();
    balancingInterval_ = par("balancingInterval");
    lastBalancedApps_ = -1; // last number of application seen by the load balancer (used to avoid unnecessary balancing)
    lastBalancedHosts_ = -1; // last number of active hosts seen by the load balancer (used to avoid unnecessary balancing)

    enableHostActivationDelay_ = par("enableHostActivationDelay").boolValue();
    hostActivationTriggered_ = false;

    if(enablePeriodicLoadBalancing_)
    {
        balancingTimer_ = new cMessage("balancingTimer");
        scheduleAfter(balancingInterval_+0.001, balancingTimer_);
    }

    int orch = par("orchestrationType").intValue();
    switch(orch)
    {
        case 0:
            orchestrationType_ = DUMMY_ORCHESTRATION;
            break;
        case 1:
            orchestrationType_ = EXTERNAL_ORCHESTRATION;
            break;
        default:
            orchestrationType_ = DUMMY_ORCHESTRATION;
    }

    orchestrationPolicy_ = par("orchestrationPolicy").stringValue();

    defaultRam_ = par("defaultRam");
    defaultDisk_ = par("defaultDisk");
    defaultCpu_ = par("defaultCpu"); // Expressed in MIPs
    if(!fromTraceFile_)
    {
        deltaTime_ = par("deltaTime").doubleValue();
        deltaMsg_ = new cMessage("deltaMsg");
        mode_ = CREATE;
        scheduleAfter(deltaTime_, deltaMsg_);
    }
    else
    {
      // read the tracefile and create structure with snapshots
        std::string fileName = par("traceFileName").stringValue();

        simtime_t timeLine = simTime();
        snapshotPeriod_ = par("snapshotPeriod").doubleValue();
        bool realTimeStamp = snapshotPeriod_ > 0? false: true;
        std::ifstream inputFileStream;
        inputFileStream.open(fileName);
        if (!inputFileStream)
            throw cRuntimeError("Error opening data file '%s'", fileName.c_str());

        while(true)
        {
            Snapshot newsnapShot;
            // read line by line
            double time;
            int num;
            if(realTimeStamp) //read two numbers
            {
                inputFileStream >> time; // newsnapShot.snapShotTime;
            }
            else
            {
                timeLine += snapshotPeriod_;
                time = timeLine.dbl();
            }
            if(!inputFileStream.eof())
            {
                inputFileStream >> num;  // newsnapShot.numMecApps;
                newsnapShot.snapShotTime = time;
                newsnapShot.numMecApps = num;
                snapshotList_.push_back(newsnapShot);
            }
            else
            {
                break;
            }
        }

        std::cout << "****" << std::endl;
//  debug
        for(auto& snap : snapshotList_)
        {
            std::cout << "time " << snap.snapShotTime << " apps " << snap.numMecApps << std::endl;
        }
        std::cout << "###" << std::endl;

        // schedule first snapshot
        snapMsg_ = new cMessage("snapshotMsg");
        scheduleNextSnapshot();
    }
}


void BgMecAppManager::scheduleNextSnapshot()
{
    if(!snapshotList_.empty())
    {
        double timer = snapshotList_.front().snapShotTime;
        EV << "BgMecAppManager::handleMessage - next snapshot is scheduled at time: " << timer << endl;
        // TODO create structure for quantum and smoother line
        scheduleAt(timer, snapMsg_);
    }
    else
    {
        EV << "BgMecAppManager::scheduleNextSnapshot()- no more snapshot available" << endl;
    }
}

bool BgMecAppManager::createBgModules(cModule* mecHost)
 {
     // the way it is currently written is for use in a loop wherein currentBgMecApps_ is incremented at every cycle
     int id = currentBgMecApps_;

     EV << "BgMecAppManager::createBgModules - creating app " << id << endl;
     ResourceDescriptor resource = {defaultRam_, defaultDisk_, defaultCpu_};
     BgMecAppDescriptor appDescriptor;
     appDescriptor.centerX = par("centerX");
     appDescriptor.centerY = par("centerY");
     appDescriptor.radius = par("radius");
     if( mecHost == nullptr )
         appDescriptor.mecHost = chooseMecHost();
     else
         appDescriptor.mecHost = mecHost;
     appDescriptor.resources = resource;
     appDescriptor.timer = nullptr;

     // this does the insertion into the map
     bgMecApps_[id] = appDescriptor;

     cModule* bgAppModule = createBgMecApp(id);

     if(bgAppModule != nullptr) // resource available
     {
         cModule* bgUeModule = createBgUE(id);
         auto desc = bgMecApps_.find(id);
         desc->second.bgMecApp = bgAppModule;
         desc->second.bgUe = bgUeModule;
         EV << "BgMecAppManager::handleMessage - bg environment with id [" << id << "] started" << endl;
         currentBgMecApps_++;
         return true;
     }
     else
     {
         EV << "BgMecAppManager::handleMessage - bg environment with id [" << id << "] NOT started" << endl;
         bgMecApps_.erase(id);
         return false;
     }

 }

 void BgMecAppManager::deleteBgModules()
 {
     int id = --currentBgMecApps_;
     deleteBgMecApp(id);
     deleteBgUe(id);

     //if(isMecHostEmpty(bgMecApps_[id].mecHost))
     //    deactivateNewMecHost(bgMecApps_[id].mecHost);

     //check if the MecHost is empty and in case deactivate it
     EV << "BgMecAppManager::handleMessage - bg environment with id [" << id << "] stopped" << endl;
 }

bool BgMecAppManager::relocateBgMecApp(int appId, cModule* mecHost)
{
    EV << "BgMecAppManager::relocateBgMecApp - relocating app " << appId << endl;
    if(bgMecApps_.find(appId) == bgMecApps_.end())
    {
        throw cRuntimeError("BgMecAppManager::relocateBgMecApp mec app %d does not exist",appId);
    }
    BgMecAppDescriptor appDescriptor = bgMecApps_[appId];
    appDescriptor.mecHost = mecHost;

    deleteBgMecApp(appId);

    // this does the insertion into the map
    bgMecApps_[appId] = appDescriptor;

    cModule* bgAppModule = createBgMecApp(appId);
    bgMecApps_[appId].bgMecApp = bgAppModule;

    return true;
 }


 void BgMecAppManager::handleMessage(cMessage* msg)
{
    if(msg->isSelfMessage())
    {
        // ==============================
        //      SNAPSHOT UPDATE
        // ==============================
        if(msg->isName("snapshotMsg"))
        {
            // read running bgApp
            int numApps = snapshotList_.front().numMecApps;
            EV << "BgMecAppManager::handleMessage (snapshotMsg) - current number of BG Mec Apps " << currentBgMecApps_ << ", expected " << numApps << endl;

            emit(totalAppSignal_,numApps);
            // call orchestration algorithm HERE
            //doOrchestration( numApps );

            updateBgMecAppsLoad( numApps );

            //schedule next snapshot
            snapshotList_.pop_front();
            bgAppsVector_.record(currentBgMecApps_);
            scheduleNextSnapshot();
        }// ==============================


        // ==============================
        //   PERIODIC LOAD BALANCING
        // ==============================
        else if(msg->isName("balancingTimer"))
        {
            EV << "BgMecAppManager::handleMessage (balanceTimer) - re-balancing load among servers" << endl;
            doOrchestration( currentBgMecApps_ );

            updateBgMecAppsLoad(currentBgMecApps_);
            scheduleAfter(balancingInterval_, balancingTimer_);
        }
        // ==============================


        // ==============================
        //   DELAYED MEC HOST ACTIVATION
        // ==============================
        else if(msg->isName("mecHostActivation"))
        {
            hostActivationTriggered_ = false;
            activateNewMecHost();
            delete msg;
        }// ==============================



        // ==========================
        //          IGNORE THIS
        // ==========================
        else if(msg->isName("deltaMsg"))
        {
            if(mode_ == CREATE)
            {
                bool res = createBgModules();
                EV << "BgMecAppManager::handleMessage (deltaMsg) - currentBgMecApps: " << currentBgMecApps_ << endl;
                if(currentBgMecApps_ == maxBgMecApp_/2)
                {
                    EV << "BgMecAppManager::handleMessage (deltaMsg) - Scheduled activation of a new Mec host in " << mecHostActivationTime_ << " seconds" << endl;
                    cMessage* activateMecHostMsg = new cMessage("mecHostActivation");
                    scheduleAfter(mecHostActivationTime_, activateMecHostMsg);
                }

                if(currentBgMecApps_ == maxBgMecApp_ || res == false)
                         mode_ = DELETE;
            }
            else
            {
                deleteBgModules();
                if(currentBgMecApps_ == 0)
                    mode_ = CREATE;
            }
            scheduleAfter(deltaTime_, deltaMsg_);
            bgAppsVector_.record(currentBgMecApps_);
        }
        // ==========================

    }
}

void BgMecAppManager::doOrchestration( int numApps )
{
    switch(orchestrationType_)
    {
        case DUMMY_ORCHESTRATION:
            dummyOrchestration( numApps );
            break;
        case EXTERNAL_ORCHESTRATION:
            externalOrchestration(numApps);
            break;
        default:
            dummyOrchestration( numApps );
            break;
    }
    return;
}

void BgMecAppManager::triggerMecHostActivation()
{
    if(hostActivationTriggered_)
    {
        EV << "BgMecAppManager::triggerMecHostActivation - host activation already in progress" << endl;
    }
    if(enableHostActivationDelay_)
    {
        cMessage* activateMecHostMsg = new cMessage("mecHostActivation");
        scheduleAfter(mecHostActivationTime_, activateMecHostMsg);
        EV << "BgMecAppManager::triggerMecHostActivation - Scheduling MEC HOST activation in " << mecHostActivationTime_ << " seconds" << endl;
        hostActivationTriggered_ = true;
    }
    else
    {
        EV << "BgMecAppManager::triggerMecHostActivation - Instantaneous MEC HOST activation" << endl;
        activateNewMecHost();
    }
}

void BgMecAppManager::dummyOrchestration( int numApps )
{
    EV << "BgMecAppManager::dummyOrchestration - apps per hosts: " << numApps/runningMecHosts_.size() << ". max[" << maxBgMecApp_ << "] - min[" << minBgMecApp_ << "]" << endl;
    if( numApps/runningMecHosts_.size() >= maxBgMecApp_)
    {
        EV << "BgMecAppManager::dummyOrchestration - Triggering activation of a new Mec host." << endl;
        triggerMecHostActivation();

    }
    else if( numApps/runningMecHosts_.size() <= minBgMecApp_ )
    {
        EV << "BgMecAppManager::dummyOrchestration - Scheduled deactivation of a Mec host now" << endl; // in " << mecHostActivationTime_ << " seconds" << endl;
        deactivateLastMecHost();
    }
}

void BgMecAppManager::externalOrchestration(int numApps)
{
    EV << "BgMecAppManager::externalOrchestration - calling external orchestrator for " << numApps << " tasks" << endl;

    std::stringstream cmd;
    cmd << "python3 ./external_orchestration.py ";

    // k: current number of tasks
    // m: number of servers
    // n: server capacity
    // as the orchestration is slightly offset, we substract 1ms to be aligned with snapshots
    cmd << numApps << " " << lastMecHostActivated_+1 << " " << maxBgMecApp_ << " " << simTime()-0.001;

    orchestrationPolicy_ = par("orchestrationPolicy").stringValue();
    if( orchestrationPolicy_.compare("prediction") == 0 )
        cmd <<" "<< "oracle"  << " " << par("predictionFileName").stringValue();
    else
        cmd <<" "<< orchestrationPolicy_  << " " << par("traceFileName").stringValue();

    cmd << " > decisionFile.txt";
    std::string commandString = cmd.str();

    std::cout << simTime() << " " << cmd.str()<< std::endl;
    EV << "BgMecAppManager::externalOrchestration - launching command " << commandString << endl;
    system(cmd.str().c_str());

    std::ifstream inputFileStream;
    inputFileStream.open("decisionFile.txt");

    int activate;
    inputFileStream >> activate;

    std::cout << simTime() << " " << activate << std::endl;

    emit(activationDecisionSignal_,activate);

    //============== EXTREME EDGE ORCHESTRATION ==============
    // verify the availability of extreme-edge resources
    unsigned int reqGNB = -1, respGNB = -1;
    if( orchestratedApp_ != nullptr )
    {
        reqGNB = orchestratedApp_->getCurrentgNB();
    }
    if( orchestratedResponder_ != nullptr )
    {
        respGNB = orchestratedResponder_->getCurrentgNB();
    }

    EV << "BgMecAppManager::externalOrchestration - APP on gNB " << reqGNB << " - exEdge resource on gNB " << respGNB << endl;
    // if a new server would be needed, but an extreme edge node is available
//    if( activate == 1 && reqGNB == respGNB )
//    {
//        activate = 3;
//    }
    //========================================================

    EV << "BgMecAppManager::externalOrchestration - decision is " << activate << endl;
    if ( activate == 1 )
        triggerMecHostActivation();
    else if( activate == -1 )
    {
        deactivateLastMecHost();
        changeServingEdge(2); // set to EDGE
    }
    else if( activate == 2 ) // set to EDGE
        changeServingEdge(activate);
    else if( activate == 3 ) // set to extreme EDGE
        changeServingEdge(activate);
    else
        return;

    return;
}

void BgMecAppManager::updateBgMecAppsLoad(int numApps)
{
    int deltaApps = numApps - currentBgMecApps_;
    EV << "BgMecAppManager::updateBgMecAppsLoad - currentApps[" << currentBgMecApps_ << "] - targetApps[" << numApps << "] - deltaApps[" << deltaApps << "]" << endl;

    // this avoid re-balancing the load when the load in unchanged
    if( (lastBalancedApps_ == currentBgMecApps_) && (lastBalancedHosts_ == lastMecHostActivated_) && deltaApps == 0 )
    {
        std::cout << simTime() << " balancing skipped" << std::endl;
        EV << "BgMecAppManager::updateBgMecAppsLoad - nothing to do" << endl;
        return;
    }
    lastBalancedApps_ = currentBgMecApps_;
    lastBalancedHosts_ = lastMecHostActivated_;

    // ==================== Service REMOVAL ====================
    // first delete applications if needed
    if( deltaApps < 0 )
    {
        for( int n = 0 ; n < -deltaApps ; n++ )
            deleteBgModules();
    }
    // =========================================================


    // ==================== Service RELOCATION =================
    // relocate a total of currentBgMecApps_ over lastMecHostActivated_
    int appPerHost = floor(currentBgMecApps_ / (lastMecHostActivated_+1));
    EV << "BgMecAppManager::updateBgMecAppsLoad - RELOCATION: appsPerHost = " << currentBgMecApps_<< " / " << lastMecHostActivated_+1 << " = " << appPerHost << endl;
    int hostId = 0, appId = 0, relocated = 0;

    for( hostId = 0 ; hostId <= lastMecHostActivated_ ; ++hostId )
    {
        for( appId = 0 ; appId < appPerHost ; ++appId )
        {
            relocateBgMecApp(appId+(hostId*appPerHost), runningMecHosts_[hostId]);
            ++relocated;
            EV << "\t " << appId << "]" << appId+(hostId*appPerHost) << "/" << appPerHost << " on " << hostId << ". (total " << relocated << ")"<< endl;
        }
    }

    // handle remaining app (in case currentBgMecApps_ is not a multiple of lastMecHostActivated_
    if( relocated < currentBgMecApps_ )
    {
        EV << "BgMecAppManager::updateBgMecAppsLoad - relocating last app on host 0" << endl;
        relocateBgMecApp(relocated, runningMecHosts_[0]);
    }
    // =========================================================


    // ================ New Service CREATION ===================
    // create applications if needed
    if( deltaApps > 0 )
    {
        // activate a total of deltaApps over lastMecHostActivated_
        appPerHost = floor(deltaApps / (lastMecHostActivated_+1));
        int created = 0;
        EV << "BgMecAppManager::updateBgMecAppsLoad - CREATION: appsPerHost = " << deltaApps<< " / " << lastMecHostActivated_+1 << " = " << appPerHost << endl;

        for( hostId = 0 ; hostId <= lastMecHostActivated_ ; ++hostId )
        {
            for( appId = 0 ; appId < appPerHost ; ++appId )
            {
                createBgModules(runningMecHosts_[hostId]);
                ++created;
                EV << "\t " << appId << "]" << appId << "/" << appPerHost << " on " << hostId << ". (total " << created << ")"<< endl;
            }
        }
        // handle remaining app (in case deltaApps is not a multiple of lastMecHostActivated_
        if( created < deltaApps )
        {
            EV << "BgMecAppManager::updateBgMecAppsLoad - creating last app on host "<< lastMecHostActivated_ << endl;
            createBgModules(runningMecHosts_[lastMecHostActivated_]);
        }
    }
    std::cout << simTime() << " balancing done" << std::endl;
    // =========================================================
}


cModule* BgMecAppManager::createBgMecApp(int id)
{
    VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(bgMecApps_[id].mecHost->getSubmodule("vim"));

    //create bgMecApp module and get the id used for register the mec app to the VIM
    cModuleType *moduleType = cModuleType::get("simu5g.nodes.mec.bgMecAppManager.bgModules.BgMecApp");         //MEAPP module package (i.e. path!)
    std::string moduleName("bgMecApp_");
    std::stringstream appName;
    appName << moduleName << id;

//    cModule *module = moduleType->create(appName.str().c_str(), bgMecApps_[id].mecHost);       //MEAPP module-name & its Parent Module
    //or
    cModule *appModule = moduleType->createScheduleInit(appName.str().c_str(), bgMecApps_[id].mecHost);       //MEAPP module-name & its Parent Module

    appModule->setName(appName.str().c_str());
    int moduleId = appModule->getId();

    // register the MEC app on the VIM of the chosen MEC host
    bool success = vim->registerMecApp(moduleId, bgMecApps_[id].resources.ram, bgMecApps_[id].resources.disk, bgMecApps_[id].resources.cpu, admissionControl_);

    if(success)
    {
        EV << "BgMecAppManager::createBgMecApp: bgMecApp created " << appName.str() << endl;
        return appModule;
    }
    else{
        EV << "BgMecAppManager::createBgMecApp: bgMecApp NOT created " << appName.str() << endl;
        appModule->deleteModule();
        return nullptr;
    }
}

void BgMecAppManager::deleteBgMecApp(int id)
{
    VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(bgMecApps_[id].mecHost->getSubmodule("vim"));

    bool success = vim->deRegisterMecApp(bgMecApps_[id].bgMecApp->getId());
    if(success)
    {
        if(bgMecApps_[id].bgMecApp != nullptr)
        {
            bgMecApps_[id].bgMecApp->deleteModule();
            EV << "BgMecAppManager::deleteBgMecApp: bgMecApp with id [" << id << "] deleted " << endl;
        }
    }
}

cModule* BgMecAppManager::createBgUE(int id)
{
    cModuleType *moduleType = cModuleType::get("simu5g.nodes.mec.bgMecAppManager.bgModules.BgUe");         //MEAPP module package (i.e. path!)
    std::string moduleName("bgUe_");
    std::stringstream appName;
    appName << moduleName << id;

    cModule *module = moduleType->create(appName.str().c_str(), getParentModule());       //MEAPP module-name & its Parent Module
    module->setName(appName.str().c_str());

    double x,y;
    x = bgMecApps_[id].centerX + uniform(bgMecApps_[id].radius/2, bgMecApps_[id].radius)*(intuniform(0, 1)*2 -1);
    y = bgMecApps_[id].centerY + uniform(bgMecApps_[id].radius/2, bgMecApps_[id].radius)*(intuniform(0, 1)*2 -1);
    module->finalizeParameters();
    module->buildInside();

//    module->getSubmodule("mobility")->par("initialX") = x;
//    module->getSubmodule("mobility")->par("initialY") = y;
    module->callInitialize();

    std::stringstream display;
    display << "i=device/pocketpc;p=" << x << "," << y;
    module->setDisplayString(display.str().c_str());
//    module->getSubmodule("mobility")->setDisplayString(display.str().c_str());


    if(module != nullptr)
    {
        EV << "BgMecAppManager::createBgUE: BgUe created " << appName.str() << endl;
        return module;
    }
    else{
        EV << "BgMecAppManager::createBgUE: BgUe NOT created " << appName.str() << endl;
        module->deleteModule();
        return nullptr;
    }

}

void BgMecAppManager::deleteBgUe(int id)
{
    if(bgMecApps_[id].bgUe != nullptr)
    {
        bgMecApps_[id].bgUe->deleteModule();
        EV << "BgMecAppManager::deleteBgUe: BGUe with id [" << id << "] deleted " << endl;
    }
}

void BgMecAppManager::readMecHosts()
{
    EV <<"BgMecAppManager::readMecHosts" << endl;
    //getting the list of mec hosts associated to this mec system from parameter
    if(hasPar("mecHostList") && strcmp(par("mecHostList").stringValue(), "")){
        std::string mecHostList = par("mecHostList").stdstringValue();
        EV <<"BgMecAppManager::readMecHosts list " << (char*)par("mecHostList").stringValue() << endl;
        char* token = strtok ((char*)mecHostList.c_str() , ", ");            // split by commas
        while (token != NULL)
        {
            EV <<"BgMecAppManager::readMecHosts list " << token << endl;
            cModule *mhModule = getSimulation()->getModuleByPath(token);
            mhModule->getDisplayString().setTagArg("i",1, "red");
            mecHosts_.push_back(mhModule);
            token = strtok (NULL, ", ");
        }
    }

    if(mecHosts_.size() >= 1)
        activateNewMecHost(); //activate the firstMecHostAdded
    return;
}


cModule* BgMecAppManager::chooseMecHost()
{
    return runningMecHosts_.back();
}

// the policy used to activate and deactivate does not require the  runningMecHosts_ var
// just the lastMecHostActivated_ is necessary, but more complex policies will be implemented

void BgMecAppManager::activateNewMecHost()
{
    if(lastMecHostActivated_ == mecHosts_.size() -1)
    {
        EV << "BgMecAppManager::activateNewMecHost() - no more mecHosts are available" << endl;
    }
    else
    {
        std::cout << simTime() << " activating mec host" << std::endl;
        EV << "BgMecAppManager::activateNewMecHost() - turning on Mec host with index " << lastMecHostActivated_+1  << endl;
        cModule* mh = mecHosts_[++lastMecHostActivated_];
        mh->getDisplayString().setTagArg("i",1, "green");
        runningMecHosts_.push_back(mh);
    }
}

void BgMecAppManager::deactivateNewMecHost(cModule* mecHost)
{
    if(runningMecHosts_.size() == 1)
    {
        EV << "BgMecAppManager::deactivateNewMecHost: at least one MEC host must be present" << endl;
        return;
    }
    auto it = runningMecHosts_.begin();
    for(; it != runningMecHosts_.end(); ++it)
    {
        if(*it == mecHost)
        {
            EV << "BgMecAppManager::deactivateNewMecHost() - mec Host deactivated" << endl;
            (*it)->getDisplayString().setTagArg("i",1, "red");
            runningMecHosts_.erase(it);
            lastMecHostActivated_--;
            return;
        }
    }
    EV << "BgMecAppManager::deactivateNewMecHost() - mec Host not found in running mecHostList" << endl;
}

void BgMecAppManager::deactivateLastMecHost()
{
    if(runningMecHosts_.size() == 1)
    {
        EV << "BgMecAppManager::deactivatLastMecHost: at least one MEC host must be present" << endl;
        return;
    }

    runningMecHosts_.back()->getDisplayString().setTagArg("i",1, "red");
    runningMecHosts_.pop_back();
    lastMecHostActivated_--;

    EV << "BgMecAppManager::deactivatLastMecHost - mec Host deactivated" << endl;
    return;
}

bool BgMecAppManager::isMecHostEmpty(cModule* mecHost)
{
    for(auto& mh : runningMecHosts_)
    {
        if(mh == mecHost)
        {
            VirtualisationInfrastructureManager* vim = check_and_cast<VirtualisationInfrastructureManager*>(mecHost->getSubmodule("vim"));
            return (vim->getCurrentMecApps() == 0);
        }
    }
    EV << "BgMecAppManager::isEmpty - the mecHost is not running" << endl;
    throw cRuntimeError("BgMecAppManager::isEmpty - the mecHost [%s] is not running", mecHost->getFullName());
}


void BgMecAppManager::changeServingEdge( int action )
{
    if( action == 2 )
        orchestratedApp_->setCurrentResponder(PRIMARY);
    if( action == 3 )
        orchestratedApp_->setCurrentResponder(SECONDARY);
}
