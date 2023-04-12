#include "try.hpp"
#include <iostream>


static std::mutex mtx;

int fun1(int a, int b)
{
	std::unique_lock<std::mutex> u_lock(mtx);
	std::cout << "fun1: " << std::this_thread::get_id() << ": " << a + b << std::endl;
	return a + b;
}

class A
{
public:
	double operator()(double a, double b)
	{
		std::unique_lock<std::mutex> u_lock(mtx);
		std::cout << "fun2: " << std::this_thread::get_id() << ": " << a + b << std::endl;
		return a + b;
	}

	static std::string fun3(std::string a, std::string b)
	{
		std::unique_lock<std::mutex> u_lock(mtx);
		std::cout << "fun3: " << std::this_thread::get_id() << ": " << a + b << std::endl;
		return a + b;
	}
};

int main()
{
	ThreadPool* pt = ThreadPool::GetInstance();

	std::future<int> ft1 = pt->AddTask(fun1, 1, 2);
	std::future<double> ft2 = pt->AddTask(A(), 1.1, 2.2);
	std::future<std::string> ft3 = pt->AddTask(&A::fun3, "1", "2");
	std::future<void> ft4 = pt->AddTask([] {
		std::unique_lock<std::mutex> u_lock(mtx);
		std::cout << "fun4: " << std::this_thread::get_id() << std::endl;
		});

	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << ft1.get() << std::endl;
	std::cout << ft2.get() << std::endl;
	std::cout << ft3.get() << std::endl;
	ft4.get();

	return 0;
}