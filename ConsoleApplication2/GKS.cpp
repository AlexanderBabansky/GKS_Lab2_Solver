#include "GKS.h"
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <functional>
#include <map>
#include <list>

#define CHECK_VALUE(a) if(a<=0)throw std::runtime_error("Invalid input");
using namespace std;

void GSKSolver::InitData(int oper_c, int det_c, std::vector<int> pr_mat, std::vector<int> pipe_mat)
{
	_ASSERT(oper_c > 0 && det_c > 0 && pr_mat.size() == oper_c * det_c);
	if (oper_c <= 0 || det_c <= 0 || pr_mat.size() != oper_c * det_c)
		throw runtime_error("Invalid input");
	m_OperationsCount = oper_c;
	m_DetailsCount = det_c;
	InitMatrix(m_ProcessTime, oper_c, det_c, pr_mat);
	if (pipe_mat.size()) {
		_ASSERT(pipe_mat.size() == oper_c * det_c);
		InitMatrix(m_Pipeline, oper_c, det_c, pipe_mat);
	}

	bag_rules[0] = [&](int det, int progress) {//SPT
		progress--;
		return INT_MAX - m_ProcessTime[m_Pipeline[progress][det] - 1][det];
	};
	bag_rules[1] = [&](int det, int progress) {//MWKR
		int res = 0;
		for (int a = progress; a < m_OperationsCount; a++) {
			res += m_ProcessTime[m_Pipeline[progress][det] - 1][det];
		}
		return res;
	};
	bag_rules[3] = [&](int det, int progress) {//LUKR
		int res = 0;
		for (int a = progress; a < m_OperationsCount; a++) {
			res += m_ProcessTime[m_Pipeline[progress][det] - 1][det];
		}
		return INT_MAX - res;
	};
}

void GSKSolver::ParseParameters(const char* params)
{
	std::stringstream ss;
	ss << params;
	std::string read_val;
	while (!ss.eof()) {
		ss >> read_val;
		if (read_val == "-m") {
			ss >> read_val;
			m_OperationsCount = atoi(read_val.c_str());
			CHECK_VALUE(m_OperationsCount);
		}
		else if (read_val == "-n") {
			ss >> read_val;
			m_DetailsCount = atoi(read_val.c_str());
			CHECK_VALUE(m_DetailsCount);
		}
		else if (read_val == "-t") {
			CHECK_VALUE(m_DetailsCount);
			CHECK_VALUE(m_OperationsCount);
			ss >> read_val;
			std::ifstream matrix_file;
			matrix_file.open(read_val);
			if (!matrix_file.is_open())throw std::runtime_error("No matrix file");

			std::vector<int> temp_matrix;
			temp_matrix.resize(m_OperationsCount * m_DetailsCount);
			int idx = 0;
			while (!matrix_file.eof() && idx < m_OperationsCount * m_DetailsCount) {
				matrix_file >> read_val;
				int v = atoi(read_val.c_str());
				if (v <= 0) {
					matrix_file.close();
					throw std::runtime_error("Invalid input");
				}
				temp_matrix[idx] = v;
				idx++;
			}
			InitMatrix(m_ProcessTime, m_OperationsCount, m_DetailsCount, temp_matrix);
			matrix_file.close();
		}
	}
}

