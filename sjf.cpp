// sjf.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process{
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0, waiting_time=0, turnaround_time=0;
};
void printGantt(const vector<pair<string,int>>& g){
    cout<<"Gantt: "; for (auto&e:g) cout<<e.first<<"("<<e.second<<") "; cout<<"\n";
}
void metrics(vector<Process>& ps, int total, const vector<pair<string,int>>& g){
    double aw=0,at=0; for(auto&x:ps){aw+=x.waiting_time; at+=x.turnaround_time;}
    aw/=ps.size(); at/=ps.size();
    double cpu = (double)accumulate(ps.begin(),ps.end(),0,[](int s,const Process& p){return s+p.burst_time;})/max(1,total)*100.0;
    double thr = (double)ps.size()/max(1,total);
    printGantt(g);
    cout<<"Average Waiting Time: "<<aw<<"\n";
    cout<<"Average Turnaround Time: "<<at<<"\n";
    cout<<"CPU Utilization: "<<cpu<<"%\n";
    cout<<"Throughput: "<<thr<<" processes/unit time\n";
}

int main(){
    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    sort(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    vector<pair<string,int>> gantt;
    int time=0, done=0, n=ps.size();
    vector<bool> finished(n,false);

    while (done<n){
        // collect arrived and unfinished
        vector<int> ready;
        for(int i=0;i<n;i++) if(!finished[i] && ps[i].arrival_time<=time) ready.push_back(i);

        if (ready.empty()){
            int next_arr = min_element(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;})->arrival_time;
            int jump = max(0,next_arr-time);
            gantt.push_back({"IDLE", jump});
            time += jump;
            continue;
        }

        // pick shortest burst
        int idx = *min_element(ready.begin(), ready.end(),
            [&](int a,int b){return ps[a].burst_time < ps[b].burst_time;});

        gantt.push_back({ps[idx].id, ps[idx].burst_time});
        time += ps[idx].burst_time;
        ps[idx].turnaround_time = time - ps[idx].arrival_time;
        ps[idx].waiting_time    = ps[idx].turnaround_time - ps[idx].burst_time;
        finished[idx]=true; ++done;
    }
    metrics(ps,time,gantt);
}
