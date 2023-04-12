#pragma once
#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <queue>

using Task = std::function<void()>;
#define THREAD_NUM 10

class ThreadPool
{
private:
    std::vector<std::thread> workers; //�̳߳�
    std::queue<Task> taskQueue_; //�������
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> run_; //�̳߳�����Ϊtrue����֮Ϊfalse

    static ThreadPool* ptp_;

    ThreadPool(int num = THREAD_NUM) : run_(true)
    {
        //���캯����ֱ������num���߳�
        for (int i = 0; i < num; i++) {
            workers.emplace_back(std::thread([this] {
                //�߳�ѭ��ȥ�������ȡ����ִ�У�û��������ȴ��������������ȥ
                while (run_) {
                    Task task;

                    {
                        std::unique_lock<std::mutex> u_mtx(mtx_); //ȡ����������
                        cv_.wait(u_mtx, [this] {
                            //ֻ�е��������Ϊ�ղŵȴ������̳߳�����ֹͣ����ֹͣ�ȴ�
                            //�������ڷ�ֹ�̳߳������������߳��ڵȴ�
                            return !run_ || !taskQueue_.empty();
                            });
                        if (!run_ && taskQueue_.empty()) {
                            //ֹͣ�����ұ�֤�������꣬��ֱ�ӷ���
                            return;
                        }

                        task = move(taskQueue_.front());
                        taskQueue_.pop();
                    }

                    //ִ������
                    task();
                }
                }));
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

public:
    static ThreadPool* GetInstance(int num = THREAD_NUM)
    {
        static std::mutex mtx;
        if (ptp_ == nullptr) {
            {
                std::unique_lock<std::mutex> u_mtx(mtx);
                if (ptp_ == nullptr) {
                    ptp_ = new ThreadPool(THREAD_NUM);
                }
            }
        }
        return ptp_;
    }

    static void DelInstance()
    {
        static std::mutex mtx;
        if (ptp_ != nullptr) {
            {
                std::unique_lock<std::mutex> u_mtx(mtx);
                if (ptp_ != nullptr) {
                    delete ptp_;
                }
            }
        }
    }

    ~ThreadPool()
    {
        run_ = false;
        cv_.notify_all();
        for (auto& t : workers) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    template<class F, class ... Args>
    auto AddTask(F&& f, Args&&... args) ->std::future<decltype(f(args...))>
    {
        using RetType = decltype(f(args...)); //f��������ֵ
        auto ptask = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...)); //task��һ������ָ��
        std::future<RetType> ft = ptask->get_future();

        {
            //�����������
            std::unique_lock<std::mutex> u_lock(mtx_);
            taskQueue_.emplace([ptask]() {
                (*ptask)();
                });
        }

        cv_.notify_one(); //����һ���߳�

        return ft;
    }
};