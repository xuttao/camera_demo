#include "semaphore.h"
#include <chrono>
namespace std {

    Semaphore::Semaphore(int32_t _count) :m_count(_count) {

    }

    Semaphore::~Semaphore() {

    }

    void Semaphore::wait() {
        std::unique_lock<std::mutex> locker(mutex_lock);
        --m_count;
        while (m_count < 0) {
            condition_var.wait(locker);
        }
    }

    bool Semaphore::wait(int _msec){
        std::unique_lock<std::mutex> locker(mutex_lock);
        --m_count;
        //�����ж���ٻ��ѵ�����������������������ȴ�ֱ����ʱ
        bool res = condition_var.wait_for(locker, std::chrono::duration<int, std::milli>(_msec), [&] {return m_count >= 0; });
        if (!res) {
            ++m_count;
        }
        return res;
        //auto ret = std::cv_status::no_timeout;
        //while (m_count < 0 && ret == std::cv_status::no_timeout)
        //{
        //    ret = condition_var.wait_for(locker, std::chrono::duration<int, std::milli>(_msec));
        //}
        //if (std::cv_status::no_timeout == ret)
        //{
        //    return true;
        //}
        //++m_count;
        //return false;
    }

    void Semaphore::post(int32_t _count) {
        std::lock_guard<std::mutex> locker(mutex_lock);
        m_count += _count;

        if (m_count <= 0)
            condition_var.notify_one();
    }

    void Semaphore::init(int32_t _count) {
        std::lock_guard<std::mutex> locker(mutex_lock);
        m_count = _count;
    }

    int32_t Semaphore::count() {
        std::lock_guard<std::mutex> locker(mutex_lock);
        return m_count;
    }

}