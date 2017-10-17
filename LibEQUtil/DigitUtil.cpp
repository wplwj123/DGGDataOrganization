#include "DigitUtil.h"

#include <string>
#include <bitset>

#include <cassert>

using std::string;
using std::bitset;

void eqtm::util::decimal_to_quaternary(EQ_UCHAR* quat, const EQ_ULLONG& decimal, const EQ_UINT& len) {
	EQ_UINT rem = 0, len_cpy = len;
	EQ_UINT len_counter = 0;
	EQ_ULLONG decimal_cpy = decimal;
	while (decimal_cpy != 0) {
		// use bit operation
		rem = decimal_cpy & 0x03;
		decimal_cpy = decimal_cpy >> 2;

		// check
		assert(len_cpy > 0);

		quat[--len_cpy] = '0' + rem;
		len_counter++;
	}

	EQ_UINT len_blank = len - len_counter;
	for (size_t i = 0; i < len_blank; ++i) {
		quat[i] = '0';
	}
	quat[len] = '\0';
}

EQ_ULLONG eqtm::util::quaternary_to_decimal(const EQ_UCHAR* quat) {
	assert(quat != nullptr);

	string quat_str = (char*)quat;
	EQ_UINT len = static_cast<EQ_UINT>(quat_str.size()); // not include '\0'

	EQ_ULLONG base = 4;

	EQ_ULLONG ret = 0;
	for (EQ_UINT i = 0; i < len; ++i) {
		EQ_ULLONG cur_quat_num = static_cast<EQ_ULLONG>(quat_str[i] - '0');
		ret += eq_pow(base, len - i - 1) * cur_quat_num;
	}

	return ret;
}

void eqtm::util::decimal_to_binary(EQ_UCHAR* bin, const EQ_ULLONG& decimal, const EQ_UINT& len) {
	EQ_UINT rem = 0, len_cpy = len;
	EQ_UINT len_counter = 0;
	EQ_ULLONG decimal_cpy = decimal;
	while (decimal_cpy != 0) {
		// use bit operation
		rem = decimal_cpy & 0x01;
		decimal_cpy = decimal_cpy >> 1;

		// check
		assert(len_cpy > 0);

		bin[--len_cpy] = '0' + rem;
		len_counter++;
	}

	EQ_UINT len_blank = len - len_counter;
	for (size_t i = 0; i < len_blank; ++i) {
		bin[i] = '0';
	}
	bin[len] = '\0';
}

EQ_ULLONG eqtm::util::binary_to_decimal(const EQ_UCHAR* bin) {
	assert(bin != nullptr);

	string bin_str = (char*)bin;
	EQ_UINT len = static_cast<EQ_UINT>(bin_str.size()); // not include '\0'

	EQ_ULLONG ret = 0;
	for (EQ_UINT i = 0; i < len; ++i) {
		EQ_ULLONG cur_bin_num = static_cast<EQ_ULLONG>(bin_str[i] - '0');
		ret += eq_pow(2, len - i - 1) * cur_bin_num;
	}

	return ret;
}

void eqtm::util::binary_to_ij(const EQ_UCHAR* bin,
	EQ_ULLONG& i, EQ_ULLONG& j) {
	assert(bin != nullptr);

	// bin char array reform
	string bin_str = (char*)bin;
	EQ_ULLONG bin_len = bin_str.size();

	string i_str, j_str;
	for (size_t pos = 0; pos < bin_len / 2; ++pos) {
		j_str.push_back(bin_str.at(2 * pos));
		i_str.push_back(bin_str.at(2 * pos + 1));
	}

	// bin to decimal
	i = binary_to_decimal((EQ_UCHAR*)i_str.c_str());
	j = binary_to_decimal((EQ_UCHAR*)j_str.c_str());
}

void eqtm::util::ij_to_binary(EQ_UCHAR* bin,
	const EQ_ULLONG& i, const EQ_ULLONG& j, const EQ_UINT& len) {
	// decimal to char array
	// 2^64 at 32 levels and ij have 32 bits
	string i_str = bitset<32>(i).to_string();
	string j_str = bitset<32>(j).to_string();

	// bin char array reform
	for (size_t k = 0; k < len / 2; ++k) {
		EQ_ULLONG cur_pos = 32 - len / 2 + k;

		// check
		assert(cur_pos >= 0);

		bin[2 * k] = j_str.at(cur_pos);
		bin[2 * k + 1] = i_str.at(cur_pos);
	}
	bin[len] = '\0';
}