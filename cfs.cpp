// cfs.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process{
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0, waiting_time=0, turnaround_time=0;
    double vruntime=0.0;
};

struct Cmp {
    bool operator()(int a, int b) const { return false; } // dummy (unused)
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
    const int BASE_SLICE = 4; // target latency-ish
    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    sort(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    for(auto& p:ps){ p.remaining_time=p.burst_time; p.vruntime=0; }

    // We'll manage a vector of ready indices, choose min vruntime among arrived.
    int time=0, next=0, n=ps.size(), finished=0;
    vector<int> ready;
    vector<pair<string,int>> gantt;

    auto arrive=[&](){
        while(next<n && ps[next].arrival_time<=time) ready.push_back(next++);
        // keep only arrived, not finished
        ready.erase(remove_if(ready.begin(),ready.end(),[&](int i){return ps[i].remaining_time==0;}),ready.end());
    };

    arrive();
    if(ready.empty() && next<n){ gantt.push_back({"IDLE", ps[next].arrival_time-time}); time=ps[next].arrival_time; arrive(); }

    while(finished<n){
        if(ready.empty()){
            if(next<n){ gantt.push_back({"IDLE", ps[next].arrival_time-time}); time=ps[next].arrival_time; arrive(); }
            continue;
        }
        // pick min vruntime
        int i = *min_element(ready.begin(), ready.end(), [&](int a,int b){
            if (ps[a].vruntime==ps[b].vruntime) return ps[a].arrival_time<ps[b].arrival_time;
            return ps[a].vruntime < ps[b].vruntime;
        });

        double weight = 1.0 / max(1, ps[i].priority); // lower number => heavier => larger slice
        int slice = min((int)ceil(BASE_SLICE * weight), ps[i].remaining_time);
        if (slice<=0) slice=1;

        gantt.push_back({ps[i].id, slice});
        time += slice;
        ps[i].remaining_time -= slice;
        ps[i].vruntime += slice / weight; // virtual time grows faster for lighter (higher-priority number) tasks
        arrive();

        if(ps[i].remaining_time==0){
            ps[i].turnaround_time = time - ps[i].arrival_time;
            ps[i].waiting_time = ps[i].turnaround_time - ps[i].burst_time;
            finished++;
            // remove from ready list if present
            ready.erase(remove(ready.begin(),ready.end(),i), ready.end());
        }
    }
    metrics(ps,time,gantt);
}