Timetable GSKSolver::solve_johnson(bool inverse)
{
	_ASSERT(m_OperationsCount == 3);
	std::vector<int> group1, group2;

	//divide to groups
	for (int a = 0; a < m_DetailsCount; a++) {
		int n1 = m_ProcessTime[0][a] + m_ProcessTime[1][a];
		int n2 = m_ProcessTime[1][a] + m_ProcessTime[2][a];
		if (n1 <= n2) {
			group1.push_back(a);
		}
		else {
			group2.push_back(a);
		}
	}

	//sort groups
	std::sort(group1.begin(), group1.end(), [&](int a, int b) {
		int n1 = m_ProcessTime[0][a] + m_ProcessTime[1][a];
		int n2 = m_ProcessTime[0][b] + m_ProcessTime[1][b];
		if (n1 < n2)
			return true;
		return false;
		});
	std::sort(group2.begin(), group2.end(), [&](int a, int b) {
		int n1 = m_ProcessTime[1][a] + m_ProcessTime[2][a];
		int n2 = m_ProcessTime[1][b] + m_ProcessTime[2][b];
		if (n1 > n2)
			return true;
		return false;
		});

	Timetable tt;
	tt.Init(m_OperationsCount, m_DetailsCount);

	std::vector<int> add_dets;
	if (!inverse) {
		add_dets = group1;
		for (auto& i : group2) {
			add_dets.push_back(i);
		}
	}
	else {
		if (group2.size()) {
			auto it = group2.end();
			do {
				it--;
				add_dets.push_back(*it);
			} while (it != group2.begin());
		}

		if (group1.size()) {
			auto it = group1.end();
			do {
				it--;
				add_dets.push_back(*it);
			} while (it != group1.begin());
		}
	}

	int first_to = 0;
	for (auto& i : add_dets) {
		first_to = tt.AddDetail(0, i, first_to, m_ProcessTime[0][i]) + m_ProcessTime[0][i];
		int time_offset = first_to;
		for (int a = 1; a < m_OperationsCount; a++) {
			time_offset = tt.AddDetail(a, i, time_offset, m_ProcessTime[a][i]) + m_ProcessTime[a][i];
		}
	}
	return tt;
}

void RemoveDetailFromAllBags(std::vector<std::list<int>>& bags, int det) {
	for (auto& b : bags) {
		for (auto a = b.begin(); a != b.end(); a++) {
			if (*a == det) {
				b.erase(a);
				break;
			}
		}
	}
}

int GSKSolver::ProcessBestDetailFromBag(std::list<int> details, Timetable& tt, int oper, int time_start) {
	int max_rating = -1, max_detail = 0;
	for (auto& d : details) {
		int r = compute_detail_bag_rating(d, details_pipeline_progress[d]);
		if (max_rating < r) {
			max_rating = r;
			max_detail = d;
		}
	}
	tt.AddDetail(oper, max_detail, time_start, m_ProcessTime[oper][max_detail]);
	RemoveDetailFromAllBags(bags, max_detail);
	timeline_complete[time_start + m_ProcessTime[oper][max_detail]].push_back(std::make_pair(oper, max_detail));
	return 1;
}

Timetable GSKSolver::solve_simulation(int bag_rule_id)
{
	if (m_Pipeline.size() != m_OperationsCount)
		throw runtime_error("Invalid pipeline matrix");
	std::list<int> idle_operations;
	compute_detail_bag_rating = bag_rules[bag_rule_id];

	bags.clear();
	details_pipeline_progress.clear();
	bags.resize(m_OperationsCount);
	details_pipeline_progress.resize(m_DetailsCount);

	Timetable tt;
	tt.Init(m_OperationsCount, m_DetailsCount);
	for (int a = 0; a < m_DetailsCount; a++) {
		bags[m_Pipeline[details_pipeline_progress[a]][a] - 1].push_back(a);
		details_pipeline_progress[a]++;
	}

	int time = 0;
	int oper = 0;
	for (auto& b : bags) {
		if (b.size()) {
			ProcessBestDetailFromBag(b, tt, oper, time);
		}
		else {
			idle_operations.push_back(oper);
		}
		oper++;
	}
	do {
		//move to next timeline
		auto tp = *(timeline_complete.begin());
		time = tp.first;
		//push to bags next pipeline
		for (auto& t : tp.second) {
			if (details_pipeline_progress[t.second] < m_OperationsCount) {
				bags[m_Pipeline[details_pipeline_progress[t.second]][t.second] - 1].push_back(t.second);
				details_pipeline_progress[t.second]++;
			}
		}
		//free operations
		for (auto& t : tp.second) {
			idle_operations.push_back(t.first);
		}
		//handle idle operations
		for (auto t = idle_operations.begin(); t != idle_operations.end();) {
			if (bags[*t].size()) {
				ProcessBestDetailFromBag(bags[*t], tt, *t, time);
				auto t_b = t;
				t_b++;
				idle_operations.erase(t);
				t = t_b;
			}
			else {
				t++;
			}
		}
		timeline_complete.erase(time);
	} while (timeline_complete.size());
	return tt;
}

