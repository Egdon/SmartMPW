//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

const int INF = 0x3f3f3f3f;

using coord_t = int;

struct Config {
	unsigned int random_seed;
	int ub_time; // ASA��ʱʱ��
	int ub_iter; // RLS����������

	coord_t lb_width = 50, ub_width = 400;
	coord_t lb_height = 50, ub_height = 300;
};

