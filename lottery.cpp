// lottery.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process{
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0, waiting_time=0, turnaround_time=0;
};

static void printGantt(const vector<pair<string,int>>& g){
    cout<<"Gantt: "; for(auto&e:g) cout<<e.first<<"("<<e.second<<") "; cout<<"\n";
}
static void metrics(vector<Process>& ps, int total, const vector<pair<string,int>>& g){
    double aw=0,at=0; for(auto&p:ps){aw+=p.waiting_time; at+=p.turnaround_time;}
    aw/=ps.size(); at/=ps.size();
    int busy=accumulate(ps.begin(),ps.end(),0,[](int s,const Process&p){return s+p.burst_time;});
    cout<<fixed<<setprecision(2);
    printGantt(g);
    cout<<"Average Waiting Time: "<<aw<<"\n";
    cout<<"Average Turnaround Time: "<<at<<"\n";
    cout<<"CPU Utilization: "<<(total? 100.0*busy/total:0)<<"%\n";
    cout<<"Throughput: "<<(total? (double)ps.size()/total:0)<<" processes/unit time\n";
}

int main(){
    const int QUANTUM=4;
    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    sort(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    for(auto& p:ps) p.remaining_time=p.burst_time;

    int time=0, next=0, n=ps.size(), finished=0;
    vector<int> ready;               // indices
    vector<pair<string,int>> gantt;

    mt19937 gen((uint32_t)chrono::high_resolution_clock::now().time_since_epoch().count());

    auto arrive=[&](){
        while(next<n && ps[next].arrival_time<=time){ ready.push_back(next++); }
        // remove finished from ready if any (safety)
        ready.erase(remove_if(ready.begin(),ready.end(),[&](int i){return ps[i].remaining_time==0;}), ready.end());
    };
    auto ticketsOf=[&](int i){ return max(1, 10 / max(1, ps[i].priority)); }; // lower priority number => more tickets

    arrive();
    if(ready.empty() && next<n){ gantt.push_back({"IDLE", ps[next].arrival_time-time}); time=ps[next].arrival_time; arrive(); }

    while(finished<n){
        if(ready.empty()){
            if(next<n){ gantt.push_back({"IDLE", ps[next].arrival_time-time}); time=ps[next].arrival_time; arrive(); }
            continue;
        }
        // build lottery wheel
        int totalT=0; for(int i:ready) totalT += ticketsOf(i);
        uniform_int_distribution<int> dist(1,totalT);
        int pick=dist(gen);
        int chosen=-1, acc=0;
        for(int i:ready){ acc+=ticketsOf(i); if(pick<=acc){ chosen=i; break; } }

        int slice=min(QUANTUM, ps[chosen].remaining_time);
        gantt.push_back({ps[chosen].id, slice});
        time += slice; ps[chosen].remaining_time -= slice;
        arrive();
        if(ps[chosen].remaining_time==0){
            ps[chosen].turnaround_time = time - ps[chosen].arrival_time;
            ps[chosen].waiting_time = ps[chosen].turnaround_time - ps[chosen].burst_time;
            finished++;
        }
    }
    metrics(ps,time,gantt);
}
