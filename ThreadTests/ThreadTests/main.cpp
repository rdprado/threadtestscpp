// Example program
#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>

// Producer-consumer

std::queue<std::string> queue;
std::queue<int> queueInt;
int consumerLimit = 1000000;
std::atomic<int> consumedCount;
std::mutex queueLock;

std::atomic<bool> spinLock = false;
// ---------------------------------------------------------
// Example 1: Producer and Consumer each occupying a core
// In a 12 core computer we should see a 16% cpu consumption
// by this program.
// Main problems (bugs and performance): 
// 1*- If producer takes more time to produce than
// consumer to consume, the consumer thread would be wasting
// an entire cpu without need
// If the consumer is slower than producer, producer could
// increase the size of queue. If the queue is limited
// producer would be wasting resources.
// 2- When the consumer gets the front element and then pops it
// we could be using differente elements.
// QUEUE is
// 0 1 2 3 4
// front = 4
// Producer pushes 5
// Consumer pops 5, not 4
// Queue still is 0 1 2 3 4
// Next consumer iteration 4 is printed again. So no surprises
// if list numbers are printed twice
// 3- Push and pop could be performed at the same time and
// they are not guaranteed to be atomic
// ---------------------------------------------------------

void ProducerMaxCPU(int consumerLimit)
{
    consumerLimit = 1000;
    int counter = 0;
    while (consumedCount < consumerLimit)
    {
        // Uncomment to see consumer wasting resources
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queueInt.push(counter++);
    }
}

void ConsumerMaxCPU(int consumerLimit)
{
    consumedCount = 0;
    while (consumedCount++)
    {
        if (!queueInt.empty())
        {
            auto front = queueInt.front();
            std::cout << front << std::endl;
            queueInt.pop();
        }
        else
        {
            std::cout << "consumer wasting resources" << std::endl;
        }
    }
}

// ---------------------------------------------------------
// Example 2: Producer and consumer with simple queue lock
// to achieve mutual exclusion. Treate queue as the critical
// region.
// Now we won't see more bugs and will se the consumer
// removing elements in the exact order that they were pushed
// Main problems: the mais problems with this approach
// is when there are unbalances between threads, and usually
// there will be in some time intervals, and we would have
// the consumer thread wasting resources having to lock
// and check the queue every iteration even when the queue is empty.
// The other way around could also be true if the queue has a limit
// and the consumer is slower than producer and producer would
// have to wait until the queue has space, wasting resources
// ---------------------------------------------------------

void Producer_SimpleLock(int consumerLimit, bool wait)
{
    int counter = 0;
    //consumerLimit = 1000;


    while (consumedCount < consumerLimit)
    {
        if (wait)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // sleep for random number to make the producer slower
        }
        std::lock_guard<std::mutex> lockguard(queueLock);
        queue.push("test" + std::to_string(counter++));
    }
}

void Consumer_SimpleLock(int consumerLimit)
{
    consumedCount = 0;
    while (consumedCount < consumerLimit)
    {
        std::lock_guard<std::mutex> lockguard(queueLock);
        if (!queue.empty())
        {
            auto front = queue.front();
            //std::cout << front << std::endl;
            queue.pop();
            consumedCount++;
        }
    }
}

// ---------------------------------------------------------
// Example 3: Spin lock approach: a busy wait.
// For instance, when the producer is slower than consumer
// and consumer would have to check every time for an
// empty queue and do nothing. Use sthe spin lock and make
// the producer release it.
// ---------------------------------------------------------

void Producer_SpinLock(int consumerLimit, bool wait)
{
    srand(time(0));  // Initialize random number generator.
    int counter = 0;
    while (consumedCount < consumerLimit)
    {
        if (wait)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // sleep for random number to make the producer slower
        }
        std::lock_guard<std::mutex> lockguard(queueLock);
        queue.push("test" + std::to_string(counter++));
        spinLock = false; // consumer can consume
    }
}

void Consumer_SpinLock(int consumerLimit)
{
    consumedCount = 0;
    while (consumedCount < consumerLimit)
    {
        while (spinLock)
        {
            // busy wait
            //std::cout << "consumer waiting" << std::endl;
        }

        std::lock_guard<std::mutex> lockguard(queueLock);
        if (!queue.empty())
        {
            auto front = queue.front();
            queue.pop();
            consumedCount++;
        }
        else
        {
            spinLock = true;
            continue;
        }
    }
}

int main()
{
    std::cout << "Producer, consumer" << std::endl;

    auto t1 = std::chrono::high_resolution_clock::now();

    // --------------- SimpleLock vs SPinLock with slow producer ------------------//
    /*std::thread tProducer(Producer_SimpleLock, consumerLimit * 0.001, true);
    std::thread tConsumer(Consumer_SimpleLock, consumerLimit * 0.001);
    
    std::thread tProducer(Producer_SpinLock, consumerLimit * 0.001, false);
    std::thread tConsumer(Consumer_SpinLock, consumerLimit * 0.001); */
    // ----------------------------------------------------------------------------//

    // --------------- SimpleLock vs SPinLock well balanced  ------------------//
    std::thread tProducer(Producer_SimpleLock, consumerLimit, false);
    std::thread tConsumer(Consumer_SimpleLock, consumerLimit);

    /*std::thread tProducer(Producer_SpinLock, consumerLimit, false);
    std::thread tConsumer(Consumer_SpinLock, consumerLimit);*/
    // ----------------------------------------------------------------------------//

    tProducer.join();
    tConsumer.join();

    auto t2 = std::chrono::high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << ms_int.count() << "ms\n";
}