#ifndef __WHEEL_SUBSCRIBER
#define __WHEEL_SUBSCRIBER

// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
// Copyright 2025 Bernd Porr, mail@berndporr.me.uk
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

#include "brakePubSubTypes.hpp"
#include "steeringPubSubTypes.hpp"
#include "throttlePubSubTypes.hpp"
#include "buttonPubSubTypes.hpp"

#include "steering_wheel/logiwheel.h"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

class RobotSubscriber
{
public:

private:

    DomainParticipant* participant = nullptr;

    Subscriber* subscriber = nullptr;

    DataReader* readerSteering = nullptr;
    DataReader* readerThrottle = nullptr;
    DataReader* readerButton = nullptr;

    Topic* topicSteering = nullptr;
    Topic* topicThrottle = nullptr;
    Topic* topicButton = nullptr;

    TypeSupport typeSteering;
    TypeSupport typeThrottle;
    TypeSupport typeButton;

    class SteeringListener : public DataReaderListener
    {
    public:
        SteeringListener() {}
        ~SteeringListener() override {}

	LogiWheel::AnalogueCallback callback;
	
	// callback
        void on_data_available(DataReader* reader) override
	    {
		SampleInfo info;
		SteeringMsg msg;
		if (reader->take_next_sample(&msg, &info) == eprosima::fastdds::dds::RETCODE_OK)
		{
		    if (callback && info.valid_data) callback(msg.steering());
		}
	    }
    } listenerSteering;

    class ThrottleListener : public DataReaderListener
    {
    public:
        ThrottleListener() {}
        ~ThrottleListener() override {}

	LogiWheel::AnalogueCallback callback;
	
	// callback
        void on_data_available(DataReader* reader) override
	    {
		SampleInfo info;
		ThrottleMsg msg;
		if (reader->take_next_sample(&msg, &info) == eprosima::fastdds::dds::RETCODE_OK)
		{
		    if (callback && info.valid_data) callback(msg.throttle());
		}
	    }
    } listenerThrottle;

    class ButtonListener : public DataReaderListener
    {
    public:
        ButtonListener() {}
        ~ButtonListener() override {}

	LogiWheel::ButtonCallback callback;

	// callback
        void on_data_available(DataReader* reader) override
	    {
		SampleInfo info;
		ButtonMsg msg;
		if (reader->take_next_sample(&msg, &info) == eprosima::fastdds::dds::RETCODE_OK)
		{
		    if (callback && info.valid_data) callback(msg.index());
		}
	    }
    } listenerButton;

public:

    RobotSubscriber() : typeSteering(new SteeringMsgPubSubType()),
			typeThrottle(new ThrottleMsgPubSubType()),
			typeButton(new ButtonMsgPubSubType())
	{}

    virtual ~RobotSubscriber()
	{
	    if (readerSteering != nullptr) subscriber->delete_datareader(readerSteering);
	    if (topicSteering != nullptr) participant->delete_topic(topicSteering);

	    if (readerThrottle != nullptr) subscriber->delete_datareader(readerThrottle);
	    if (topicThrottle != nullptr) participant->delete_topic(topicThrottle);

	    if (readerButton != nullptr) subscriber->delete_datareader(readerButton);
	    if (topicButton != nullptr) participant->delete_topic(topicButton);

	    if (subscriber != nullptr) participant->delete_subscriber(subscriber);

	    DomainParticipantFactory::get_instance()->delete_participant(participant);
	}

    void registerSteeringCallback(LogiWheel::AnalogueCallback cb) {
	listenerSteering.callback = cb;
    }

    void registerThrottleCallback(LogiWheel::AnalogueCallback cb) {
	listenerThrottle.callback = cb;
    }

    void registerButtonCallback(LogiWheel::ButtonCallback cb) {
	listenerButton.callback = cb;
    }

    //!Initialize the subscriber
    bool init()
	{
	    DomainParticipantQos participantQos;
	    participantQos.name("Participant_subscriber");
	    participant = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

	    if (participant == nullptr)
	    {
		return false;
	    }

	    // Register the Types
	    typeSteering.register_type(participant);
	    typeThrottle.register_type(participant);
	    typeButton.register_type(participant);

	    // Create the Subscriber
	    subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
	    if (subscriber == nullptr) return false;

	    // Topic Steering
	    topicSteering = participant->create_topic("SteeringTopic", "SteeringMsg", TOPIC_QOS_DEFAULT);
	    if (topicSteering == nullptr) return false;
	    readerSteering = subscriber->create_datareader(topicSteering,
							    DATAREADER_QOS_DEFAULT,
							    &listenerSteering);
	    if (readerSteering == nullptr) return false;

	    // Topic Throttle
	    topicThrottle = participant->create_topic("ThrottleTopic", "ThrottleMsg", TOPIC_QOS_DEFAULT);
	    if (topicThrottle == nullptr) return false;
	    readerThrottle = subscriber->create_datareader(topicThrottle,
							    DATAREADER_QOS_DEFAULT,
							    &listenerThrottle);
	    if (readerThrottle == nullptr) return false;

	    // Topic Button
	    topicButton = participant->create_topic("ButtonTopic", "ButtonMsg", TOPIC_QOS_DEFAULT);
	    if (topicButton == nullptr) return false;
	    readerButton = subscriber->create_datareader(topicButton,
							 DATAREADER_QOS_DEFAULT,
							 &listenerButton);
	    if (readerButton == nullptr) return false;

	    return true;
	}

};

#endif
