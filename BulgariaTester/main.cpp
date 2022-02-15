#include "../../Processor/SocketSender.hpp"
#include "../../Processor/Processor.hpp"
#include "../../Processor/ASender.hpp"

#include <iostream>
#include <cstdio>

#include <chrono>

#include <vector>

#include <array>
#include <thread>

#include <mutex>
#include <atomic>

#include <memory>

using namespace std::chrono_literals;


std::atomic<bool> can_work(true);

std::vector<float> values {123, 123, 123};
std::mutex values_control;
std::atomic<bool> values_was_changed(true);

const int MAX_CANALS_AMOUNT = 3;
//std::vector<std::shared_ptr<ocketSender<msg_t>>> senders;



void update_values (std::vector<float>& new_values) {
	std::unique_lock<std::mutex> lock (values_control);
	
	if (values.empty ()) {
		values = std::vector<float> (new_values);
	} else {
		for (size_t idx = 0; idx < new_values.size (); ++idx) {
			values[idx] = new_values[idx];
		}
		
		for (size_t idx = new_values.size () - 1; idx < values.size (); idx++) {
			values[idx] = 0;
		}
	}
	
	values_was_changed.store (true);
}


void edit_values () {
	int canals;
	std::cout << "Input amount of canals for editing: ";
	std::cin >> canals;
	
	canals = abs(canals);
	canals = canals > MAX_CANALS_AMOUNT? MAX_CANALS_AMOUNT: canals;
	
	std::vector<float> new_values (canals);
	for (int i = 0; i < canals;	++i) {
		std::cout << "Input value for [" << i + 1 << "] canal: ";
		
		std::cin >> new_values[i];
	}
	
	update_values (new_values);
}


void values_controller () {
	const char EDIT_BUTTON = 'L';
	
	while (can_work.load ()) {
		std::cout << "\n\nIf you want to change values, push the " << EDIT_BUTTON << " button;\n";
		std::cout << "Press 'Esc' for exit;\n";
		
		auto button = std::getchar ();
		
		if (button == EDIT_BUTTON ||
			button == EDIT_BUTTON + 32) {
			edit_values ();
			
		} else if (button == 27) {
			can_work.store (false);
		}
	}
}

void send (std::vector<std::shared_ptr<SocketSender<msg_t>>>& senders,
			std::vector<msg_t>& packets) {
	for (auto& packet : packets) {
		for (auto& sender : senders) {
			sender.get () -> set_value (packet);
			sender.get () -> send ();
		}
	}	
}


std::vector<msg_t> make_new_packets () {
	std::unique_lock<std::mutex> lock(values_control);
	std::vector<msg_t> packets (MAX_CANALS_AMOUNT);
	const short VALID_STATE = 0;
	
	// мдааааааааааааааа
	packets[0].packet_id = packets_id_t::PCKG_ID1;
	packets[1].packet_id = packets_id_t::PCKG_ID2;
	packets[2].packet_id = packets_id_t::PCKG_ID3;
	
	for (size_t idx = 0; idx < values.size (); idx++) {
		packets[idx].element[0].state = VALID_STATE;
		packets[idx].element[0].value = values[idx];
	}
	
	for (size_t idx = values.size (); idx < packets.size (); idx++) {
		packets[idx].element[0].state = 1;
	}
	
	return packets;
}


void packet_sender (std::vector<std::shared_ptr<SocketSender<msg_t>>>& senders) {
	std::vector<msg_t> packets;
	
	while (can_work.load ()) {
		std::this_thread::sleep_for(1s);
		
		if (values_was_changed.load ()) {
			packets = make_new_packets ();
			
			values_was_changed.store (false);
		}
		
		send (senders, packets);
	}
}

std::vector<std::shared_ptr<SocketSender<msg_t>>> create_senders () {
	std::vector<std::shared_ptr<SocketSender<msg_t>>> senders = {
		std::shared_ptr<SocketSender<msg_t>> (new SocketSender<msg_t> (std::string ("127.0.0.1"), 65000, proto_t::UDP)),
		std::shared_ptr<SocketSender<msg_t>> (new SocketSender<msg_t> (std::string ("127.0.0.1"), 65001, proto_t::UDP))
	};
	
	return senders;
}

int main(int argc, char **argv)
{
	std::cout << "Start  testing!\n";
	auto senders = create_senders ();
	
	std::array<std::thread, 2> threads {
		std::thread (values_controller),
		std::thread (packet_sender, std::ref(senders))
	};
	
	for (auto& thrd : threads) {
		thrd.join ();
	}
	
	std::cout << "Stopped testing!\n";
	return 0;
}
 