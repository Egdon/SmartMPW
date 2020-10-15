//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <list>
#include <random>
#include <numeric>

#include "Instance.hpp"

namespace mbp {

	using namespace std;

	class MpwBinPack {

	public:

		MpwBinPack() = delete;

		MpwBinPack(const Instance &ins, coord_t width, coord_t height, default_random_engine &gen)
			: _ins(ins), _src(ins.get_polygon_ptrs()), _bin_width(width), _bin_height(height), _gen(gen) {
			reset();
			init_sort_rules();
		}

		/// ����binWidth��������ֲ�����
		//void random_local_search(int iter) {
		//	// the first time to call RLS on W_k
		//	if (iter == 1) {
		//		for (auto &rule : _sort_rules) {
		//			_rects.assign(rule.sequence.begin(), rule.sequence.end());
		//			vector<Rect> target_dst;
		//			int target_area = insert_bottom_left_score(target_dst, level_gs) * binWidth;
		//			double target_wirelength = cal_wirelength(target_dst, level_wl);
		//			add_his_sol(target_area, target_wirelength);
		//			rule.target_objective = cal_objective(target_area, target_wirelength, alpha, beta, level_norm);
		//			check_rule_sol(rule, target_area, target_wirelength, target_dst);
		//		}
		//		// �������У�Խ�����Ŀ�꺯��ֵԽСѡ�и���Խ��
		//		sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
		//	}
		//	// �����Ż�
		//	SortRule &picked_rule = _sort_rules[_discrete_dist(_gen)];
		//	for (int i = 1; i <= iter; ++i) {
		//		SortRule new_rule = picked_rule;
		//		int a = _uniform_dist(_gen);
		//		int b = _uniform_dist(_gen);
		//		while (a == b) { b = _uniform_dist(_gen); }
		//		swap(new_rule.sequence[a], new_rule.sequence[b]);
		//		_rects.assign(new_rule.sequence.begin(), new_rule.sequence.end());
		//		vector<Rect> target_dst;
		//		int target_area = insert_bottom_left_score(target_dst, level_gs) * binWidth;
		//		double target_wirelength = cal_wirelength(target_dst, level_wl);
		//		add_his_sol(target_area, target_wirelength);
		//		new_rule.target_objective = min(new_rule.target_objective, cal_objective(target_area, target_wirelength, alpha, beta, level_norm));
		//		if (new_rule.target_objective < picked_rule.target_objective) {
		//			picked_rule = new_rule;
		//			check_rule_sol(picked_rule, target_area, target_wirelength, target_dst);
		//		}
		//	}
		//	// ������������б�
		//	sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
		//}

		coord_t insert_bottom_left_score(vector<polygon_ptr> &dst) {
			reset();
			dst.clear();
			dst.reserve(_polygons.size());

			while (!_polygons.empty()) {
				auto bottom_skyline_iter = min_element(_skyline.begin(), _skyline.end(),
					[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; });
				auto best_skyline_index = distance(_skyline.begin(), bottom_skyline_iter);

				polygon_ptr best_dst_node;
				size_t best_polygon_index = -1;
				if (find_polygon_for_skyline_bottom_left(best_skyline_index, _polygons, best_dst_node, best_polygon_index)) {
					assert(best_polygon_index != -1);
					_used_area += _src.at(best_polygon_index)->area;
					_polygons.remove(best_polygon_index);
					dst.push_back(best_dst_node);
				}
				else { // ���
					if (best_skyline_index == 0) { _skyline[best_skyline_index].y = _skyline[best_skyline_index + 1].y; }
					else if (best_skyline_index == _skyline.size() - 1) { _skyline[best_skyline_index].y = _skyline[best_skyline_index - 1].y; }
					else { _skyline[best_skyline_index].y = min(_skyline[best_skyline_index - 1].y, _skyline[best_skyline_index + 1].y); }
					merge_skylines(_skyline);
				}
			}

			assert(_used_area == _ins.get_total_area());
			//debug_run(utils_visualize_transform::dst_to_boxes(dst)); // ���ӻ�packing����
			return get_min_bin_height();
		}

	private:
		void reset() {
			_used_area = 0;
			_skyline.clear();
			_skyline.push_back({ 0,0,_bin_width });
		}

