#pragma once
#include <vector>
#include <list>
#include <map>
#include <functional>

void InitMatrix(std::vector<std::vector<int>>& mat, int w, int h, std::vector<int> data);

class Timetable {
	struct ProcessStruct {
		int detail;//-1 if idle
		int start_time;
		int duration;
	};
	int m_OperationsCount = 0;
	int m_DetailsCount = 0;
	std::vector<std::vector<ProcessStruct>> m_Timetable;//oper,time_id
	std::vector<std::vector<int>> m_TimetableMatrix;//oper_id,det
	std::vector<std::vector<int>> m_Pipeline;//oper,det
	std::vector<std::vector<int>> m_Waiting;//oper,det
	int m_LastTime = 0;
public:
	void Init(int oper_c, int det_c);
	int AddDetail(int oper_id, int det_id, int start_time, int duration);
	void UpdateLastTime(int t);
	std::vector<std::vector<Timetable::ProcessStruct>> GetTimetable();
	std::vector<std::vector<int>> GetTimetableMatrix();
	float ComputeQuality(int qua_id);
	int ComputeTrk(int oper_id);
	int ComputeTprk(int oper_id);
	int ComputeTprkBet(int oper_id);
	int ComputePrCount(int oper_id);
	int ComputePrCountBet(int oper_id);
	int GetLastTime();
};

class GSKSolver {
private:
	//Timetable m_Timetable;
	int m_OperationsCount = 0;
	int m_DetailsCount = 0;
	std::vector<std::vector<int>> m_ProcessTime;//oper,det
	std::vector<std::vector<int>> m_Pipeline;//oper,det

	std::function<int(int, int)> bag_rules[10];
	std::function<int(int, int)> compute_detail_bag_rating;
	std::vector<std::list<int>> bags;//oper, dets...
	std::vector<int> details_pipeline_progress;//det
	std::map<int, std::vector<std::pair<int, int>>> timeline_complete;//time,(operations,detail)...

	int ProcessBestDetailFromBag(std::list<int> details, Timetable& tt, int oper, int time_start);
public:
	void InitData(int oper_c, int det_c, std::vector<int> pr_mat, std::vector<int> pipe_mat=std::vector<int>());
	void ParseParameters(const char* params);//may throw

	Timetable solve_johnson(bool inverse = false);
	Timetable solve_simulation(int bag_rule_id=0);
};