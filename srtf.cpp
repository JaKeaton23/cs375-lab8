// srtf.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process{
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0, waiting_time=0, turnaround_time=0;
};

void flushRun(vector<pair<string,int>>& g, string& cur, int& len){
    if (len>0){ g.push_back({cur,len}); len=0; }
}
void metrics(vector<Process>& ps, int total, const vector<pair<string,int>>& g){
    double aw=0,at=0; for(auto&x:ps){aw+=x.waiting_time; at+=x.turnaround_time;}
    aw/=ps.size(); at/=ps.size();
    double cpu=(double)accumulate(ps.begin(),ps.end(),0,[](int s,const Process&p){return s+p.burst_time;})/max(1,total)*100.0;
    double thr=(double)ps.size()/max(1,total);
    cout<<"Gantt: "; for(auto&e:g) cout<<e.first<<"("<<e.second<<") "; cout<<"\n";
    cout<<"Average Waiting Time: "<<aw<<"\n";
    cout<<"Average Turnaround Time: "<<at<<"\n";
    cout<<"CPU Utilization: "<<cpu<<"%\n";
    cout<<"Throughput: "<<thr<<" processes/unit time\n";
}

int main(){
    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    for (auto& p: ps) p.remaining_time = p.burst_time;

    int time=0, finished=0, n=ps.size();
    vector<pair<string,int>> gantt; string run="IDLE"; int runlen=0;

    while (finished<n){
        // choose arrived with smallest remaining
        int idx=-1; int best=INT_MAX; int next_arr=INT_MAX;
        for (int i=0;i<n;i++){
            if (ps[i].remaining_time>0){
                next_arr=min(next_arr, ps[i].arrival_time);
                if (ps[i].arrival_time<=time && ps[i].remaining_time<best){
                    best=ps[i].remaining_time; idx=i;
                }
            }
        }
        if (idx==-1){ // idle until next arrival
            if (time<next_arr){
                if (run!="IDLE"){ flushRun(gantt, run, runlen); run="IDLE"; }
                runlen += (next_arr-time); time = next_arr;
                continue;
            }
        } else {
            // context switch if needed
            if (run!=ps[idx].id){ flushRun(gantt, run, runlen); run=ps[idx].id; }
            // run for 1 tick
            ps[idx].remaining_time--; runlen++; time++;
            if (ps[idx].remaining_time==0){
                ps[idx].turnaround_time = time - ps[idx].arrival_time;
                ps[idx].waiting_time    = ps[idx].turnaround_time - ps[idx].burst_time;
                finished++;
            }
        }
    }
    flushRun(gantt, run, runlen);
    metrics(ps,time,gantt);
}
