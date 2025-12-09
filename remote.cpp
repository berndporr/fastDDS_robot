// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
// Copyright 2025 Bernd Porr
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "brakePubSubTypes.h"
#include "steeringPubSubTypes.h"
#include "throttlePubSubTypes.h"
#include "buttonPubSubTypes.h"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "logiwheel.h"

using namespace eprosima::fastdds::dds;

class RemotePublisher
{
private:

    DomainParticipant* participant = nullptr;

    Publisher* publisher = nullptr;

    Topic* topicSteering = nullptr;
    Topic* topicBrake = nullptr;
    Topic* topicThrottle = nullptr;
    Topic* topicButton = nullptr;

    DataWriter* writerSteering = nullptr;
    DataWriter* writerBrake = nullptr;
    DataWriter* writerThrottle = nullptr;
    DataWriter* writerButton = nullptr;

    TypeSupport typeSteering;
    TypeSupport typeBrake;
    TypeSupport typeThrottle;
    TypeSupport typeButton;

public:

    RemotePublisher() : typeSteering(new SteeringMsgPubSubType()),
			typeBrake(new BrakeMsgPubSubType()),
			typeThrottle(new ThrottleMsgPubSubType()),
			typeButton(new ButtonMsgPubSubType())
	{}

    virtual ~RemotePublisher()
	{
	    if (writerSteering != nullptr)
		publisher->delete_datawriter(writerSteering);
	    if (topicSteering != nullptr)
		participant->delete_topic(topicSteering);
	    
	    if (writerBrake != nullptr)
		publisher->delete_datawriter(writerBrake);
	    if (topicBrake != nullptr)
		participant->delete_topic(topicBrake);
	    
	    if (writerThrottle != nullptr)
		publisher->delete_datawriter(writerThrottle);
	    if (topicThrottle != nullptr)
		participant->delete_topic(topicThrottle);
	    
	    if (writerButton != nullptr)
		publisher->delete_datawriter(writerButton);
	    if (topicButton != nullptr)
		participant->delete_topic(topicButton);
	    
	    if (publisher != nullptr)
		participant->delete_publisher(publisher);
	    DomainParticipantFactory::get_instance()->delete_participant(participant);
	}

    //!Initialize the publisher
    bool init()
	{
	    DomainParticipantQos participantQos;
	    participantQos.name("Participant_publisher");
	    participant = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);
	    if (participant == nullptr) return false;

	    // Register the Types
	    typeSteering.register_type(participant);
	    typeBrake.register_type(participant);
	    typeThrottle.register_type(participant);
	    typeButton.register_type(participant);

	    // Create the Publisher
	    publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);
	    if (publisher == nullptr) return false;

	    // Steering
	    topicSteering = participant->create_topic("SteeringTopic", "SteeringMsg", TOPIC_QOS_DEFAULT);
	    if (topicSteering == nullptr) return false;
	    writerSteering = publisher->create_datawriter(topicSteering, DATAWRITER_QOS_DEFAULT);
	    if (writerSteering == nullptr) return false;
	
	    // Throttle
	    topicThrottle = participant->create_topic("ThrottleTopic", "ThrottleMsg", TOPIC_QOS_DEFAULT);
	    if (topicThrottle == nullptr) return false;
	    writerThrottle = publisher->create_datawriter(topicThrottle, DATAWRITER_QOS_DEFAULT);
	    if (writerThrottle == nullptr) return false;
	
	    // Brake
	    topicBrake = participant->create_topic("BrakeTopic", "BrakeMsg", TOPIC_QOS_DEFAULT);
	    if (topicBrake == nullptr) return false;
	    writerBrake = publisher->create_datawriter(topicBrake, DATAWRITER_QOS_DEFAULT);
	    if (writerBrake == nullptr) return false;

	    // Button
	    topicButton = participant->create_topic("ButtonTopic", "ButtonMsg", TOPIC_QOS_DEFAULT);
	    if (topicButton == nullptr) return false;
	    writerButton = publisher->create_datawriter(topicButton, DATAWRITER_QOS_DEFAULT);
	    if (writerBrake == nullptr) return false;

	    return true;
	}

    void publishSteering(SteeringMsg& msg)
	{
	    writerSteering->write(&msg);
	}
    
    void publishThrottle(ThrottleMsg& msg)
	{
	    writerThrottle->write(&msg);
	}

    void publishBrake(BrakeMsg& msg)
	{
	    writerBrake->write(&msg);
	}

    void publishButton(ButtonMsg& msg)
	{
	    writerButton->write(&msg);
	}
};


int main(int argc, char *argv[]) {
    LogiWheel logiwheel;
    RemotePublisher remotePublisher;
    
    logiwheel.registerSteeringCallback([&](float v){
	SteeringMsg msg;
	msg.steering(v);
	remotePublisher.publishSteering(msg);
    });

    logiwheel.registerThrottleCallback([&](float v){
	ThrottleMsg msg;
	msg.throttle(v);
	remotePublisher.publishThrottle(msg);
    });

    logiwheel.registerBrakeCallback([&](float v){
	BrakeMsg msg;
	msg.brake(v);
	remotePublisher.publishBrake(msg);
    });

    logiwheel.registerButtonCallback([&](int d){
	ButtonMsg msg;
	msg.index(d);
	remotePublisher.publishButton(msg);
    });

    if(!remotePublisher.init())
    {
	std::cerr << "Publisher init fail. Exiting." << std::endl;
	return -1;
    }
    if (!logiwheel.start()) {
	std::cerr << "Wheel init fail. Is it plugged in? Exiting." << std::endl;
	return -1;
    }
    printf("Press any key to stop it.\n");
    getc(stdin);
    logiwheel.stop();
    return 0;
}
