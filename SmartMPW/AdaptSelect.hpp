//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include "Instance.hpp"
#include "MpwBinPack.hpp"

using namespace mbp;

class AdaptSelect {
public:

	AdaptSelect() = delete;

	AdaptSelect(const Environment &env, const Config &cfg) :
		_env(env), _cfg(cfg), _ins(env), _gen(_cfg.random_seed),
		_obj_area(numeric_limits<coord_t>::max()) {}

	/// ��ѡ��ȶ���
	struct CandidateWidth {
		coord_t value;
		int iter;
		unique_ptr<MpwBinPack> mbp_solver; // ��ָ�룬����������ɵĿ���
	};

	void run() {

		_start = clock();

		vector<coord_t> candidate_widths = cal_candidate_widths_on_interval();

		// ��֧��ʼ��iter=1
		vector<CandidateWidth> cw_objs; cw_objs.reserve(candidate_widths.size());
		for (coord_t bin_width : candidate_widths) {
			cw_objs.push_back({ bin_width, 1, unique_ptr<MpwBinPack>(
				new MpwBinPack(_ins.get_polygon_ptrs(), bin_width, INF, _gen)) });
			cw_objs.back().mbp_solver->random_local_search(1);
			check_cwobj(cw_objs.back());
		}

		// �������У�Խ�����ѡ�и���Խ��
		sort(cw_objs.begin(), cw_objs.end(), [](auto &lhs, auto &rhs) {
			return lhs.mbp_solver->get_obj_area() > rhs.mbp_solver->get_obj_area(); });

		// ��ʼ����ɢ���ʷֲ�
		vector<int> probs; probs.reserve(cw_objs.size());
		for (int i = 1; i <= cw_objs.size(); ++i) { probs.push_back(2 * i); }
		discrete_distribution<> discrete_dist(probs.begin(), probs.end());

		// �����Ż� 
		while ((clock() - _start) / static_cast<double>(CLOCKS_PER_SEC) < _cfg.ub_time) {
			CandidateWidth &picked_width = cw_objs[discrete_dist(_gen)];
			picked_width.iter = min(2 * picked_width.iter, _cfg.ub_iter);
			picked_width.mbp_solver->random_local_search(picked_width.iter);
			check_cwobj(picked_width);
			sort(cw_objs.begin(), cw_objs.end(), [](auto &lhs, auto &rhs) {
				return lhs.mbp_solver->get_obj_area() > rhs.mbp_solver->get_obj_area(); });
		}
	}

	void record_sol(string sol_path) const {
		ofstream sol_file(sol_path);
		for (auto &dst_node : _dst) {
			sol_file << "In Polygon:" << endl;
			for (auto &point : *dst_node->in_points) { sol_file << "(" << point.x << "," << point.y << ")"; }
			sol_file << endl << "Polygon:" << endl;
			vis::bg::for_each_point(dst_node->ring, [&](vis::bg_point_t &point) {
				sol_file << "(" << point.x() << "," << point.y() << ")"; });
			sol_file << endl;
		}
	}

	void draw_html(string html_path) const {
		utils_visualize_drawer::Drawer html_drawer(html_path, _cfg.ub_width, _cfg.ub_height);
		for (auto &dst_node : _dst) {
			string polygon_str;
			vis::bg::for_each_point(dst_node->ring, [&](vis::bg_point_t &point) {
				polygon_str += to_string(point.x()) + "," + to_string(point.y()) + " "; });
			html_drawer.polygon(polygon_str);
		}
	}

	void record_log(string log_path) const {
		ofstream log_file(log_path, ios::app);
		log_file.seekp(0, ios::end);
		if (log_file.tellp() <= 0) {
			log_file << "Instance,"
				"ObjArea,FillRatio,WHRatio,"
				"Duration,RandomSeed"
				<< endl;
		}
		log_file << _env._ins_name << ","
			<< _obj_area << "," << _fill_ratio << "," << _wh_ratio << ","
			<< _duration << "," << _cfg.random_seed << endl;
	}

private:
	/// ������[lb_width, ub_width]�ڣ��Ⱦ�����ɺ�ѡ���
	vector<coord_t> cal_candidate_widths_on_interval(coord_t interval = 1) {
		vector<coord_t> candidate_widths;
		// [todo]
		//coord_t min_width = min(_cfg.lb_width, min_element());
		candidate_widths.reserve(_cfg.ub_width - _cfg.lb_width + 1);
		for (coord_t cw = _cfg.lb_width; cw <= _cfg.ub_width; cw += interval) {
			if (cw * _cfg.ub_height >= _ins.get_total_area()) { candidate_widths.push_back(cw); }
		}
		return candidate_widths;
	}

	/// ���cw_obj��RLS���
	void check_cwobj(const CandidateWidth &cw_obj) {
		if (1.0 * cw_obj.mbp_solver->get_obj_area() / cw_obj.value > _cfg.ub_height) {
			cw_obj.mbp_solver->reset_obj_area(); // ��ǰ�ⲻ�Ϸ�
		}
		if (cw_obj.mbp_solver->get_obj_area() < _obj_area) {
			_obj_area = cw_obj.mbp_solver->get_obj_area();
			_fill_ratio = 1.0 * _ins.get_total_area() / _obj_area;
			_wh_ratio = 1.0 * cw_obj.value * cw_obj.value / _obj_area;
			_dst = cw_obj.mbp_solver->get_dst();
			_duration = (clock() - _start) / static_cast<double>(CLOCKS_PER_SEC);
		}
	}

private:
	const Environment &_env;
	const Config &_cfg;

	const Instance _ins;
	default_random_engine _gen;
	clock_t _start;
	double _duration;

	coord_t _obj_area;
	double _fill_ratio;
	double _wh_ratio;
	vector<polygon_ptr> _dst;
};