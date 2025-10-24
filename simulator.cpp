// simulator.cpp 
// Modular Task Scheduling Simulator: fcfs, sjf, srtf, priority, rr, mlq, mlfq, lottery, cfs, edf
#include <bits/stdc++.h>
using namespace std;

struct Process {
    string id; int arrival_time; int burst_time; int priority;
    int remaining_time=0; int waiting_time=0; int turnaround_time=0;
    int deadline=0;                // for EDF
    double vruntime=0.0;           // for CFS
    int qlevel=0;                  // for MLFQ
};

using Gantt = vector<pair<string,int>>;

static void printGantt(const Gantt& g){
    cout << "Gantt: ";
    for (auto& e: g) cout << e.first << "(" << e.second << ") ";
    cout << "\n";
}
static void calcMetrics(vector<Process>& ps, int total_time,
                        double& avg_wait, double& avg_turn, double& cpu, double& thr){
    double aw=0, at=0; int busy=0;
    for (auto& p: ps){ aw += p.waiting_time; at += p.turnaround_time; busy += p.burst_time; }
    int n = (int)ps.size();
    avg_wait = n? aw/n : 0; avg_turn = n? at/n : 0;
    cpu = total_time? (100.0*busy/total_time) : 0.0;
    thr = total_time? (double)n/total_time : 0.0;
}
static void printResults(vector<Process>& ps, int total_time, const Gantt& g){
    double aw,at,cpu,thr; calcMetrics(ps,total_time,aw,at,cpu,thr);
    printGantt(g);
    cout<<fixed<<setprecision(2);
    cout<<"Average Waiting Time: "<<aw<<"\n";
    cout<<"Average Turnaround Time: "<<at<<"\n";
    cout<<"CPU Utilization: "<<cpu<<"%\n";
    cout<<"Throughput: "<<thr<<" processes/unit time\n";
}

class Scheduler{
public:
    virtual ~Scheduler()=default;
    virtual string name() const = 0;
    virtual void schedule(vector<Process>& ps, Gantt& g, int& total_time)=0;
};

// -------- shared helpers --------
static void ensureRemaining(vector<Process>& ps){ for(auto& p: ps) p.remaining_time=p.burst_time; }
static void sortByArrival(vector<Process>& ps){
    sort(ps.begin(), ps.end(), [](const Process&a, const Process&b){return a.arrival_time<b.arrival_time;});
}
static int nextArrivalAfter(const vector<Process>& ps, int t){
    int nx=INT_MAX; for (auto& p: ps) if(p.remaining_time>0) nx=min(nx, p.arrival_time);
    return (nx==INT_MAX)? t : nx;
}
static void idleUntil(int& cur, int to, Gantt& g){ if(to>cur){ g.push_back({"IDLE", to-cur}); cur=to; } }

// ================= implementations =================

// FCFS
class FCFSScheduler: public Scheduler{
public: string name() const override { return "fcfs"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); int t=0;
        for (auto& p: ps){
            if (p.arrival_time>t) idleUntil(t, p.arrival_time, g);
            g.push_back(make_pair(p.id, p.burst_time));
            t += p.burst_time;
            p.turnaround_time = t - p.arrival_time;
            p.waiting_time    = p.turnaround_time - p.burst_time;
        }
        total=t;
    }
};

// SJF (non-preemptive)
class SJFScheduler: public Scheduler{
public: string name() const override { return "sjf"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps);
        int n=ps.size(), t=0, done=0; vector<char> fin(n,0);
        while(done<n){
            int idx=-1;
            for(int i=0;i<n;i++) if(!fin[i] && ps[i].arrival_time<=t)
                if(idx==-1 || ps[i].burst_time<ps[idx].burst_time) idx=i;
            if(idx==-1){ idleUntil(t, nextArrivalAfter(ps,t), g); continue; }
            g.push_back(make_pair(ps[idx].id, ps[idx].burst_time));
            t+=ps[idx].burst_time;
            ps[idx].turnaround_time=t-ps[idx].arrival_time;
            ps[idx].waiting_time=ps[idx].turnaround_time-ps[idx].burst_time;
            fin[idx]=1; done++;
        }
        total=t;
    }
};

