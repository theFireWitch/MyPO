#include <iostream>
#include <queue>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <functional>
#include <random>
#include <chrono>
#include <numeric>
#include <algorithm>

#define MIN_TIME 6
#define MAX_TIME 12
#define MAX_QUEUE_SIZE 10
#define BUFFER_COUNT 3

using namespace std;
unsigned seed = chrono::system_clock::now().time_since_epoch().count();
using read_write_lock = shared_mutex;
using read_lock = shared_lock<read_write_lock>;
using write_lock = unique_lock<read_write_lock>;

atomic<int> index_global(0);
atomic<int> tasks_created_count(0);
atomic<int> tasks_rejected_count(0);
vector<atomic<int>> tasks_in_buffer(BUFFER_COUNT);
chrono::high_resolution_clock::time_point time_queue_full_start;
chrono::high_resolution_clock::time_point time_queue_full_end;
vector<double> time_queue_full;
mutex text_mutex;
atomic<bool> stop_signal(false);

struct Task {
	int task_id;
	int random_delay;
	Task() {}
	Task(int id, int delay) {
		task_id = id;
		random_delay = delay;
	}
};

void do_task(Task task);

template <typename task_type_t>
class task_queue
{
	using task_queue_implementation = queue<task_type_t>;
public:
	inline task_queue() = default;
	inline ~task_queue() { clear(); }
	inline task_queue(const int queue_count) {
		buffer_count = queue_count;
	}
	bool empty() const{
		read_lock _(m_rw_lock);
		bool result = true;
		for (int idx = 0; idx < buffer_count; idx++)
			result = result && buffer[idx].empty();
		return result;
	}
	size_t size() const{
		read_lock _(m_rw_lock);
		int result = 0;
		for (int idx = 0; idx < buffer_count; idx++)
			result += buffer[idx].size();
		return result;
	}
	void clear(){
		write_lock _(m_rw_lock);
		for (int idx = 0; idx < buffer_count; idx++){
			while (!buffer[idx].empty()) {
				buffer[idx].pop();
			}
		}
	}
	bool pop(task_type_t& task, int buffer_id){
		write_lock _(m_rw_lock);
		if (!buffer[buffer_id].empty()) {
			task = move(buffer[buffer_id].front());
			buffer[buffer_id].pop();
			if (queue_full) {
				time_queue_full_end = chrono::high_resolution_clock::now();
				double duration = chrono::duration_cast<chrono::nanoseconds>(time_queue_full_end - time_queue_full_start).count() * 1e-9;
				time_queue_full.emplace_back(duration);
				queue_full = false;
			}
			return true;
		}
		return false;
	}
	template <typename... arguments>
	void emplace(arguments&&... parameters){
		write_lock _(m_rw_lock);
		vector<int> indices(buffer_count);
		iota(indices.begin(), indices.end(), 0);
		shuffle(indices.begin(), indices.end(), default_random_engine(chrono::system_clock::now().time_since_epoch().count()));
		for (int idx : indices) {
			if (buffer[idx].size() < max_queue_size) {
				buffer[idx].emplace(forward<arguments>(parameters)...);
				tasks_in_buffer[idx].fetch_add(1);
				text_mutex.lock();
				cout << "Queue " << idx << " has " << buffer[idx].size() << " elements" << endl;
				text_mutex.unlock();
				return;
			}
		}
		queue_full = true;
		text_mutex.lock();
		cout << "All buffers in queue are full, can`t add current task!" << endl;
		text_mutex.unlock();
		tasks_rejected_count.fetch_add(1);
		time_queue_full_start = chrono::high_resolution_clock::now();
	}
	int get_queue_count() {
		return buffer_count;
	}
private:
	const int max_queue_size = MAX_QUEUE_SIZE;
	const int buffer_count = BUFFER_COUNT;
	bool queue_full = false;
	mutable read_write_lock m_rw_lock;
	task_queue_implementation* buffer = new task_queue_implementation[buffer_count];
};