void InitMatrix(std::vector<std::vector<int>>& mat, int w, int h, std::vector<int> data)
{
	_ASSERT(data.size() == w * h);
	mat.clear();
	for (int a = 0; a < w; a++) {
		std::vector<int> col;
		for (int b = 0; b < h; b++) {
			col.push_back(data[w * b + a]);
		}
		mat.push_back(col);
	}
}

void Timetable::Init(int oper_c, int det_c)
{
	m_Timetable.clear();
	m_Waiting.clear();
	m_TimetableMatrix.clear();
	m_Pipeline.clear();

	m_OperationsCount = oper_c;
	m_DetailsCount = det_c;

	m_Timetable.resize(oper_c);
	m_Waiting.resize(oper_c);
	m_TimetableMatrix.resize(oper_c);
	m_Pipeline.resize(oper_c);

	for (int a = 0; a < oper_c; a++) {
		m_Waiting[a].resize(det_c);
		m_TimetableMatrix[a].resize(det_c);
	}
}

int Timetable::AddDetail(int oper_id, int det_id, int start_time, int duration)
{
	_ASSERT(oper_id < m_OperationsCount);
	auto& operation_tt = m_Timetable[oper_id];

	if (operation_tt.size()) {
		auto last_ps = operation_tt.back();
		if (last_ps.start_time + last_ps.duration >= start_time) {//no idle
			ProcessStruct ps{ det_id,last_ps.start_time + last_ps.duration, duration };
			operation_tt.push_back(ps);
			m_Waiting[oper_id][det_id] = ps.start_time - start_time;
			m_TimetableMatrix[oper_id][ps.detail] = ps.start_time;
			UpdateLastTime(ps.start_time + duration);
			return ps.start_time;
		}
		else {//has idle
			ProcessStruct ps{ -1,last_ps.start_time + last_ps.duration, start_time - (last_ps.start_time + last_ps.duration) };
			operation_tt.push_back(ps);

			ps = { det_id,start_time, duration };
			operation_tt.push_back(ps);
			m_TimetableMatrix[oper_id][ps.detail] = ps.start_time;
			UpdateLastTime(ps.start_time + duration);
			return start_time;
		}
	}
	else {
		if (start_time != 0) {
			ProcessStruct ps{ -1,0, start_time };
			operation_tt.push_back(ps);
		}
		ProcessStruct ps{ det_id,start_time, duration };
		operation_tt.push_back(ps);
		m_TimetableMatrix[oper_id][ps.detail] = ps.start_time;
		UpdateLastTime(ps.start_time + duration);
		return start_time;
	}
}

void Timetable::UpdateLastTime(int t)
{
	if (m_LastTime < t) {
		m_LastTime = t;
	}
}

std::vector<std::vector<Timetable::ProcessStruct>> Timetable::GetTimetable()
{
	return m_Timetable;
}

std::vector<std::vector<int>> Timetable::GetTimetableMatrix()
{
	return m_TimetableMatrix;
}