// SRTF
class SRTFScheduler: public Scheduler{
public: string name() const override { return "srtf"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); ensureRemaining(ps);
        int n=ps.size(), t=0, finished=0; string run="IDLE"; int runlen=0;
        auto flush=[&](){ if(runlen>0){ g.push_back(make_pair(run,runlen)); runlen=0; } };
        while(finished<n){
            int idx=-1, best=INT_MAX, nx=INT_MAX;
            for(int i=0;i<n;i++){
                if(ps[i].remaining_time>0){ nx=min(nx, ps[i].arrival_time); }
                if(ps[i].arrival_time<=t && ps[i].remaining_time>0 && ps[i].remaining_time<best){ best=ps[i].remaining_time; idx=i; }
            }
            if(idx==-1){ if(run!="IDLE"){ flush(); run="IDLE"; } runlen+=max(0,nx-t); t=nx; continue; }
            if(run!=ps[idx].id){ flush(); run=ps[idx].id; }
            ps[idx].remaining_time--; runlen++; t++;
            if(ps[idx].remaining_time==0){
                flush();
                ps[idx].turnaround_time=t-ps[idx].arrival_time;
                ps[idx].waiting_time=ps[idx].turnaround_time-ps[idx].burst_time;
                finished++; run="IDLE";
            }
        }
        total=t;
    }
};

// Priority (non-preemptive, lower number = higher priority) with simple aging
class PriorityNPScheduler: public Scheduler{
public: string name() const override { return "priority"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps);
        const int AGE_STEP=5;
        int n=ps.size(), t=0, done=0, last_age=0; vector<char> fin(n,0);
        while(done<n){
            if(t-last_age>=AGE_STEP){
                for(int i=0;i<n;i++) if(!fin[i] && ps[i].arrival_time<=t)
                    ps[i].priority=max(0, ps[i].priority-1);
                last_age=t;
            }
            int idx=-1;
            for(int i=0;i<n;i++) if(!fin[i] && ps[i].arrival_time<=t){
                if(idx==-1 || ps[i].priority<ps[idx].priority ||
                   (ps[i].priority==ps[idx].priority && ps[i].burst_time<ps[idx].burst_time))
                    idx=i;
            }
            if(idx==-1){ idleUntil(t, nextArrivalAfter(ps,t), g); continue; }
            g.push_back(make_pair(ps[idx].id, ps[idx].burst_time));
            t+=ps[idx].burst_time;
            ps[idx].turnaround_time=t-ps[idx].arrival_time;
            ps[idx].waiting_time=ps[idx].turnaround_time-ps[idx].burst_time;
            fin[idx]=1; done++;
        }
        total=t;
    }
};

// Round Robin
class RRScheduler: public Scheduler{
    int q;
public:
    explicit RRScheduler(int quantum): q(quantum) {}
    string name() const override { return "rr"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); ensureRemaining(ps);
        int n=ps.size(), t=0, nextIdx=0, fin=0; queue<int> rq;
        auto arrive=[&](){ while(nextIdx<n && ps[nextIdx].arrival_time<=t) rq.push(nextIdx++); };
        arrive(); if(rq.empty() && nextIdx<n){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); }
        while(fin<n){
            if(rq.empty()){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); continue; }
            int i=rq.front(); rq.pop();
            int slice=min(q, ps[i].remaining_time);
            g.push_back(make_pair(ps[i].id, slice)); t+=slice; ps[i].remaining_time-=slice; arrive();
            if(ps[i].remaining_time>0) rq.push(i);
            else { ps[i].turnaround_time=t-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; fin++; }
        }
        total=t;
    }
};