class thread_pool
{
public:
	inline thread_pool() = default;
	inline ~thread_pool() { terminate(); }
public:
	void initialize(const size_t workers_per_queue){

		write_lock _(m_rw_lock);
		if (m_initialized || m_terminated){
			return;
		}
		int queue_count = m_tasks.get_queue_count();
		m_workers.reserve(queue_count * workers_per_queue);
		for (size_t id = 0; id < queue_count; id++){
			for (size_t j = 0; j < workers_per_queue; ++j) {
				m_workers.emplace_back(&thread_pool::routine, this, id);
			}
		}
		m_initialized = !m_workers.empty();
	}
	void terminate(){{
			write_lock _(m_rw_lock);
			if (working_unsafe()){
				m_terminated = true;
			}
			else{
				m_workers.clear();
				m_terminated = false;
				m_initialized = false;
				return;
			}
		}
		m_task_waiter.notify_all();
		for (thread& worker : m_workers){
			worker.join();
		}
		m_workers.clear();
		m_terminated = false;
		m_initialized = false;
	}
	void routine(const int queue_id){
		while (true){
			bool task_accquiered = false;
			Task task;
			{
				write_lock _(m_rw_lock);
				auto wait_condition = [this, &task_accquiered, &task, queue_id] {
					task_accquiered = m_tasks.pop(task, queue_id);
					return (m_terminated || task_accquiered) && !m_paused;
					};
				m_task_waiter.wait(_, wait_condition);
			}
			if (m_terminated && !task_accquiered){
				return;
			}
			do_task(task);
		}
	}
	bool working() const{
		read_lock _(m_rw_lock);
		return working_unsafe();
	}
	bool working_unsafe() const{
		return m_initialized && !m_terminated;
	}
	void pause() {
		write_lock _(m_rw_lock);
		m_paused = true;
		text_mutex.lock();
		cout << "pause start" << endl;
		text_mutex.unlock();
	}
	void resume() {
		write_lock _(m_rw_lock);
		m_paused = false;
		text_mutex.lock();
		cout << "pause stop" << endl;
		text_mutex.unlock();
		m_task_waiter.notify_all();
	}
public:
	template <typename Task, typename... arguments>
	void add_task(Task&& task, arguments&&... parameters){{
			read_lock _(m_rw_lock);
			if (!working_unsafe() || m_paused) {
				text_mutex.lock();
				cout << ": rejected" << endl;
				text_mutex.unlock();
				return;
			}
		}
		m_tasks.emplace(task);
		m_task_waiter.notify_one();
	}
private:
	mutable read_write_lock m_rw_lock;
	mutable condition_variable_any m_task_waiter;
	vector<thread> m_workers;
	task_queue<Task> m_tasks;
	bool m_initialized = false;
	bool m_terminated = false;
	bool m_paused = false;
};

void do_task(Task task) {
	text_mutex.lock();
	cout << "Task " << task.task_id << " started: time to sleep - " << task.random_delay << endl;
	text_mutex.unlock();

	this_thread::sleep_for(chrono::seconds(task.random_delay));

	text_mutex.lock();
	cout << "Task " << task.task_id << " completed" << endl;
	text_mutex.unlock();
}

void generate_func(thread_pool& pool, int gen_id) {
	srand(chrono::system_clock::now().time_since_epoch().count());
	while (!stop_signal.load()) {
		int New_id = index_global.fetch_add(1);
		int random_delay = rand() % (MAX_TIME - MIN_TIME + 1) + MIN_TIME;
		Task task(New_id, random_delay);

		text_mutex.lock();
		cout << "Generator " << gen_id << " tries to add task " << task.task_id << endl;
		text_mutex.unlock();

		tasks_created_count.fetch_add(1);
		pool.add_task(task);

		int delay = (rand() % 3 + 1) * 500;  
		text_mutex.lock();
		cout << "Generator " << gen_id << " sleeps for " << (float)delay / 1000 << "sec." << endl;
		text_mutex.unlock();
		this_thread::sleep_for(chrono::milliseconds(delay));
	}
}

void add_pause(thread_pool& pool) {
	this_thread::sleep_for(chrono::seconds(10));
	pool.pause();
	this_thread::sleep_for(chrono::seconds(10));
	pool.resume();
	this_thread::sleep_for(chrono::seconds(10));
}

int main()
{
	srand(chrono::system_clock::now().time_since_epoch().count());
	int workers_per_queue = 2;
	int generators_count = 2;
	thread_pool pool;
	pool.initialize(workers_per_queue);

	vector<thread> generators;
	for (size_t i = 0; i < generators_count; ++i) {
		generators.emplace_back(generate_func, ref(pool), i);
	}
	
	this_thread::sleep_for(chrono::seconds(60));
	stop_signal.store(true);
	pool.terminate();

	for (auto& generator : generators) {
		if (generator.joinable()) {
			generator.join();
		}
	}
	if (!time_queue_full.empty()) {
		cout << endl << endl << "Total time queue was full: " << accumulate(time_queue_full.begin(), time_queue_full.end(), 0.0) << " seconds" << endl;
		cout << "Min time queue was full: " << *min_element(time_queue_full.begin(), time_queue_full.end()) << " seconds" << endl;
		cout << "Max time queue was full: " << *max_element(time_queue_full.begin(), time_queue_full.end()) << " seconds" << endl;
	}
	else {
		cout << endl << endl << "Total time queue was full: 0 seconds" << endl;
	}
	for (int idx = 0; idx < BUFFER_COUNT; idx++) {
		cout << "Tasks in buffer " << idx + 1 << ": " << tasks_in_buffer[idx] << endl;
	}
	cout << "Tasks created: " << tasks_created_count.load() << endl;
	cout << "Tasks rejected: " << tasks_rejected_count.load() << endl;
	return 0; 
}

