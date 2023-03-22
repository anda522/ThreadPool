#include<iostream>
#include<queue>
#include<shared_mutex>

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

int main() {

	return 0;
}