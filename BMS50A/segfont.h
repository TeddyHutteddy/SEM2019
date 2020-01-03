/*
 * segfont.h
 *
 * Created: 24/04/2019 11:13:33 AM
 *  Author: teddy
 */ 

#include <libmodule/ltd_2601g_11.h>

namespace segfont {
	using namespace libmodule::userio::ic_ldt_2601g_11_fontdata;
	constexpr ConstDigit english_const[] = {
		//Digits
		{'0', 1,
			1,  1,
			  0,
			1,  1,
			  1,  0 },
		{'1', 0,
			0,  1,
			  0,
			0,  1,
			  0,  0 },
		{'2', 1,
			0,  1,
			  1,
			1,  0,
			  1,  0 },
		{'3', 1,
			0,  1,
			  1,
			0,  1,
			  1,  0 },
		{'4', 0,
			1,  1,
			  1,
			0,  1,
			  0,  0 },
		{'5', 1,
			1,  0,
			  1,
			0,  1,
			  1,  0 },
		{'6', 1,
			1,  0,
			  1,
			1,  1,
			  1,  0 },
		{'7', 1,
			0,  1,
			  0,
			0,  1,
			  0,  0 },
		{'8', 1,
			1,  1,
			  1,
			1,  1,
			  1,  0 },
		{'9', 1,
			1,  1,
			  1,
			0,  1,
			  1,  0 },
		//Letters
		{'A', 1,
			1,  1,
			  1,
			1,  1,
			  0,  0 },
		{'b', 0,
			1,  0,
			  1,
			1,  1,
			  1,  0 },
		{'c', 0,
			0,  0,
			  1,
			1,  0,
			  1,  0 },
		{'C', 1,
			1,  0,
			  0,
			1,  0,
			  1,  0 },
		{'d', 0,
			0,  1,
			  1,
			1,  1,
			  1,  0 },
		{'E', 1,
			1,  0,
			  1,
			1,  0,
			  1,  0 },
		{'F', 1,
			1,  0,
			  1,
			1,  0,
			  0,  0 },
		{'g', 1,
			1,  1,
			  1,
			0,  1,
			  1,  0 },
		{'h', 0,
			1,  0,
			  1,
			1,  1,
			  0,  0 },
		{'H', 0,
			1,  1,
			  1,
			1,  1,
			  0,  0 },
		{'i', 0,
			0,  0,
			  0,
			0,  1,
			  0,  0 },
		{'I', 1,
			0,  1,
			  0,
			0,  1,
			  1,  0 },
		{'j', 0,
			0,  1,
			  0,
			1,  1,
			  1,  0 },
		{'K', 0,
			1,  0,
			  1,
			1,  0,
			  0,  0 },
		{'L', 0,
			1,  0,
			  0,
			1,  0,
			  1,  0 },
		{'M', 1,
			1,  1,
			  0,
			1,  1,
			  0,  0 },
		{'n', 0,
			0,  0,
			  1,
			1,  1,
			  0,  0 },
		{'o', 0,
			0,  0,
			  1,
			1,  1,
			  1,  0 },
		{'O', 1,
			1,  1,
			  0,
			1,  1,
			  1,  0 },
		{'p', 1,
			1,  1,
			  1,
			1,  0,
			  0,  0 },
		{'P', 1,
			1,  1,
			  1,
			1,  0,
			  0,  0 },
		{'q', 1,
			1,  1,
			  1,
			0,  1,
			  0,  0 },
		{'r', 0,
			0,  0,
			  1,
			1,  0,
			  0,  0 },
		{'S', 1,
			1,  0,
			  1,
			0,  1,
			  1,  0 },
		{'t', 0,
			1,  0,
			  1,
			1,  0,
			  1,  0 },
		{'u', 0,
			0,  0,
			  0,
			1,  1,
			  1,  0 },
		{'U', 0,
			1,  1,
			  0,
			1,  1,
			  1,  0 },
		{'v', 0,
			1,  1,
			  1,
			0,  0,
			  0,  0 },
		{'V', 0,
			1,  1,
			  0,
			1,  1,
			  1,  0 },
		{'W', 0,
			1,  1,
			  1,
			1,  1,
			  1,  0 },
		{'X', 0,
			1,  0,
			  1,
			0,  1,
			  0,  0 },
		{'y', 0,
			1,  1,
			  1,
			0,  1,
			  1,  0 },
		{'Z', 1,
			0,  1,
			  1,
			1,  0,
			  1,  0 },
		//Special characters
		{'-', 0,
			0,  0,
			  1,
			0,  0,
			  0,  0 },
		{'!', 0,
			0,  1,
			  0,
			0,  1,
			  0,  1 },
		{'?', 1,
			0,  1,
			  1,
			1,  0,
			  0,  1 }
	};
	//43 characters
	constexpr size_t english_len = sizeof english_const / sizeof(ConstDigit);

	template <size_t ...seq>
	struct english_serial_t {
		static SerialDigit const pgm_arr[sizeof...(seq)];
	};
	template <size_t ...seq>
	SerialDigit const english_serial_t<seq...>::pgm_arr[] PROGMEM = {conv_constdigit_to_serialdigit(english_const[seq])...};
	using english_serial = english_serial_t<0,1,2,3,4,5,6,7,8,9,
											10,11,12,13,14,15,16,17,18,19,
											20,21,22,23,24,25,26,27,28,29,
											30,31,32,33,34,35,36,37,38,39,
											40,41,42,43,44>;
	extern Font english_font;
}
