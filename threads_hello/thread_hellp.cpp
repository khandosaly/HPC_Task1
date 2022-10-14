#include <thread>
#include <cstdio>
#include <string>
#include <vector>

using namespace std;

int thread_func(int id, int nt) {
    printf("Hello, I am %d from %d\n", id, nt);
    return 0;
}

//struct Func {
//    void operator()() {
//        printf("Hello, I am %d from %d\n", id, nt);
//    }
//    int id, nt;
//};

int main(int argc, char **argv) {

    int nt = (argc > 1 ? stoi(argv[1]) : 1);

    vector<thread> threads;

    for (int i = 0; i < nt; i++) {
        //thread thr(Func {i, nt});
        thread thr(&thread_func, i, nt);
        threads.push_back(move(thr));
    }
    //threads.emplace_back(Func {});
    //threads.emplace_back([nt](int v) { printf("Lambda, v=%d, nt=%d\n", v, nt); }, 42);

    for (auto &thread: threads) {
        thread.join();
    }

    return 0;
}