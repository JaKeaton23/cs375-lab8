// fcfs.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process {
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time = 0; int waiting_time = 0; int turnaround_time = 0;
};

void printGantt(const vector<pair<string,int>>& g){
    cout << "Gantt: ";
    for (auto& e: g) cout << e.first << "(" << e.second << ") ";
    cout << "\n";
}
void calculateAndPrintMetrics(vector<Process>& ps, int total_time, const vector<pair<string,int>>& gantt){
    double avg_w=0, avg_t=0;
    for (auto& p: ps){ avg_w += p.waiting_time; avg_t += p.turnaround_time; }
    avg_w/=ps.size(); avg_t/=ps.size();
    double cpu_util = (double)accumulate(ps.begin(), ps.end(), 0,
        [](int s,const Process& p){return s+p.burst_time;}) / max(1,total_time) * 100.0;
    double throughput = (double)ps.size()/max(1,total_time);
    printGantt(gantt);
    cout << "Average Waiting Time: "   << avg_w << "\n";
    cout << "Average Turnaround Time: " << avg_t << "\n";
    cout << "CPU Utilization: " << cpu_util << "%\n";
    cout << "Throughput: " << throughput << " processes/unit time\n";
}

int main(){
    vector<Process> p = {
        {"P1",0,8,2}, {"P2",1,4,1}, {"P3",2,9,3}, {"P4",3,5,4}
    };
    sort(p.begin(), p.end(), [](auto&a,auto&b){return a.arrival_time<b.arrival_time;});

    vector<pair<string,int>> gantt;
    int time=0;
    for (auto& x: p){
        if (x.arrival_time>time) { // idle gap
            gantt.push_back({"IDLE", x.arrival_time-time});
            time = x.arrival_time;
        }
        gantt.push_back({x.id, x.burst_time});
        time += x.burst_time;
        x.turnaround_time = time - x.arrival_time;
        x.waiting_time    = x.turnaround_time - x.burst_time;
    }
    calculateAndPrintMetrics(p, time, gantt);
}
