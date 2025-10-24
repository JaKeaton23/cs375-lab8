// mlq.cpp
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
    int busy = accumulate(ps.begin(),ps.end(),0,[](int s,const Process&p){return s+p.burst_time;});
    cout<<fixed<<setprecision(2);
    printGantt(g);
    cout<<"Average Waiting Time: "<<aw<<"\n";
    cout<<"Average Turnaround Time: "<<at<<"\n";
    cout<<"CPU Utilization: "<<(total? (100.0*busy/total):0)<<"%\n";
    cout<<"Throughput: "<<(total? (double)ps.size()/total:0)<<" processes/unit time\n";
}

int main(){
    const int RRQ = 4;
    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    sort(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    for(auto& p:ps) p.remaining_time = p.burst_time;

    queue<int> highRR, lowFCFS;
    int time=0, iNext=0, n=ps.size(), finished=0;
    vector<pair<string,int>> gantt;

    // 🔧 FIXED: push index first, then increment (avoids sequence-point warning)
    auto enqueueArrivals=[&](){
        while(iNext<n && ps[iNext].arrival_time<=time){
            if(ps[iNext].priority<3) highRR.push(iNext);
            else                     lowFCFS.push(iNext);
            iNext++;
        }
    };

    enqueueArrivals();
    if(highRR.empty()&&lowFCFS.empty() && iNext<n){
        gantt.push_back({"IDLE", ps[iNext].arrival_time-time});
        time = ps[iNext].arrival_time; enqueueArrivals();
    }

    while(finished<n){
        if(!highRR.empty()){
            int i = highRR.front(); highRR.pop();
            int slice = min(RRQ, ps[i].remaining_time);
            gantt.push_back({ps[i].id, slice});
            time += slice; ps[i].remaining_time -= slice;
            enqueueArrivals();
            if (ps[i].remaining_time>0) highRR.push(i);
            else {
                ps[i].turnaround_time = time - ps[i].arrival_time;
                ps[i].waiting_time    = ps[i].turnaround_time - ps[i].burst_time;
                finished++;
            }
        } else if(!lowFCFS.empty()){
            int i = lowFCFS.front(); lowFCFS.pop();
            int run = ps[i].remaining_time;
            gantt.push_back({ps[i].id, run});
            time += run; ps[i].remaining_time = 0;
            enqueueArrivals();
            ps[i].turnaround_time = time - ps[i].arrival_time;
            ps[i].waiting_time    = ps[i].turnaround_time - ps[i].burst_time;
            finished++;
        } else {
            // idle to next arrival
            if(iNext<n){
                gantt.push_back({"IDLE", ps[iNext].arrival_time-time});
                time = ps[iNext].arrival_time;
                enqueueArrivals();
            }
        }
    }
    metrics(ps,time,gantt);
}