float Timetable::ComputeQuality(int qua_id)
{
	switch (qua_id)
	{
	case 12:
	{
		int T_max = 0;
		for (int k = 0; k < m_OperationsCount; k++) {
			int Trk = ComputeTrk(k), Tprk = ComputeTprk(k);
			if (T_max < (Trk + Tprk))T_max = Trk + Tprk;
		}
		return T_max;
	}
	case 22:
	{
		float K_sum = 0;
		for (int k = 0; k < m_OperationsCount; k++) {
			int Trk = ComputeTrk(k), Tprk = ComputeTprk(k);
			K_sum += 1.0f * Trk / (Trk + Tprk);
		}
		return K_sum;
	}
	case 24:
	{
		int Tprk_max = 0;
		for (int k = 0; k < m_OperationsCount; k++) {
			for (auto& i : m_Timetable[k]) {
				if (i.detail == -1 && i.start_time != 0) {
					if (Tprk_max < i.duration) { Tprk_max = i.duration; }
				}
			}
		}
		return Tprk_max;
	}
	case 27:
	{
		float T_sum = 0;
		for (int k = 0; k < m_OperationsCount; k++) {
			auto pr_count = ComputePrCountBet(k);
			if (pr_count) {
				T_sum += 1.0f * ComputeTprkBet(k) / pr_count;
			}
		}
		return T_sum;
	}
	case 32:
	{
		int Toch_max = 0;
		for (int j = 0; j < m_DetailsCount; j++) {
			int Toch_sum = 0;
			for (int i = 0; i < m_OperationsCount; i++) {
				Toch_sum += m_Waiting[i][j];
			}
			if (Toch_max < Toch_sum)Toch_max = Toch_sum;
		}
		return Toch_max;
	}
	case 35:
	{
		float Toch_max = 0;
		for (int j = 0; j < m_DetailsCount; j++) {
			float Toch_sum = 0;
			int Nj = 0;
			for (int i = 0; i < m_OperationsCount; i++) {
				Toch_sum += m_Waiting[i][j];
				if (m_Waiting[i][j])
					Nj++;
			}
			if (Nj) {
				Toch_sum /= Nj;
				if (Toch_max < Toch_sum)Toch_max = Toch_sum;
			}
		}
		return Toch_max;
	}
	case 37:
	{
		float Toch_sum2 = 0;
		for (int j = 0; j < m_DetailsCount; j++) {
			float Toch_sum = 0;
			int Nj = 0;
			for (int i = 0; i < m_OperationsCount; i++) {
				Toch_sum += m_Waiting[i][j];
				if (m_Waiting[i][j])
					Nj++;
			}
			if (Nj) {
				Toch_sum /= Nj;
				Toch_sum2 += Toch_sum;
			}
		}
		return Toch_sum2;
	}
	default:
		throw std::runtime_error("Quality not implemented");
	}
}

int Timetable::ComputeTrk(int oper_id)
{
	int Trk = 0;
	for (auto& i : m_Timetable[oper_id]) {
		if (i.detail != -1) {
			Trk += i.duration;
		}
	}
	return Trk;
}

int Timetable::ComputeTprk(int oper_id)
{
	int Tprk = 0;
	for (auto& i : m_Timetable[oper_id]) {
		if (i.detail == -1) {
			Tprk += i.duration;
		}
	}
	return Tprk;
}

int Timetable::ComputeTprkBet(int oper_id)
{
	int Tprk = 0;
	for (auto& i : m_Timetable[oper_id]) {
		if (i.detail == -1 && i.start_time != 0) {
			Tprk += i.duration;
		}
	}
	return Tprk;
}

int Timetable::ComputePrCount(int oper_id)
{
	int Tprk = 0;
	for (auto& i : m_Timetable[oper_id]) {
		if (i.detail == -1) {
			Tprk++;
		}
	}
	return Tprk;
}

int Timetable::ComputePrCountBet(int oper_id)
{
	int Tprk = 0;
	for (auto& i : m_Timetable[oper_id]) {
		if (i.detail == -1 && i.start_time != 0) {
			Tprk++;
		}
	}
	return Tprk;
}

int Timetable::GetLastTime()
{
	return m_LastTime;
}
