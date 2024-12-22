#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <bits/stdc++.h>

/*
Written by Govind S Ashan (ME23B168)

Example of a Publisher-Subscriber Model

There are 3 main classes:
    1. Broker Class
    2. Publisher Class
    3. Subscriber Class

Broker Class: 
    Creating an object of the Broker class initializes the network. The Broker class manages and stores Publisher and Subscriber objects, facilitating communication between them. Interaction between Subscriber and Publisher is seamlessly handled through the Broker.

    Variables:
        1. equityPublisher_
        2. bondPublisher_
        3. subscribers_
    Methods:
        1. get_equity_publisher
        2. get_bond_publisher
        3. get_subscriber
        4. add_subscriber
        3. err_msg

Publisher Class:

    2 Derived Classes:
        1. EquityPublisher
        2. BondPublisher
    Variables:
        1. instrumentData_
        2. instrumentSubscribers_
    Methods:
        1. update_data
        2. get_data 
        3. subcscribe
        4. is_valid_instrument
    
Subscriber Class:

    2 Derived Classes:
        1. FreeSubscriber
        2. PaidPubscriber
    Variables:
        1. requestCount_
        2. MAX_REQUESTS
    Methods:
        1. subscribe
        2. get_data
        3. get_id
*/



// Macros defining instrument ID ranges for equity and bond instruments
#define EQUITY_INSTRUMENT_MAX 1000
#define BOND_INSTRUMENT_MAX 2000

// Struct representing message data for instruments
struct Message {
    std::uint64_t instrumentId;
    double lastTradedPrice;
    double bondYield;                // only for BondPublisher
    std::uint64_t lastDayVolume;     // only for EquityPublisher


    // Constructor to Initialise the message based on Instrument ID
    Message(std::uint64_t instrumentId_, double lastTradedPrice_, double bondYieldOrLastDayVolume) {
        instrumentId = instrumentId_;
        lastTradedPrice = lastTradedPrice_;
        if (instrumentId < EQUITY_INSTRUMENT_MAX)
            bondYield = bondYieldOrLastDayVolume;
        else if (instrumentId < BOND_INSTRUMENT_MAX)
            lastDayVolume = bondYieldOrLastDayVolume;
    }

    Message() {}
};

// Forward Declarations
class Subscriber;
class Broker;

// Publisher Base Class
class Publisher {
protected:
    std::unordered_map<std::uint64_t, Message> instrumentData_; // Last Received Message for each instrumentID, Map (InstrumentID, Message)
    std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> instrumentSubscribers_; // Map (SubscriberID, InstrumentID)

public:
    virtual ~Publisher() = default;

    virtual void update_data(std::uint64_t instrumentId, double lastTradedPrice, double additionalData) = 0;

    void subscribe(std::uint64_t instrumentId, Subscriber* subscriber);

    bool get_data(std::uint64_t instrumentId, Message& data) const;

    auto get_instrumentSubscribers() {
        return instrumentSubscribers_;
    }

    virtual bool is_valid_instrument(std::uint64_t instrumentId) const = 0;
};

// EquityPublisher Class derived from Publisher class
class EquityPublisher : public Publisher {
public:
    void update_data(std::uint64_t instrumentId, double lastTradedPrice, double lastDayVolume) override {
        if (is_valid_instrument(instrumentId)) {
            Message msg = Message(instrumentId, lastTradedPrice, lastDayVolume);
            instrumentData_[instrumentId] = msg;
        }
    }

    bool is_valid_instrument(std::uint64_t instrumentId) const override {
        return instrumentId < EQUITY_INSTRUMENT_MAX;
    }
};

// BondPublisher Class derived from Publisher Class
class BondPublisher : public Publisher {
public:
    void update_data(std::uint64_t instrumentId, double lastTradedPrice, double bondYield) override {
        if (is_valid_instrument(instrumentId)) {
            Message msg = Message(instrumentId, lastTradedPrice, bondYield);
            instrumentData_[instrumentId] = msg;
        }
    }

    bool is_valid_instrument(std::uint64_t instrumentId) const override {
        return instrumentId >= EQUITY_INSTRUMENT_MAX && instrumentId < BOND_INSTRUMENT_MAX;
    }
};

// Subscriber Base Class
class Subscriber {
protected:
    std::uint64_t id_; // Subscriber ID
    Broker& broker_; // Broker associated with the subscriber

public:
    Subscriber(std::uint64_t id, Broker& broker) : id_(id), broker_(broker) {}
    virtual ~Subscriber() = default;

    void subscribe(std::uint64_t instrumentId);

    uint64_t get_id() {
        return id_;
    }

    
    virtual bool get_data(std::uint64_t instrumentId) = 0;
};

