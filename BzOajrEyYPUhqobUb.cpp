#include <iostream>
#include <string>
#include <cstdint>
#include <bitset>
#include <cassert>
#include <algorithm>
#include <vector>
#include <utility>

// 基本的な考え方
// 端っこのビットの外側は全て端っこのビットで埋まっている (1010 なら ...111 [1010] 000... と考える)
// しかし、多くの部分で末端が0埋め扱いされているので、ところどころ挙動が怪しいはず

// TODO slow, implement following:
// reinterpret_cast + pext + bit shift
// maybe ZMM reg work (however not in STM F767)
// asciiからなら文字(c)ごとに穢い論理圧縮書けばもっと早くなりうる
std::uint32_t string_to_uint32(std::string const& ls_data, char const c) noexcept {
	assert(ls_data.size() == 32);
	std::uint32_t data = 0u;
	for (std::size_t i = 0; i < 32; i++) {
		data |= (ls_data[i] == c ? 1u : 0u) << i;
	}
	return data;
}

std::string uint32_to_string(std::uint32_t const data) noexcept {
	auto ls_data = std::bitset<32>(data).to_string();
	std::reverse(ls_data.begin(), ls_data.end());
	return ls_data;
}

// TODO maybe slow
std::uint32_t find_repetition(std::uint32_t const data, std::size_t const num) noexcept {
	std::uint32_t rep = data;
	std::size_t i = 0;
	for (; i < num; i++) {
		rep &= (rep >> 1);
	}
	for (; i > 0; i--) {
		rep |= (rep << 1);
	}
	return rep;
}

// TODO 適当にclass化しちくり
//std::vector<std::uint32_t> g_rep_vec;
std::vector<std::pair<int, int>> g_rep_vec;
std::vector<std::pair<int, int>> g_line_vec;	// TODO 絶対もっと速い構造ある
void initialize() {
	g_rep_vec.reserve(16);
	g_line_vec.reserve(16);		// こっちは多分もっと小さくて良い
}

// TODO slow, implement following:
// bit manipulation つながっている最初の1の塊だけ分離
// 0 -> 1を捕まえる
// least trailing pop bit
// 1 -> 0 を捕まえる
// least trailing pop bit
// うまいことmaskつくる
//std::vector<std::uint32_t> const& split_repetition(std::uint32_t const data) noexcept {
std::vector<std::pair<int, int>> const& split_repetition(std::uint32_t const data) noexcept {
	g_rep_vec.clear();
	std::uint32_t mask = 1u;
	for (int i = 0; mask; i++) {
		if (data & mask) {
			int j = i;
			for (; (data & mask); i++) { mask <<= 1; }
			//g_rep_vec.push_back(((1u << (i - j)) - 1) << j);
			g_rep_vec.push_back(std::pair<int, int>(j, i));
		}
		mask <<= 1;
	}
	return g_rep_vec;
}

std::vector<std::pair<int, int>> const& find_line_pattern(
	std::uint32_t const data, std::size_t const num_border, std::size_t const num_line) noexcept
{
	// require more than num_border 0-s
	std::uint32_t bound = find_repetition(~data, num_border);
	// require more than num_line 1-s
	std::uint32_t line = find_repetition(data, num_line);
	// line の両端が border に接しているかを確認し、接しているもののみ記録
	g_line_vec.clear();
	for (auto const& rep: split_repetition(line)) {
		// TODO 境界部の実装が怪しい、算術シフト左シフトが欲しい
		auto left = bound & (1u << (rep.first - 1));
		auto right = bound & (1u << (rep.second + 1));
		if (left and right) { // TODO 高速化の余地ばかり
			g_line_vec.push_back(rep);
		}
	}
	return g_line_vec;
}

// 101 -> 111 にする
std::uint32_t blur_pattern(std::uint32_t const data) noexcept {
	return data | ((data >> 1) & (data << 1));
}
// 0100 | 0010 を消す (あんまり自信ないが多分こんな感じで書ける)
std::uint32_t blur_pattern_weak(std::uint32_t const data) noexcept {
	auto data5 = ((data >> 2) | (data << 2)) | (data << 1) | (data >> 1);
	return data ^ (~data5);
}
// 10011 とか 11001 とかも塗りつぶす (あんまり自信ないが多分こんな感じで書ける)
std::uint32_t blur_pattern_strong(std::uint32_t const data) noexcept {
	auto data2 = data & (data >> 1);
	return data | ((data2 << 2) & (data >> 1)) | ((data2 >> 1) & (data << 1));
}

void find_lines(
	std::uint32_t const data, std::size_t const num_border, std::size_t const num_line) noexcept
{
	std::cout << "  data: " << uint32_to_string(data) << std::endl;
	for (auto const& l: find_line_pattern(data, num_border, num_line)) {
		std::cout << "  line: " << l.first << " <-> " << l.second << std::endl;
	}
}

int main()
{
	initialize();

	std::string ls_data_bin = "00011010000001010111111000000001";
	std::string ls_data_oct = "00034020000002130334433430000006";

	auto ls_data_bin_1 = string_to_uint32(ls_data_bin, '1');
	auto ls_data_bin_1_blur = blur_pattern(ls_data_bin_1);

	auto ls_data_oct_34 = string_to_uint32(ls_data_oct, '3') | string_to_uint32(ls_data_oct, '4');
	auto ls_data_oct_34_blur = blur_pattern(ls_data_oct_34);

	// XXX DEMO of split_repetition
	for (auto const& d: split_repetition(ls_data_bin_1)) {
		auto i = d.second;
		auto j = d.first;
		auto data = ((1u << (i - j)) - 1) << j;
		std::cout << uint32_to_string(data) << " of " << j << " <-> " << i << std::endl;
	}

	// main DEMO
	std::cout << "Trial 0:" << std::endl;
	find_lines(ls_data_bin_1, 2, 3);

	std::cout << "Trial 1:" << std::endl;
	find_lines(ls_data_bin_1_blur, 1, 3);

	std::cout << "Trial 2:" << std::endl;
	find_lines(ls_data_oct_34_blur, 2, 5);

	return 0;
}