// MLQ (High RR q=4 if priority<3, else Low FCFS)
class MLQScheduler: public Scheduler{
public: string name() const override { return "mlq"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); ensureRemaining(ps);
        const int RRQ=4;
        int n=ps.size(), t=0, nextIdx=0, fin=0; queue<int> hi, lo;
        auto arrive=[&](){
            while(nextIdx<n && ps[nextIdx].arrival_time<=t){
                (ps[nextIdx].priority<3 ? hi : lo).push(nextIdx++);
            }
        };
        arrive(); if(hi.empty()&&lo.empty()&&nextIdx<n){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); }
        while(fin<n){
            if(!hi.empty()){
                int i=hi.front(); hi.pop();
                int slice=min(RRQ, ps[i].remaining_time);
                g.push_back(make_pair(ps[i].id, slice)); t+=slice; ps[i].remaining_time-=slice; arrive();
                if(ps[i].remaining_time>0) hi.push(i);
                else { ps[i].turnaround_time=t-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; fin++; }
            } else if(!lo.empty()){
                int i=lo.front(); lo.pop();
                int run=ps[i].remaining_time;
                g.push_back(make_pair(ps[i].id, run)); t+=run; ps[i].remaining_time=0; arrive();
                ps[i].turnaround_time=t-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; fin++;
            } else {
                idleUntil(t, ps[nextIdx].arrival_time, g); arrive();
            }
        }
        total=t;
    }
};

// MLFQ (3 queues, RR quanta 2/4/8; demote on full quantum; simple periodic promotion)
class MLFQScheduler: public Scheduler{
public: string name() const override { return "mlfq"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); ensureRemaining(ps);
        const int Q[3]={2,4,8}; const int PROMOTE_PERIOD=12;
        array<queue<int>,3> q{}; int n=ps.size(), t=0, nextIdx=0, fin=0;
        for(auto& p:ps) p.qlevel=0;

        auto arrive=[&](){ while(nextIdx<n && ps[nextIdx].arrival_time<=t){ ps[nextIdx].qlevel=0; q[0].push(nextIdx++); } };
        auto periodicPromote=[&](){
            if(t==0 || t%PROMOTE_PERIOD) return;
            for(int L=2; L>=1; --L){
                int s=q[L].size();
                while(s--){ int i=q[L].front(); q[L].pop(); q[L-1].push(i); }
            }
        };

        arrive(); if(q[0].empty()&&q[1].empty()&&q[2].empty()&&nextIdx<n){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); }
        while(fin<n){
            periodicPromote();
            int L = !q[0].empty()?0:(!q[1].empty()?1:(!q[2].empty()?2:-1));
            if(L==-1){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); continue; }
            int i=q[L].front(); q[L].pop();
            int slice=min(Q[L], ps[i].remaining_time);
            g.push_back(make_pair(ps[i].id, slice)); t+=slice; ps[i].remaining_time-=slice; arrive();
            if(ps[i].remaining_time>0){
                int NL = (slice==Q[L] && L<2)? L+1 : L;
                q[NL].push(i);
            }else{
                ps[i].turnaround_time=t-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; fin++;
            }
        }
        total=t;
    }
};

// Lottery (quantum 4; tickets ~ 10/priority)
class LotteryScheduler: public Scheduler{
public: string name() const override { return "lottery"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); ensureRemaining(ps); const int QUANTUM=4;
        int n=ps.size(), t=0, nextIdx=0, fin=0; vector<int> ready;
        mt19937 gen((uint32_t)chrono::high_resolution_clock::now().time_since_epoch().count());
        auto arrive=[&](){
            while(nextIdx<n && ps[nextIdx].arrival_time<=t) ready.push_back(nextIdx++);
            ready.erase(remove_if(ready.begin(),ready.end(),[&](int i){return ps[i].remaining_time==0;}), ready.end());
        };
        auto tickets=[&](int i){ return max(1, 10 / max(1, ps[i].priority)); };

