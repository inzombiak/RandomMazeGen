#ifndef I_THREADED_SOLVER_H
#define I_THREADED_SOLVER_H

#include <future>
#include <random>  
#include <atomic>
#include <condition_variable>

class IThreadedSolver
{
public:
	IThreadedSolver() : m_generate()
	{
		m_generate.clear();
	}
	virtual ~IThreadedSolver() {
		TerminateGeneration();
	};

	void TerminateGeneration()
	{
		m_generate.clear();
		std::unique_lock<std::mutex> lock(m_tsDoneCVMutex);
		
		auto not_paused = [this](){return m_done == true; };
		m_doneCV.wait(lock, not_paused);
	}

protected:
	IThreadedSolver::IThreadedSolver(const IThreadedSolver &){}
	bool CanGenerate()
	{
		std::unique_lock<std::mutex> lock(m_generateMutex);
		return m_generate.test_and_set();
	}

	void ClearGenerate()
	{
		std::unique_lock<std::mutex> lock(m_generateMutex);
		m_generate.clear();
	}

	void SetDoneState(bool val)
	{
		//std::unique_lock<std::mutex> lock(m_tsDoneCVMutex);
		m_done = val;
		m_doneCV.notify_all();
	}

private:

	std::mutex m_generateMutex;
	std::atomic_flag m_generate;
	std::condition_variable m_doneCV;
	std::mutex m_tsDoneCVMutex;
	std::atomic<bool> m_done = true;
};

#endif