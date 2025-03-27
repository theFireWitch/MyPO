#include <iostream>
#include <queue>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <functional>
#include <random>
#include <chrono>
#include <numeric>
using namespace std;

unsigned seed = chrono::system_clock::now().time_since_epoch().count();
using read_write_lock = shared_mutex;
using read_lock = shared_lock<read_write_lock>;
using write_lock = unique_lock<read_write_lock>;

void rand_array_gen(vector<int>& array) {
	iota(array.begin(), array.end(), 0);
	shuffle(array.begin(), array.end(), default_random_engine(seed));
}

template <typename task_type_t>
class task_queue
{
	using task_queue_implementation = queue<task_type_t>;
public:
	inline task_queue() = default;
	inline ~task_queue() { clear(); }
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
	bool pop(task_type_t& task){
		write_lock _(m_rw_lock);
		vector<int> indices(buffer_count);
		rand_array_gen(indices);
		for (int idx : indices) {
			if (!buffer[idx].empty()) {
				task = move(buffer[idx].front());
				buffer[idx].pop();
				return true;
			}
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
			if (buffer[idx].size() < 10) {
				buffer[idx].emplace(forward<arguments>(parameters)...);
				return;
			}
		}
		cout << "All buffers in queue are full, can`t add current task!" << endl;
	}
private:
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
	void initialize(const size_t worker_count){
		write_lock _(m_rw_lock);
		if (m_initialized || m_terminated){
			return;
		}
		m_workers.reserve(worker_count);
		for (size_t id = 0; id < worker_count; id++){
			//m_workers.emplace_back(&routine, this);
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
	void routine(){
		while (true){
			bool task_accquiered = false;
			function<void()> task;{
				write_lock _(m_rw_lock);
				auto wait_condition = [this, &task_accquiered, &task] {
					task_accquiered = m_tasks.pop(task);
					return m_terminated || task_accquiered;
					};
				m_task_waiter.wait(_, wait_condition);
			}
			if (m_terminated && !task_accquiered){
				return;
			}
			task();
		}
	}
	bool working() const{
		read_lock _(m_rw_lock);
		return working_unsafe();
	}
	bool working_unsafe() const{
		return m_initialized && !m_terminated;
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
	task_queue<function<void()>> m_tasks;
	bool m_initialized = false;
	bool m_terminated = false;
};

int main()
{

	return 0;
}