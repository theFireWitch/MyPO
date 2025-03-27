#include <iostream>
#include <queue>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <functional>
#include <random>
#include <chrono>
#include <numeric>

#define MIN_TIME 6
#define MAX_TIME 12

using namespace std;
unsigned seed = chrono::system_clock::now().time_since_epoch().count();
using read_write_lock = shared_mutex;
using read_lock = shared_lock<read_write_lock>;
using write_lock = unique_lock<read_write_lock>;

atomic<int> index_global(0);
mutex text_mutex;

struct Task {
	int task_id;
	int random_delay;
	function<void()> task;
	Task() {
		task_id = index_global.fetch_add(1);
		random_delay = rand() % (MAX_TIME - MIN_TIME + 1) + MIN_TIME;
	}
};

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
//public:
	//task_queue(const task_queue& other) = delete;
	//task_queue(task_queue&& other) = delete;
	//task_queue& operator=(const task_queue& rhs) = delete;
	//task_queue& operator=(task_queue&& rhs) = delete;
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
			return true;
		}
		return false;
	}
	template <typename... arguments>
	void emplace(arguments&&... parameters){
		write_lock _(m_rw_lock);
		vector<int> indices(buffer_count);
		iota(indices.begin(), indices.end(), 0);
		shuffle(indices.begin(), indices.end(), default_random_engine(seed));
		for (int idx : indices) {
			if (buffer[idx].size() < max_queue_size) {
				buffer[idx].emplace(forward<arguments>(parameters)...);
				return;
			}
		}
		cout << "All buffers in queue are full, can`t add current task!" << endl;
	}
	int get_queue_count() {
		return buffer_count;
	}
	int get_queue_max_size() {
		return max_queue_size;
	}
private:
	const int max_queue_size = 10;
	const int buffer_count = 3;
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
	void routine(const int queue_id){ //TODO: add pause
		while (true){
			bool task_accquiered = false;
			Task task;
			{
				write_lock _(m_rw_lock);
				auto wait_condition = [this, &task_accquiered, &task, queue_id] {
					task_accquiered = m_tasks.pop(task, queue_id);
					return m_terminated || task_accquiered;
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
	void pause() { // TODO: realization 
		write_lock _(m_rw_lock);
		m_paused = true;
	}
	void resume() {
		write_lock _(m_rw_lock);
		m_paused = false;
		m_task_waiter.notify_all();
	}
public:
	template <typename task_t, typename... arguments>
	void add_task(task_t&& task, arguments&&... parameters){{
			read_lock _(m_rw_lock);
			if (!working_unsafe()) {
				return;
			}
		}
		auto bind = bind(forward<task_t>(task), forward<arguments>(parameters)...);
		m_tasks.emplace(bind);
		m_task_waiter.notify_one();
	}
//public:
//	thread_pool(const thread_pool& other) = delete;
//	thread_pool(thread_pool&& other) = delete;
//	thread_pool& operator=(const thread_pool& rhs) = delete;
//	thread_pool& operator=(thread_pool&& rhs) = delete;
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

int main()
{
	srand(seed);
	int workers_per_queue = 2;
	thread_pool pool;
	pool.initialize(workers_per_queue);

	return 0;
}