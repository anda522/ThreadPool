#include<iostream>
#include<queue>
#include<shared_mutex>
#include<functional>
#include<vector>
#include<thread>
#include<condition_variable>
#include<furture>

template<typename T>
struct SafeQueue {
	// 存储队列
	std::queue<T> q;
	// 队列内的共享锁，需要保证队列的互斥访问
	std::shared_mutex _m;
	
	// 判断队列是否为空
	bool empty() {
		// 添加共享锁:访问队列状态，属于读操作，可多个线程同时读
		std::shared_lock<std::shared_mutex> lock(_m);
		return q.empty();
	}

	// 获取队列的大小
	auto size() {
		// 添加共享锁:访问队列大小，属于读操作，可多个线程同时读
		std::shared_lock<std::shared_mutex> lock(_m);
		return q.size();
	}

	// 向队列中添加元素
	void push(T& ele) {
		// 添加独占锁:修改队列状态（大小），属于写操作，只能同步互斥访问
		std::unique_lock<std::shared_mutex> lock(_m);
		q.push(ele);
	}

	// 删除队列元素，并将删除的元素复制到ele上
	bool pop(T& ele) {
		// 添加独占锁:修改队列状态（大小），属于写操作，只能同步互斥访问
		std::unique_lock<std::shared_mutex> lock(_m);
		if(q.empty()) {
			return false;
		}
		// 将要删除的元素转换为右值（可能是避免多余拷贝），赋值给ele
		ele = move(q.front());
		q.pop();
		return true;
	}
};

// 线程池实现
class ThreadPool {
private:
	// worker用于线程的不断(故有while循环)执行，最多支持n个线程的执行
	class worker {
		ThreadPool *pool;
		// worker的构造函数
		worker(ThreadPool* _pool): pool(_pool) {};
		// 重载()
		void operator ()() {
			// 只要线程池没关，就一直询问且执行线程
			// 避免虚假唤醒，设置while循环
			while(!pool->is_shutdown) { 
				// 执行线程操作
				{
					// wait需要和unique_lock配套使用
					std::unique_lock<std::mutex> lock(pool->_m);
					// false: 解开互斥锁，线程挂起（阻塞）
					// true: 取消阻塞，当前线程继续工作
					pool->cv.wait(lock, [this]() {
						return this->pool->is_shutdown || 
							!this->pool->q.empty();
					});
				}
				function<void()> func;
				bool flag = pool->q.pop(func);
				if(flag) {
					// 执行线程函数
					func();
				}
			}
		}
	};

public:
	// 线程池是否关闭
	bool is_shutdown;
	// 安全队列,存储类型为函数
	SafeQueue<std::function<void()>> q;
	// 所有线程
	std::vector<std::thread> threads;
	// 互斥量
	std::mutex _m;
	// 条件变量
	std::condition_variable cv;

	// 构造函数
	ThreadPool(int n): threads(n), is_shutdown(false) {
		for(auto& t : threads) {
			// worker传入线程池的指针
			t = std::thread(worker(this));
		}
	}

	// 任务提交
	template<typename F, typename... Args>
	auto submit(F&& f, Args&& ...args) -> std::furture<decltype(f(args...))> {
		
		function<decltype(f(args...))> func = [&f, arg...]() {
			return f(args...);
		};

		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		std::function<void()> wrapper_func = [task_ptr]() {
			(*task_ptr)();
		};

		q.push(wrapper_func);
		cv.notify_one();
	}

	// 析构函数
	~ThreadPool() {
		auto f = submit([]() {});
		f.get();

		is_shutdown = true;
		// 唤醒所有线程
		cv.notify_all();
		for(auto& t : threads) {
			if(t.joinable()) {
				t.join();
			}
		}
	}

};

int main() {

	return 0;
}