        arrive(); if(ready.empty()&&nextIdx<n){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); }
        while(fin<n){
            if(ready.empty()){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); continue; }
            int tot=0; for(int i:ready) tot+=tickets(i);
            uniform_int_distribution<int> dist(1,tot);
            int pick=dist(gen), acc=0, chosen=ready.front();
            for(int i:ready){ acc+=tickets(i); if(pick<=acc){ chosen=i; break; } }
            int slice=min(QUANTUM, ps[chosen].remaining_time);
            g.push_back(make_pair(ps[chosen].id, slice)); t+=slice; ps[chosen].remaining_time-=slice; arrive();
            if(ps[chosen].remaining_time==0){
                ps[chosen].turnaround_time=t-ps[chosen].arrival_time;
                ps[chosen].waiting_time=ps[chosen].turnaround_time-ps[chosen].burst_time; fin++;
            }
        }
        total=t;
    }
};

// CFS-lite (min vruntime; slice ∝ 1/priority)
class CFSScheduler: public Scheduler{
public: string name() const override { return "cfs"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); ensureRemaining(ps); const int BASE_SLICE=4;
        int n=ps.size(), t=0, nextIdx=0, fin=0; vector<int> ready;
        auto arrive=[&](){
            while(nextIdx<n && ps[nextIdx].arrival_time<=t) ready.push_back(nextIdx++);
            ready.erase(remove_if(ready.begin(),ready.end(),[&](int i){return ps[i].remaining_time==0;}), ready.end());
        };
        for(auto& p: ps) p.vruntime=0.0;

        arrive(); if(ready.empty()&&nextIdx<n){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); }
        while(fin<n){
            if(ready.empty()){ idleUntil(t, ps[nextIdx].arrival_time, g); arrive(); continue; }
            int i=*min_element(ready.begin(), ready.end(), [&](int a,int b){
                if(ps[a].vruntime==ps[b].vruntime) return ps[a].arrival_time<ps[b].arrival_time;
                return ps[a].vruntime<ps[b].vruntime;
            });
            double w = 1.0 / max(1, ps[i].priority);
            int slice = max(1, min((int)ceil(BASE_SLICE*w), ps[i].remaining_time));
            g.push_back(make_pair(ps[i].id, slice)); t+=slice; ps[i].remaining_time-=slice; ps[i].vruntime += slice / w; arrive();
            if(ps[i].remaining_time==0){
                ps[i].turnaround_time=t-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; fin++;
                ready.erase(remove(ready.begin(),ready.end(),i), ready.end());
            }
        }
        total=t;
    }
};

// EDF (preemptive). If no deadline present, use arrival + 2*burst.
class EDFScheduler: public Scheduler{
public: string name() const override { return "edf"; }
    void schedule(vector<Process>& ps, Gantt& g, int& total) override {
        sortByArrival(ps); ensureRemaining(ps);
        for(auto& p: ps) if(p.deadline==0) p.deadline = p.arrival_time + 2*p.burst_time;

        int n=ps.size(), t=0, finished=0; string run="IDLE"; int runlen=0;
        auto flush=[&](){ if(runlen>0){ g.push_back(make_pair(run,runlen)); runlen=0; } };
        auto best=[&]()->int{
            int idx=-1, bestDL=INT_MAX, bestRem=INT_MAX;
            for(int i=0;i<n;i++) if(ps[i].remaining_time>0 && ps[i].arrival_time<=t){
                if(ps[i].deadline<bestDL || (ps[i].deadline==bestDL && ps[i].remaining_time<bestRem)){
                    bestDL=ps[i].deadline; bestRem=ps[i].remaining_time; idx=i;
                }
            }
            return idx;
        };

        while(finished<n){
            int idx = best();
            if(idx==-1){
                int nx=nextArrivalAfter(ps,t);
                if(run!="IDLE"){ flush(); run="IDLE"; }
                runlen += max(0, nx-t); t=nx; continue;
            }
            if(run!=ps[idx].id){ flush(); run=ps[idx].id; }
            ps[idx].remaining_time--; runlen++; t++;
            if(ps[idx].remaining_time==0){
                flush();
                ps[idx].turnaround_time=t-ps[idx].arrival_time;
                ps[idx].waiting_time=ps[idx].turnaround_time-ps[idx].burst_time; finished++; run="IDLE";
            }
        }
        total=t;
    }
};