// Broker Class ( Handles Publishers and Subscribers in the network )
class Broker {
private:
    EquityPublisher equityPublisher_; // Initialisation of Equity Publisher
    BondPublisher bondPublisher_; // Initialisation of Bond Publisher
    std::unordered_map<std::uint64_t, std::unique_ptr<Subscriber>> subscribers_; // Map (SubscriberID, Subscriber)

public:
    EquityPublisher& get_equity_publisher() { return equityPublisher_; }
    BondPublisher& get_bond_publisher() { return bondPublisher_; }

    Subscriber* get_subscriber(std::uint64_t subscriberId) {
        if (subscribers_.count(subscriberId)) {
            return subscribers_[subscriberId].get();
        }
        return nullptr;
    }

    void add_subscriber(std::uint64_t subscriberId, std::unique_ptr<Subscriber> subscriber) {
        subscribers_[subscriberId] = std::move(subscriber);
    }

    void err_msg(char type, std::uint64_t subscriberID, std::uint64_t instrumentID) {
        std::cout<<type<<", "<<subscriberID<<", "<<instrumentID<<", invalid request"<<std::endl;;
    }

};

// Maps SubscriberID to InstrumentID in the respective Publisher
void Publisher::subscribe(std::uint64_t instrumentId, Subscriber* subscriber) {
    auto& instrumentIDs = instrumentSubscribers_[subscriber->get_id()]; // Access or create the vector
    if (std::find(instrumentIDs.begin(), instrumentIDs.end(), instrumentId) == instrumentIDs.end()) {
        instrumentIDs.push_back(instrumentId); // Add only if not already subscribed
    }
}

// Latest data corresponding to the InstrumentID is procured
bool Publisher::get_data(std::uint64_t instrumentId, Message& data) const {
    if (instrumentData_.count(instrumentId)) {
        data = instrumentData_.at(instrumentId);
        return true;
    }
    return false;
}

// Calls Publisher::subscribe for the relevant publisher
void Subscriber::subscribe(std::uint64_t instrumentId) {
    if (broker_.get_equity_publisher().is_valid_instrument(instrumentId)) {
        broker_.get_equity_publisher().subscribe(instrumentId, this);
    } else if (broker_.get_bond_publisher().is_valid_instrument(instrumentId)) {
        broker_.get_bond_publisher().subscribe(instrumentId, this);
    }
}

// FreeSubscriber Class
class FreeSubscriber : public Subscriber {
private:
    int requestCount_ = 0;
    static const int MAX_REQUESTS = 100;

public:
    FreeSubscriber(std::uint64_t id, Broker& broker) : Subscriber(id, broker) {}

    // Calls Publisher::get_data for the relevant Publisher, also keeps track of the Request Count
    bool get_data(std::uint64_t instrumentId) override {
        if (requestCount_ >= MAX_REQUESTS) {
            broker_.err_msg('F',id_,instrumentId);
            return false;
        }

        Message data;
        auto& equityPublisher = broker_.get_equity_publisher();
        auto& bondPublisher = broker_.get_bond_publisher();
        
        if (equityPublisher.is_valid_instrument(instrumentId)) {
            auto it = equityPublisher.get_instrumentSubscribers().find(id_);
            if (it != equityPublisher.get_instrumentSubscribers().end() ) {
                if (std::find(it->second.begin(),it->second.end(),instrumentId)!=it->second.end()) {
                    equityPublisher.get_data(instrumentId, data);
                    std::cout << "F," << id_ << "," << instrumentId << "," << data.lastTradedPrice << "," << data.lastDayVolume << "\n";
                    ++requestCount_;
                    return true;
                }
            }
        }

        else if (bondPublisher.is_valid_instrument(instrumentId)) {
            auto it = bondPublisher.get_instrumentSubscribers().find(id_);
            if (it != bondPublisher.get_instrumentSubscribers().end()) {
                if (std::find(it->second.begin(), it->second.end(), instrumentId) != it->second.end()) {
                    bondPublisher.get_data(instrumentId, data);
                    std::cout << "F," << id_ << "," << instrumentId << "," << data.lastTradedPrice << "," << data.bondYield << "\n";
                    ++requestCount_;
                    return true;
                }
            }       
        }
        broker_.err_msg('F',id_,instrumentId);
        return false;
    }
};

// PaidSubscriber Class
class PaidSubscriber : public Subscriber {
public:
    PaidSubscriber(std::uint64_t id, Broker& broker) : Subscriber(id, broker) {}


