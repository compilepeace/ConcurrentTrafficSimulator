#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

 
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
  	std::unique_lock<std::mutex> ulck(_mutex);		// acquire the lock
  	_cv.wait(ulck, [this] {return !_queue.empty();});	// unlock the mutex if _queue is empty OR lock mutex and continue if queue has items
  	auto msg = std::move(_queue.front());     // peek at the front item
    _queue.pop_front();                       // pop item from front
    return msg;
}

/*
why _queue.clear()
At peripheral intersections, Traffic is less. As the number of changes in a traffic lights at these intersections
is more compared to the number of vehicles approaching, there will be the accumulation of Traffic light messages in _queue.

By the time a new vehicle approaches at any of these peripheral intersections, it would be receiving out 
some older traffic light msg. 
That's the reason it seems vehicles are crossing red at these intersections, 
in fact, they are crossing based on some previous green signal in that _queue.

But once the new traffic light msg arrives, all the older msgs are redundant. 
So we have to clear _queue at send (as soon as new msg arrives).

For central intersection, _queue clear/ no clear won't be a problem, 
as traffic is huge there it will have enough receives to keep it empty for new msg.
*/
template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
  	std::lock_guard<std::mutex> lck(_mutex);	// acquire the lock
 	// do work
    _queue.clear();
  	_queue.emplace_back(msg);   // using emplace_back(msg) equivalent to push_back(std::move(msg))
  	_cv.notify_one();	// notify a single thread that an item in queue is now available
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (_queue.receive() == TrafficLightPhase::green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
	threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
  	
  	//std::chrono::time_point<std::chrono::system_clock> startPoint = std::chrono::system_clock::now();
    auto tp1 = std::chrono::high_resolution_clock::now();
    int durationThreshold = 4 + (rand() % 3); 
    while (true){
      	std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto tp2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(tp2 - tp1).count();
        if (duration >= durationThreshold){
            // toggle _currentPhase 
            _currentPhase = (_currentPhase == TrafficLightPhase::red) ? TrafficLightPhase::green : TrafficLightPhase::red;
          	// update _currentPhase in dequeue
          	_queue.send(std::move(_currentPhase));
            tp1 = std::chrono::high_resolution_clock::now();        // update time point
        }
    }
}