// -------------- Input --------------
static vector<Process> loadProcesses(const string& filename){
    vector<Process> ps;
    if(filename.empty()) return ps;
    ifstream in(filename);
    if(!in){ cerr<<"Error opening file: "<<filename<<"\n"; return ps; }
    string line;
    while(getline(in,line)){
        // strip leading spaces
        size_t i=line.find_first_not_of(" \t\r\n");
        if(i==string::npos) continue;
        if(line[i]=='#') continue;
        // remove inline comment
        size_t hash=line.find('#'); if(hash!=string::npos) line=line.substr(0,hash);
        istringstream iss(line);
        Process p{};
        if(!(iss>>p.id>>p.arrival_time>>p.burst_time>>p.priority)) continue;
        if(iss>>p.deadline) { /* explicit deadline parsed */ }
        ps.push_back(p);
    }
    return ps;
}
static vector<Process> defaultProcesses(){
    return { {"P1",0,8,2}, {"P2",1,4,1}, {"P3",2,9,3}, {"P4",3,5,4} };
}
static vector<Process> generateRandom(int n){
    vector<Process> ps; ps.reserve(n);
    mt19937 gen((uint32_t)chrono::high_resolution_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> A(0,20), B(1,10), P(1,5);
    for(int i=1;i<=n;i++){
        Process p; p.id="P"+to_string(i); p.arrival_time=A(gen); p.burst_time=B(gen); p.priority=P(gen);
        ps.push_back(p);
    }
    sortByArrival(ps);
    return ps;
}

// -------------- main --------------
int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    map<string,string> args;
    for(int i=1;i+1<=argc-1;i+=2) args[argv[i]]=argv[i+1];
    string type = args.count("--scheduler")? args["--scheduler"] : "rr";
    string input= args.count("--input")? args["--input"] : "";
    int quantum = args.count("--quantum")? max(1, stoi(args["--quantum"])) : 4;
    bool useRandom = args.count("--random");
    int numRandom = args.count("--num")? max(1, stoi(args["--num"])) : 10;

    vector<Process> ps = useRandom ? generateRandom(numRandom)
                                   : (!input.empty()? loadProcesses(input) : defaultProcesses());
    if(ps.empty()){ cerr<<"No processes loaded.\n"; return 1; }

    unique_ptr<Scheduler> sch;
    if      (type=="fcfs")    sch = make_unique<FCFSScheduler>();
    else if (type=="sjf")     sch = make_unique<SJFScheduler>();
    else if (type=="srtf")    sch = make_unique<SRTFScheduler>();
    else if (type=="priority")sch = make_unique<PriorityNPScheduler>();
    else if (type=="rr")      sch = make_unique<RRScheduler>(quantum);
    else if (type=="mlq")     sch = make_unique<MLQScheduler>();
    else if (type=="mlfq")    sch = make_unique<MLFQScheduler>();
    else if (type=="lottery") sch = make_unique<LotteryScheduler>();
    else if (type=="cfs")     sch = make_unique<CFSScheduler>();
    else if (type=="edf")     sch = make_unique<EDFScheduler>();
    else { cerr<<"Unknown scheduler: "<<type<<"\n"; return 1; }

    Gantt g; int total=0;
    sch->schedule(ps, g, total);
    printResults(ps, total, g);
    return 0;
}