    // Calls Publisher::get_data for the relevant Publisher
    bool get_data(std::uint64_t instrumentId) override {
        Message data;
        auto& equityPublisher = this->broker_.get_equity_publisher();
        auto& bondPublisher = this->broker_.get_bond_publisher();

        if (equityPublisher.is_valid_instrument(instrumentId)) {
            auto it = equityPublisher.get_instrumentSubscribers().find(id_);
            if (it != equityPublisher.get_instrumentSubscribers().end()) {
                if (std::find(it->second.begin(),it->second.end(),instrumentId) != it->second.end()) {
                    equityPublisher.get_data(instrumentId, data);
                    std::cout << "F," << id_ << "," << instrumentId << "," << data.lastTradedPrice << "," << data.lastDayVolume << "\n";
                    return true;
                }
            }
        }

        else if (bondPublisher.is_valid_instrument(instrumentId)) {
            auto it = bondPublisher.get_instrumentSubscribers().find(id_);
            if (it != bondPublisher.get_instrumentSubscribers().end()) {
                if (std::find(it->second.begin(), it->second.end(), instrumentId) != it->second.end()) {
                    bondPublisher.get_data(instrumentId, data);
                    std::cout << "F," << id_ << "," << instrumentId << "," << data.lastTradedPrice << "," << data.bondYield << "\n";
                    return true;
                }
            }       
        }


        broker_.err_msg('P',id_,instrumentId);
        return false;
    }
};

// Function to Parse the input strings and do the relevant operations
void parseInput(const std::string& input, Broker &broker_) {
    std::istringstream iss(input);
    std::string command;
    iss >> command;

    if (command == "P") {
        // Calls Publisher::update_data for the relavant Publisher
        uint64_t instrumentId;
        double lastTradedPrice, bondYieldOrLastDayVolume;
        iss >> instrumentId >> lastTradedPrice >> bondYieldOrLastDayVolume;
        
        if (instrumentId<EQUITY_INSTRUMENT_MAX) {
            auto& equity_publisher_ = broker_.get_equity_publisher();
            equity_publisher_.update_data(instrumentId,lastTradedPrice,bondYieldOrLastDayVolume);
        }
        else if (instrumentId<BOND_INSTRUMENT_MAX) {
            auto& bond_publisher_ = broker_.get_bond_publisher();
            bond_publisher_.update_data(instrumentId,lastTradedPrice,bondYieldOrLastDayVolume);
        }

        else {
            std::cout<<"No Publisher matches the given Instrument ID"<<std::endl;
        }

    } else if (command == "S") {
        
        char type;
        uint64_t subscriberId, instrumentId;
        std::string action;
        iss >> type >> subscriberId >> action >> instrumentId;

        // Calls Subscribe::subscribe
        if (action == "subscribe") {
            if (type == 'P') {
                auto subscriber = std::make_unique<PaidSubscriber>(subscriberId, broker_);
                broker_.add_subscriber(subscriberId, std::move(subscriber));
            } else {
                auto subscriber = std::make_unique<FreeSubscriber>(subscriberId, broker_);
                broker_.add_subscriber(subscriberId, std::move(subscriber));
            }

            broker_.get_subscriber(subscriberId)->subscribe(instrumentId);

            std::cout << "Subscriber " << subscriberId << " of type " << type << " subscribed to "
                    << instrumentId << "\n";

        // Calls Subscriber::get_data
        } else if (action == "get_data") {
            
            if (broker_.get_subscriber(subscriberId)!= nullptr) {
                if (!broker_.get_subscriber(subscriberId)->get_data(instrumentId)) broker_.err_msg(type, subscriberId, instrumentId);
            }
            else broker_.err_msg(type, subscriberId, instrumentId);
        } else {
            broker_.err_msg(type, subscriberId, instrumentId);
        }
    } else {
        std::cout << "Unknown command: " << command << "\n";
    }
}

auto generate_publisher_update_message = [] {
  std::string msg = "P ";
  std::uint64_t instrumentId = rand() % 2000;
  msg += std::to_string(instrumentId) + " ";
  msg += std::to_string(rand()) + " ";
  if (instrumentId >= 1000)
    // Bond update
    // bond yield is percentage (< 100)
    msg += std::to_string(static_cast<double>(rand() % 10000) / 100);
  else
    // Equity update
    // last day volume
    msg += std::to_string(rand());

  msg += "\n";
  return msg;
};

auto generate_subscribe_message = [] {
  std::string msg = "S ";
  if(rand() % 2)
      msg += "F ";
  else
      msg += "P ";
  std::uint64_t subscriberId = rand();
  msg += std::to_string(subscriberId) + " ";
  msg += "subscribe ";
  msg += std::to_string(rand() % 2000) + "\n";

  return msg;
};

auto generate_get_data_message = [] {
  std::string msg = "S ";
  if(rand() % 2)
      msg += "F ";
  else
      msg += "P ";
  std::uint64_t subscriberId = rand();
  msg += std::to_string(subscriberId) + " ";
  msg += "get_data ";
  msg += std::to_string(rand() % 2000) + "\n";

  return msg;
};


int main() {
    Broker broker;
    for (int i =0 ; i<100 ; i++) {
        
        std::string command = generate_publisher_update_message();
        parseInput(command,broker);
        command = generate_subscribe_message();
        parseInput(command,broker);
        command = generate_get_data_message();
        parseInput(command, broker);
    }

   
}
