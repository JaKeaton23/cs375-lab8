// rr.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process{
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0, waiting_time=0, turnaround_time=0;
};

void printGantt(const vector<pair<string,int>>& g){
    cout<<"Gantt: "; for(auto&e:g) cout<<e.first<<"("<<e.second<<") "; cout<<"\n";
}
void metrics(vector<Process>& ps, int total, const vector<pair<string,int>>& g){
    double aw=0,at=0; for(auto&x:ps){aw+=x.waiting_time; at+=x.turnaround_time;}
    aw/=ps.size(); at/=ps.size();
    double cpu=(double)accumulate(ps.begin(),ps.end(),0,[](int s,const Process&p){return s+p.burst_time;})/max(1,total)*100.0;
    double thr=(double)ps.size()/max(1,total);
    printGantt(g);
    cout<<"Average Waiting Time: "<<aw<<"\n";
    cout<<"Average Turnaround Time: "<<at<<"\n";
    cout<<"CPU Utilization: "<<cpu<<"%\n";
    cout<<"Throughput: "<<thr<<" processes/unit time\n";
}

int main(){
    const int QUANTUM=4;
    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    for (auto& p: ps) p.remaining_time=p.burst_time;

    queue<int> rq;
    vector<pair<string,int>> gantt;
    int time=0, nextIdx=0, n=ps.size(), finished=0;

    auto pushArrivals=[&](){
        while (nextIdx<n && ps[nextIdx].arrival_time<=time)
            rq.push(nextIdx++); };

    sort(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    pushArrivals();
    if (rq.empty() && nextIdx<n){ gantt.push_back({"IDLE", ps[nextIdx].arrival_time-time}); time=ps[nextIdx].arrival_time; pushArrivals(); }

    while (finished<n){
        if (rq.empty()){
            // idle to next arrival
            int next_t = ps[nextIdx].arrival_time;
            gantt.push_back({"IDLE", next_t-time});
            time = next_t;
            pushArrivals();
            continue;
        }
        int i=rq.front(); rq.pop();
        int slice=min(QUANTUM, ps[i].remaining_time);
        gantt.push_back({ps[i].id, slice});
        time += slice;
        ps[i].remaining_time -= slice;

        // arrivals during slice
        pushArrivals();

        if (ps[i].remaining_time>0){
            rq.push(i);
        } else {
            ps[i].turnaround_time = time - ps[i].arrival_time;
            ps[i].waiting_time    = ps[i].turnaround_time - ps[i].burst_time;
            finished++;
        }
    }
    metrics(ps,time,gantt);
}
