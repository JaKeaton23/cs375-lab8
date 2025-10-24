// edf.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process{
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0, waiting_time=0, turnaround_time=0;
    int deadline=0;
};

static void flushRun(vector<pair<string,int>>& g, string& cur, int& len){
    if(len>0){ g.push_back({cur,len}); len=0; }
}
static void metrics(vector<Process>& ps, int total, const vector<pair<string,int>>& g){
    double aw=0,at=0; for(auto&p:ps){aw+=p.waiting_time; at+=p.turnaround_time;}
    aw/=ps.size(); at/=ps.size();
    int busy=accumulate(ps.begin(),ps.end(),0,[](int s,const Process&p){return s+p.burst_time;});
    cout<<fixed<<setprecision(2);
    cout<<"Gantt: "; for(auto&e:g) cout<<e.first<<"("<<e.second<<") "; cout<<"\n";
    cout<<"Average Waiting Time: "<<aw<<"\n";
    cout<<"Average Turnaround Time: "<<at<<"\n";
    cout<<"CPU Utilization: "<<(total? 100.0*busy/total:0)<<"%\n";
    cout<<"Throughput: "<<(total? (double)ps.size()/total:0)<<" processes/unit time\n";
}

int main(){
    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    sort(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    for(auto& p:ps){
        p.remaining_time=p.burst_time;
        p.deadline = p.arrival_time + 2*p.burst_time; // default rule per sheet
    }

    int time=0, next=0, n=ps.size(), finished=0;
    vector<pair<string,int>> gantt; string run="IDLE"; int runlen=0;

    auto bestReady=[&](){
        int idx=-1; int bestDL=INT_MAX;
        for(int i=0;i<n;i++){
            if(ps[i].remaining_time>0 && ps[i].arrival_time<=time){
                if(ps[i].deadline<bestDL || (ps[i].deadline==bestDL && ps[i].remaining_time< (idx==-1?INT_MAX:ps[idx].remaining_time))){
                    bestDL=ps[i].deadline; idx=i;
                }
            }
        }
        return idx;
    };

    while(finished<n){
        // advance to first arrival if nothing ready
        if(bestReady()==-1){
            if(next<n && time<ps[next].arrival_time){
                if(run!="IDLE"){ flushRun(gantt, run, runlen); run="IDLE"; }
                runlen += ps[next].arrival_time-time;
                time = ps[next].arrival_time;
            }
        }else{
            int idx = bestReady();
            if(run != ps[idx].id){ flushRun(gantt, run, runlen); run=ps[idx].id; }
            // run 1 unit (preemptive)
            ps[idx].remaining_time--; runlen++; time++;
            if(ps[idx].remaining_time==0){
                ps[idx].turnaround_time = time - ps[idx].arrival_time;
                ps[idx].waiting_time = ps[idx].turnaround_time - ps[idx].burst_time;
                finished++;
            }
        }
    }
    flushRun(gantt, run, runlen);
    metrics(ps,time,gantt);
}
