// mlfq.cpp
#include <bits/stdc++.h>
using namespace std;

struct Process{
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0, waiting_time=0, turnaround_time=0;
    int queue_level=0; int wait_since=0;
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
    const int Q[3] = {2,4,8};         // quanta for levels 0,1,2
    const int PROMOTE_WAIT = 12;      // simple aging: promote if waited too long

    vector<Process> ps={{"P1",0,8,2},{"P2",1,4,1},{"P3",2,9,3},{"P4",3,5,4}};
    sort(ps.begin(),ps.end(),[](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    for(auto& p:ps){ p.remaining_time=p.burst_time; p.queue_level=0; p.wait_since=0; }

    array<queue<int>,3> q{};
    int time=0, next=0, n=ps.size(), finished=0;
    vector<pair<string,int>> gantt;

    auto arrive=[&](){
        while(next<n && ps[next].arrival_time<=time){
            ps[next].queue_level=0; ps[next].wait_since=time;
            q[0].push(next++);
        }
    };

    arrive();
    if(q[0].empty()&&q[1].empty()&&q[2].empty() && next<n){
        gantt.push_back({"IDLE", ps[next].arrival_time-time});
        time = ps[next].arrival_time; arrive();
    }

    while(finished<n){
        // aging promotion
        for(int L=1; L<=2; ++L){
            if(!q[L].empty()){
                int sz=q[L].size();
                while(sz--){
                    int i=q[L].front(); q[L].pop();
                    if(time - ps[i].wait_since >= PROMOTE_WAIT && L>0){
                        ps[i].queue_level=L-1; q[L-1].push(i);
                    }else q[L].push(i);
                }
            }
        }

        int level=-1;
        for(int L=0; L<3; ++L) if(!q[L].empty()){ level=L; break; }

        if(level==-1){
            if(next<n){
                gantt.push_back({"IDLE", ps[next].arrival_time-time});
                time=ps[next].arrival_time; arrive();
            }
            continue;
        }

        int i = q[level].front(); q[level].pop();
        int slice = min(Q[level], ps[i].remaining_time);
        gantt.push_back({ps[i].id, slice});
        time += slice; ps[i].remaining_time -= slice;
        arrive();

        if(ps[i].remaining_time>0){
            // demote if used full quantum
            if(slice==Q[level] && level<2) ps[i].queue_level=level+1;
            else ps[i].queue_level=level;
            ps[i].wait_since=time;
            q[ps[i].queue_level].push(i);
        }else{
            ps[i].turnaround_time = time - ps[i].arrival_time;
            ps[i].waiting_time = ps[i].turnaround_time - ps[i].burst_time;
            finished++;
        }
    }
    metrics(ps,time,gantt);
}