		coord_t get_min_bin_height() { return max_element(_skyline.begin(), _skyline.end(), [](auto &lhs, auto &rhs) { return lhs.y < rhs.y; })->y; }

		/// ���������
		struct SortRule {
			vector<size_t> sequence;
			coord_t target_area;
		};

		void init_sort_rules() {
			vector<size_t> seq(_src.size());
			// 0_����˳��
			iota(seq.begin(), seq.end(), 0);
			for (int i = 0; i < 5; ++i) { _sort_rules.push_back({ seq, numeric_limits<coord_t>::max() }); }
			// 1_����ݼ�
			sort(_sort_rules[1].sequence.begin(), _sort_rules[1].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs)->area > _src.at(rhs)->area; });
			// 2_�������
			shuffle(_sort_rules[4].sequence.begin(), _sort_rules[4].sequence.end(), _gen);

			// Ĭ������˳��
			_polygons.assign(_sort_rules[0].sequence.begin(), _sort_rules[0].sequence.end());

			// ��ɢ���ʷֲ���ʼ��
			vector<size_t> probs; probs.reserve(_sort_rules.size());
			for (size_t i = 1; i <= _sort_rules.size(); ++i) { probs.push_back(2 * i); }
			_discrete_dist = discrete_distribution<>(probs.begin(), probs.end());
		}

		/// ����������Ľ�ѡ����õĿ�
		bool find_polygon_for_skyline_bottom_left(size_t skyline_index, const list<size_t> &polygons,
			polygon_ptr &best_dst_node, size_t &best_polygon_index) {

			int best_score = -1;
			rect_ptr dst_rect;
			for (size_t p : polygons) {
				switch (_src.at(p)->shape()) {
				case Shape::R: {
					auto rect = dynamic_pointer_cast<rect_t>(_src.at(p));
					int score;
					coord_t x;
					for (int rotate = 0; rotate <= 1; ++rotate) {
						coord_t width = rect->width, height = rect->height;
						if (rotate) { swap(width, height); }
						if (score_rect_for_skyline_bottom_left(skyline_index, width, height, x, score)) {
							if (best_score < score) {
								best_score = score;
								best_polygon_index = p;
								dst_rect = make_shared<rect_t>(*rect);
								dst_rect->lb_point.x = x;
								dst_rect->lb_point.y = _skyline[skyline_index].y;
								dst_rect->rotation = rotate ? Rotation::_90_ : Rotation::_0_;
							}
						}
					}
					break;
				}
				case Shape::L: {
					auto lshape = dynamic_pointer_cast<lshape_t>(_src.at(p));
					int score;
					if (score_lshape_for_skyline_bottom_left(skyline_index, lshape, score)) {
						best_polygon_index = p;
						best_dst_node = make_shared<lshape_t>(*lshape);
						return true; // _skyline�ѱ�����
					}
					break;
				}
				case Shape::T: {
					auto tshape = dynamic_pointer_cast<tshape_t>(_src.at(p));
					int score;
					if (score_tshape_for_skyline_bottom_left(skyline_index, tshape, score)) {
						best_polygon_index = p;
						best_dst_node = make_shared<tshape_t>(*tshape);
						return true; // _skyline�ѱ�����
					}
					break;
				}
				default: { assert(false); break; }
				}
			}

			if (best_score == -1 || best_polygon_index == -1) { return false; }

			// ���е��˴�һ���Ǿ��Σ�����_skyline
			coord_t width = dst_rect->width, height = dst_rect->height;
			if (dst_rect->rotation == Rotation::_90_) { swap(width, height); }
			skylinenode_t new_skyline_node{ dst_rect->lb_point.x, dst_rect->lb_point.y + height, width };
			if (dst_rect->lb_point.x == _skyline[skyline_index].x) { // ����
				_skyline.insert(_skyline.begin() + skyline_index, new_skyline_node);
				_skyline[skyline_index + 1].x += width;
				_skyline[skyline_index + 1].width -= width;
				merge_skylines(_skyline);
			}
			else { // ����
				_skyline.insert(_skyline.begin() + skyline_index + 1, new_skyline_node);
				_skyline[skyline_index].width -= width;
				merge_skylines(_skyline);
			}
			best_dst_node = dst_rect;
			return true;
		}

		/// Space����
		struct SkylineSpace {
			coord_t x;
			coord_t y;
			coord_t width;
			coord_t hl;
			coord_t hr;
		};

		SkylineSpace skyline_nodo_to_space(size_t skyline_index) {
			coord_t hl, hr;
			if (_skyline.size() == 1) {
				hl = hr = _bin_height - _skyline[skyline_index].y;
			}
			else if (skyline_index == 0) {
				hl = _bin_height - _skyline[skyline_index].y;
				hr = _skyline[skyline_index + 1].y - _skyline[skyline_index].y;
			}
			else if (skyline_index == _skyline.size() - 1) {
				hl = _skyline[skyline_index - 1].y - _skyline[skyline_index].y;
				hr = _bin_height - _skyline[skyline_index].y;
			}
			else {
				hl = _skyline[skyline_index - 1].y - _skyline[skyline_index].y;
				hr = _skyline[skyline_index + 1].y - _skyline[skyline_index].y;
			}
			return { _skyline[skyline_index].x, _skyline[skyline_index].y, _skyline[skyline_index].width, hl, hr };
		}

		/// R��ֲ���
		bool score_rect_for_skyline_bottom_left(size_t skyline_index, coord_t width, coord_t height, coord_t &x, int &score) {
			if (width > _skyline[skyline_index].width) { return false; }
			if (_skyline[skyline_index].y + height > _bin_height) { return false; }

			SkylineSpace space = skyline_nodo_to_space(skyline_index);
			if (space.hl >= space.hr) {
				if (width == space.width && height == space.hl) { score = 7; }
				else if (width == space.width && height == space.hr) { score = 6; }
				else if (width == space.width && height > space.hl) { score = 5; }
				else if (width < space.width && height == space.hl) { score = 4; }
				else if (width == space.width && height < space.hl && height > space.hr) { score = 3; }
				else if (width < space.width && height == space.hr) { score = 2; } // ����
				else if (width == space.width && height < space.hr) { score = 1; }
				else if (width < space.width && height != space.hl) { score = 0; }
				else { return false; }

				if (score == 2) { x = _skyline[skyline_index].x + _skyline[skyline_index].width - width; }
				else { x = _skyline[skyline_index].x; }
			}
			else { // hl < hr
				if (width == space.width && height == space.hr) { score = 7; }
				else if (width == space.width && height == space.hl) { score = 6; }
				else if (width == space.width && height > space.hr) { score = 5; }
				else if (width < space.width && height == space.hr) { score = 4; } // ����
				else if (width == space.width && height < space.hr && height > space.hl) { score = 3; }
				else if (width < space.width && height == space.hl) { score = 2; }
				else if (width == space.width && height < space.hl) { score = 1; }
				else if (width < space.width && height != space.hr) { score = 0; } // ����
				else { return false; }

				if (score == 4 || score == 0) { x = _skyline[skyline_index].x + _skyline[skyline_index].width - width; }
				else { x = _skyline[skyline_index].x; }
			}
			if (x + width > _bin_width) { return false; }

			return true;
		}

		/// L��ֲ���
		bool score_lshape_for_skyline_bottom_left(size_t skyline_index, const lshape_ptr &lshape, int &score) {
			SkylineSpace space = skyline_nodo_to_space(skyline_index);
			skyline_t skyline_0 = _skyline, skyline_90 = _skyline, skyline_180 = _skyline, skyline_270 = _skyline;
			size_t skyline_old_size = _skyline.size();
			size_t min_delta = numeric_limits<size_t>::max();

			if (lshape->hd == space.width) {
				// lb_point
				point_t lb_point_0 = { skyline_0[skyline_index].x, skyline_0[skyline_index].y };
				// update
				skyline_0[skyline_index].y += lshape->vl;
				skyline_0[skyline_index].width = lshape->hu;
				// add
				skyline_0.insert(skyline_0.begin() + skyline_index + 1, {
					skyline_0[skyline_index].x + skyline_0[skyline_index].width,
					skyline_0[skyline_index].y - lshape->vm,
					lshape->hm });
				// merge
				merge_skylines(skyline_0);
				// delta
				if (min_delta > skyline_0.size() - skyline_old_size) {
					min_delta = skyline_0.size() - skyline_old_size;
					lshape->rotation = Rotation::_0_;
					lshape->lb_point = lb_point_0;
					_skyline = skyline_0;
				}
			}

			if (skyline_index < skyline_90.size() - 1
				&& lshape->vm <= skyline_90[skyline_index + 1].width
				&& lshape->hm == space.hr
				&& lshape->vr == space.width) {
				// lb_point
				point_t lb_point_90 = { skyline_90[skyline_index].x, skyline_90[skyline_index].y + lshape->hd };
				// update
				skyline_90[skyline_index].y += lshape->hd;
				skyline_90[skyline_index].width = lshape->vl;
				// delete right
				if (lshape->vm == skyline_90[skyline_index + 1].width) {
					skyline_90.erase(skyline_90.begin() + skyline_index + 1);
				}
				else if (lshape->vm < skyline_90[skyline_index + 1].width) {
					skyline_90[skyline_index + 1].x += lshape->vm;
					skyline_90[skyline_index + 1].width -= lshape->vm;
				}
				else { assert(false); }
				// merge
				merge_skylines(skyline_90);
				// delta
				if (min_delta > skyline_90.size() - skyline_old_size) {
					min_delta = skyline_90.size() - skyline_old_size;
					lshape->rotation = Rotation::_90_;
					lshape->lb_point = lb_point_90;
					_skyline = skyline_90;
				}
			}

			if (skyline_index >= 1
				&& lshape->hm <= skyline_180[skyline_index - 1].width
				&& lshape->vm == space.hl
				&& lshape->hu == space.width) {
				// lb_point
				point_t lb_point_180 = { skyline_180[skyline_index].x + lshape->hu, skyline_180[skyline_index].y + lshape->vl };
				// update
				skyline_180[skyline_index].x -= lshape->hm;
				skyline_180[skyline_index].y += lshape->vl;
				skyline_180[skyline_index].width = lshape->hd;
				// delete left
				if (lshape->hm == skyline_180[skyline_index - 1].width) {
					skyline_180.erase(skyline_180.begin() + skyline_index - 1);
				}
				else if (lshape->hm < skyline_180[skyline_index - 1].width) {
					skyline_180[skyline_index - 1].width -= lshape->hm;
				}
				else { assert(false); }
				// merge
				merge_skylines(skyline_180);
				// delta
				if (min_delta > skyline_180.size() - skyline_old_size) {
					min_delta = skyline_180.size() - skyline_old_size;
					lshape->rotation = Rotation::_180_;
					lshape->lb_point = lb_point_180;
					_skyline = skyline_180;
				}
			}

			if (lshape->vl == space.width) {
				// lb_point
				point_t lb_point_270 = { skyline_270[skyline_index].x + lshape->vl, skyline_270[skyline_index].y };
				// update
				skyline_270[skyline_index].y += lshape->hu;
				skyline_270[skyline_index].width = lshape->vm;
				// add
				skyline_270.insert(skyline_270.begin() + skyline_index + 1, {
					skyline_270[skyline_index].x + skyline_270[skyline_index].width,
					skyline_270[skyline_index].y + lshape->hm,
					lshape->vr });
				// merge
				merge_skylines(skyline_270);
				// delta
				if (min_delta > skyline_270.size() - skyline_old_size) {
					min_delta = skyline_270.size() - skyline_old_size;
					lshape->rotation = Rotation::_270_;
					lshape->lb_point = lb_point_270;
					_skyline = skyline_270;
				}
			}

			if (min_delta == numeric_limits<size_t>::max()) { return false; }
			score = 10;
			return true;
		}

		/// T��ֲ���
		bool score_tshape_for_skyline_bottom_left(size_t skyline_index, const tshape_ptr &tshape, int &score) {
			SkylineSpace space = skyline_nodo_to_space(skyline_index);
			skyline_t skyline_0 = _skyline, skyline_90 = _skyline, skyline_180 = _skyline, skyline_270 = _skyline;
			size_t skyline_old_size = _skyline.size();
			size_t min_delta = numeric_limits<size_t>::max();

			if (tshape->hd == space.width) {
				// lb_point
				point_t lb_point_0 = { skyline_0[skyline_index].x, skyline_0[skyline_index].y };
				// update
				skyline_0[skyline_index].y += tshape->vld;
				skyline_0[skyline_index].width = tshape->hl;
				// add
				skyline_0.insert(skyline_0.begin() + skyline_index + 1, {
					skyline_0[skyline_index].x + skyline_0[skyline_index].width,
					skyline_0[skyline_index].y + tshape->vlu,
					tshape->hu });
				skyline_0.insert(skyline_0.begin() + skyline_index + 2, {
					skyline_0[skyline_index + 1].x + skyline_0[skyline_index + 1].width,
					skyline_0[skyline_index + 1].y - tshape->vru,
					tshape->hr });
				// merge
				merge_skylines(skyline_0);
				// delta
				if (min_delta > skyline_0.size() - skyline_old_size) {
					min_delta = skyline_0.size() - skyline_old_size;
					tshape->rotation = Rotation::_0_;
					tshape->lb_point = lb_point_0;
					_skyline = skyline_0;
				}
			}

			if (skyline_index < skyline_90.size() - 1
				&& tshape->vru <= skyline_90[skyline_index + 1].width
				&& tshape->vrd == space.width
				&& tshape->hr == space.hr) {
				// lb_point
				point_t lb_point_90 = { skyline_90[skyline_index].x, skyline_90[skyline_index].y + tshape->hd };
				// update
				skyline_90[skyline_index].y += tshape->hd;
				skyline_90[skyline_index].width = tshape->vld;
				// add
				skyline_90.insert(skyline_90.begin() + skyline_index + 1, {
					skyline_90[skyline_index].x + skyline_90[skyline_index].width,
					skyline_90[skyline_index].y - tshape->hl,
					tshape->vlu });
				// delete right
				if (tshape->vru == skyline_90[skyline_index + 2].width) {
					skyline_90.erase(skyline_90.begin() + skyline_index + 2);
				}
				else if (tshape->vru < skyline_90[skyline_index + 2].width) {
					skyline_90[skyline_index + 2].x += tshape->vru;
					skyline_90[skyline_index + 2].width -= tshape->vru;
				}
				else { assert(false); }
				// merge
				merge_skylines(skyline_90);
				// delta
				if (min_delta > skyline_90.size() - skyline_old_size) {
					min_delta = skyline_90.size() - skyline_old_size;
					tshape->rotation = Rotation::_90_;
					tshape->lb_point = lb_point_90;
					_skyline = skyline_90;
				}
			}

			if (skyline_index < skyline_180.size() - 1 && skyline_index >= 1
				&& tshape->hl <= skyline_180[skyline_index + 1].width
				&& tshape->hr <= skyline_180[skyline_index - 1].width
				&& tshape->hu == space.width
				&& tshape->vlu == space.hr
				&& tshape->vru == space.hl) {
				// lb_point
				point_t lb_point_180 = { skyline_180[skyline_index].x + tshape->hu + tshape->hl, skyline_180[skyline_index].y + tshape->vlu + tshape->vld };
				// update
				skyline_180[skyline_index].x -= tshape->hr;
				skyline_180[skyline_index].y += (tshape->vru + tshape->vrd);
				skyline_180[skyline_index].width = tshape->hd;
				// delete left
				if (tshape->hr == skyline_180[skyline_index - 1].width) {
					skyline_180.erase(skyline_180.begin() + skyline_index - 1);
				}
				else if (tshape->hr < skyline_180[skyline_index - 1].width) {
					skyline_180[skyline_index - 1].width -= tshape->hr;
				}
				else { assert(false); }
				// delete right
				if (tshape->hl == skyline_180[skyline_index + 1].width) {
					skyline_180.erase(skyline_180.begin() + skyline_index + 1);
				}
				else if (tshape->hl < skyline_180[skyline_index + 1].width) {
					skyline_180[skyline_index + 1].x += tshape->hl;
					skyline_180[skyline_index + 1].width -= tshape->hl;
				}
				else { assert(false); }
				// merge
				merge_skylines(skyline_180);
				// delta
				if (min_delta > skyline_180.size() - skyline_old_size) {
					min_delta = skyline_180.size() - skyline_old_size;
					tshape->rotation = Rotation::_180_;
					tshape->lb_point = lb_point_180;
					_skyline = skyline_180;
				}
			}

			if (skyline_index >= 1
				&& tshape->vlu <= skyline_270[skyline_index - 1].width
				&& tshape->vld == space.width
				&& tshape->hl == space.hl) {
				// lb_point
				point_t lb_point_270 = { skyline_270[skyline_index].x + tshape->vld, skyline_270[skyline_index].y };
				// update
				skyline_270[skyline_index].x -= tshape->vlu;
				skyline_270[skyline_index].y += (tshape->hl + tshape->hu);
				skyline_270[skyline_index].width = tshape->vru;
				// add
				skyline_270.insert(skyline_270.begin() + skyline_index + 1, {
					skyline_270[skyline_index].x + skyline_270[skyline_index].width,
					skyline_270[skyline_index].y + tshape->hr,
					tshape->vrd });
				// delete left
				if (tshape->vlu == skyline_270[skyline_index - 1].width) {
					skyline_270.erase(skyline_270.begin() + skyline_index - 1);
				}
				else if (tshape->vlu < skyline_270[skyline_index - 1].width) {
					skyline_270[skyline_index - 1].width -= tshape->vlu;
				}
				else { assert(false); }
				// merge
				merge_skylines(skyline_270);
				// delta
				if (min_delta > skyline_270.size() - skyline_old_size) {
					min_delta = skyline_270.size() - skyline_old_size;
					tshape->rotation = Rotation::_270_;
					tshape->lb_point = lb_point_270;
					_skyline = skyline_270;
				}
			}

			if (min_delta == numeric_limits<size_t>::max()) { return false; }
			score = 20;
			return true;
		}

		/// ����skyline����֧�ֿ�skyline������
		static skyline_t add_skyline_level(size_t skyline_index, const skyline_t &old_skyline, const skylinenode_t &new_node) {
			skyline_t new_skyline = old_skyline;
			new_skyline.insert(new_skyline.begin() + skyline_index, new_node);

			for (size_t i = skyline_index + 1; i < new_skyline.size(); ++i) {
				assert(new_skyline[i - 1].x <= new_skyline[i].x);
				if (new_skyline[i].x < new_skyline[i - 1].x + new_skyline[i - 1].width) {
					coord_t shrink = new_skyline[i - 1].x + new_skyline[i - 1].width - new_skyline[i].x;
					new_skyline[i].x += shrink;
					new_skyline[i].width -= shrink;
					if (new_skyline[i].width <= 0) {
						new_skyline.erase(new_skyline.begin() + i);
						--i;
					}
					else { break; }
				}
				else { break; }
			}

			merge_skylines(new_skyline);
			return new_skyline;
		}

		/// �ϲ�ͬһlevel��skyline�ڵ�.
		static void merge_skylines(skyline_t &skyline) {
			for (size_t i = 0; i < skyline.size() - 1; ++i) {
				if (skyline[i].y == skyline[i + 1].y) {
					skyline[i].width += skyline[i + 1].width;
					skyline.erase(skyline.begin() + i + 1);
					--i;
				}
			}
		}

	private:
		// ����
		const Instance &_ins;
		const vector<polygon_ptr> &_src;
		coord_t _bin_width;
		coord_t _bin_height;

		// ���
		vector<polygon_ptr> _dst;
		coord_t _obj_area;

		// Packing
		skyline_t _skyline;
		coord_t _used_area;

		// �ֲ�����
		vector<SortRule> _sort_rules; // ��������б���������ֲ�����
		list<size_t> _polygons;		  // SortRule��sequence���൱��ָ�룬ʹ��list����ɾ�����������Ϊ��
		discrete_distribution<> _discrete_dist;   // ��ɢ���ʷֲ���������ѡ����(����ѡsequence����_polygons)
		uniform_int_distribution<> _uniform_dist; // ���ȷֲ������ڽ���˳��
		default_random_engine &_gen;

		// ���ֿ��ӻ� & �ص���飬��debug
		//debug_run(vector<utils_visualize::box_t> _dst_boxes;);
	};

}


