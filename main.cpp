// var 16 Lab6 Andrii Itsko K-26
#include <iostream>
#include <coroutine>
#include <stdexcept>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <format>

using namespace std;

struct get_input{};

class average_numb{
public:
    struct promise_type{
        double curr_average = 0.0;
        double input_value = 0.0;

        auto get_return_object() {return average_numb{coroutine_handle<promise_type>::from_promise(*this)}; }
        auto initial_suspend() {return suspend_never{};}
        auto final_suspend() noexcept {return std::suspend_always{};}
        void return_void(){}
        void unhandled_exception() {terminate();}

        auto yield_value(double avg){
            this->curr_average = avg;
            return suspend_always{};
        }

        struct input_awaiter{
            promise_type &p;
            bool await_ready() const noexcept {return true;}
            void await_suspend(coroutine_handle<>) const noexcept{}
            double await_resume() const noexcept{
                return p.input_value;
            }
        };
        auto await_transform(get_input){
            return input_awaiter{*this};
        }
    };

    bool send_value(double val){
        if(!coro || coro.done()){
            return false;
        }
        coro.promise().input_value = val;
        coro.resume();
        return !coro.done();
    }
    double get_average() const {
        if(!coro){
            throw runtime_error("null");
        }
        return coro.promise().curr_average;
    }

    average_numb(average_numb &&other) noexcept : coro(other.coro){
        other.coro = nullptr;
    }
    average_numb(const average_numb&) = delete;
    average_numb &operator=(const average_numb&) = delete;

    ~average_numb(){
        if(coro){
            coro.destroy();
        }
    }
private:
    average_numb(coroutine_handle<promise_type> h) : coro(h){}
    coroutine_handle<promise_type> coro;
};

average_numb var16(){
    vector<double> buffer;
    double curr_avg = 0.0;
    bool is_paused_active = false;
    chrono::steady_clock::time_point pause_start_time;
    co_yield 0.0;

    while(true){
        double val = co_await get_input{};

        if(is_paused_active){
            auto now = chrono::steady_clock::now();
            auto duration_since_zero = now - pause_start_time;

            if(duration_since_zero < chrono::seconds(1)){
                co_yield curr_avg;
                continue;
            }
            else{
                is_paused_active = false;
            }
        }

        if(val == 0.0){
            is_paused_active = true;
            pause_start_time = chrono::steady_clock::now();
            co_yield curr_avg;
            continue;
        }

        buffer.push_back(val);
        if(buffer.size() > 3){
            buffer.erase(buffer.begin());
        }
        if(!buffer.empty()){
            double sum = 0.0;
            for(double numb: buffer){
                sum += numb;
            }

            curr_avg = sum / buffer.size();
        }
        else{
            curr_avg = 0.0;
        }

        co_yield curr_avg;
    }
}


int main(){
    cout << "Start..." << endl;
    cout << "Enter numbers. \nIf you want to exit, press q" << endl;

    average_numb coroutine = var16();
    double val;
    while(true){
        cout << "enter number: ";
        if(!(cin >> val)){
            break;
        }

        coroutine.send_value(val);
        double avg = coroutine.get_average();

        cout << format("Current average of last 3 naumbers: {:.4f}", avg) << endl;
    }

    cout << "\nEnd...";
    return 0;
}