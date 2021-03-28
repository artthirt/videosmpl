#ifndef THREAD_POOL
#define THREAD_POOL

#include <queue>
#include <vector>
#include <atomic>
#include <mutex>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <unistd.h>

class thread_pool{
public:
	enum {
		WAIT_INFINITY = 0
	};

	thread_pool(size_t pool_count){
		m_done = false;
		m_running = false;
		m_available = 0;
		for(size_t i = 0; i < pool_count; i++){
			m_thread_pool.push_back(boost::thread(boost::bind(&thread_pool::run, this)));
			m_available++;
		}

		//m_main_thread = boost::thread(boost::bind(&thread_pool::main_run, this));
	}
	~thread_pool(){
		close();
	}
	/**
	 * @brief empty
	 * @return
	 */
	bool empty() const{
		return m_tasks.size() == 0;
	}
	/**
	 * @brief is_free
	 * @return
	 */
	bool is_free() const{
		return m_tasks.size() == 0 && m_available == m_thread_pool.size();
	}
	/**
	 * @brief can
	 * @return
	 */
	bool can() const{
		return m_available > 0;
	}

	template< class T >
	class Functor{
	public:
		Functor(T* func): m_func(func){}
		inline void operator() (){
			(*m_func)();
		}

	private:
		T *m_func;
	};

	/**
	 * @brief push
	 * add task to queue
	 * @param func
	 */
	template< class task >
	void push(task& func){
		m_tasks.push(boost::function< void() >(Functor<task>(&func)));
		m_condition.notify_all();		/// send notification all threads that waiting
	}
	/**
	 * @brief join
	 */
	void join(){
		for(size_t i = 0; i < m_thread_pool.size(); i++){
			m_thread_pool[i].join();
		}
	}
	/**
	 * @brief close
	 */
	void close(){
		m_done = true;
		m_condition.notify_all();
		for(size_t i = 0; i < m_thread_pool.size(); i++){
			m_thread_pool[i].join();
		}
	}
	/**
	 * @brief wait_for_free
	 * @param usec
	 */
	void wait_for_free(useconds_t usec = WAIT_INFINITY, useconds_t tick = 100){
		if(usec == WAIT_INFINITY){
			while(m_tasks.size() > 0 && m_available != m_thread_pool.size() || !m_running){
				usleep(tick);
			}
		}else{
			useconds_t cur = 0;
			while((m_tasks.size() > 0 && m_available != m_thread_pool.size()
				   || !m_running) && cur < usec){
				usleep(tick);
				cur += tick;
			}
		}
	}
	/**
	 * @brief run
	 */
	void run(){

		if(!m_thread_pool.size())
			return;

		m_running = true;

		boost::mutex mutex;
		boost::unique_lock< boost::mutex > lock(mutex);
		while(!m_done){
			if(m_thread_pool.size() > 1)	/// the notification will sent before it waited
				m_condition.wait(lock);

			if(!m_running)
				break;

			m_mutex.lock();
			if(m_available > 0 && m_tasks.size() > 0){
				m_available--;
				boost::function< void() > func = m_tasks.front();
				m_tasks.pop();
				m_mutex.unlock();

				func();
				m_available++;
			}else
				m_mutex.unlock();

			if(m_tasks.size())
				m_condition.notify_all();
		}
	}

private:
	std::vector< boost::thread > m_thread_pool;			/// main pool of threads
	std::queue< boost::function< void() > > m_tasks;		/// queue with tasks
	std::atomic_bool m_done;								/// for closing
	boost::mutex m_mutex;							/// for add and remove task from queue
	std::atomic_int m_available;							/// available threads in pool
	std::atomic_bool m_running;							/// check running any threads ater create object and break if false
	boost::condition_variable m_condition;			/// condition for send notification
};

#endif // THREAD_POOL

