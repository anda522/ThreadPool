#include<iostream>
#include<queue>
#include<shared_mutex>
#include<functional>
#include<vector>
#include<thread>
#include<condition_variable>
#include<future>
#include<mutex>
#include<time.h>
#include<chrono>

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
	// 内置线程工作类
	class worker {
	public:
		ThreadPool *pool;
		// worker的构造函数
		worker(ThreadPool* _pool): pool(_pool) {};
		// 重载()
		void operator ()() {
			bool flag;
			std::function<void()> func;
			// 只要线程池没关，就一直拿出任务队列的任务执行线程任务
			while(!pool->is_shutdown) { 
				{
					// wait需要和unique_lock配套使用
					std::unique_lock<std::mutex> lock(pool->_m);
					// 条件为false: 解开互斥锁，阻塞线程，等待条件变量通知
					// 条件为true : 取消阻塞，当前线程继续工作
					pool->cv.wait(lock, [this]() {
						return this->pool->is_shutdown || 
							!this->pool->q.empty();
					});
					flag = pool->q.pop(func);
				}
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

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

	// 任务提交
	template<typename F, typename... Args>
	auto submit(F&& f, Args&& ...args) -> std::future<decltype(f(args...))> {
		// func为左值
		std::function<decltype(f(args...))()> func = [&f, args...]() {
			return f(args...);
		};
		// 使用packaged_task获取函数签名，链接func和future，使之能够进行异步操作
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
		// 包装可进入线程的线程函数
		std::function<void()> wrapper_func = [task_ptr]() {
			(*task_ptr)();
		};
		// 将包装的函数加入队列
		q.push(wrapper_func);
		// 唤醒一个线程
		cv.notify_one();
		// 返回前面注册的任务指针
		return task_ptr->get_future();
	}

	// 析构函数
	~ThreadPool() {
		// 添加一个空任务，必须获取空任务的返回值才能结束,这样所有的任务都能够得到执行
		auto f = submit([]() {});
		f.get();
		// 线程池关闭
		{
			std::unique_lock<std::mutex> lock(_m);
			is_shutdown = true;
		}
		// 唤醒所有阻塞的线程(在线程池中阻塞的线程)
		cv.notify_all();
		for(auto& t : threads) {
			// 判断此线程是否可以
			if(t.joinable()) {
				t.join(); // 线程未运行完毕，阻塞进程继续运行线程，直到运行结束
			}
		}
	}

};

std::mutex _m;
int main() {
	ThreadPool pool(8);
	int n = 30;

	for(int i = 1; i <= n; i++) {
		pool.submit([](int id) {
			if(id % 2 == 1) {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			// 对输出cout加锁
			std::unique_lock<std::mutex> lock(_m);
			std::cout << "id : " << id << "\n";
		}, i);
	}
	return 0;
}