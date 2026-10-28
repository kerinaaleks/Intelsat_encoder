#include <iostream>
#include <locale.h>

using namespace std;

class PragmaticTCMEncoder {
private:
	uint8_t conv_state = 0; // 6 бит состояния сверточного кодера
	uint8_t diff_state = 0; // память диф.кодера перед светкой
	uint8_t u_state = 0; // память пути u
	uint8_t v_state = 0; // память пути v

	static constexpr uint8_t POLY_Q = 0b1011011; // 133(octal)
	static constexpr uint8_t POLY_R = 0b1111001; // 171(octal)

	// Параллельный подсчет четности (XOR всех единичных бит)
	// четное/нечетное кол-во бит
	static uint8_t parity(uint8_t x) {
		x ^= x >> 4; // сдвигаем число на 4 бита вправо и делаем иксор с самим собой
		x ^= x >> 2;
		x ^= x >> 1;
		return x & 1; // берем самый младший бит - это и есть результат
	}

	static uint8_t differentialEncoded(uint8_t bit, uint8_t& state) { // текущий бит и ссылка на состояние(предыдущий выход)
		uint8_t encoded = bit ^ state;
		state = encoded; // обновляем состояние 
		return encoded; // возвращаем закодированный бит
	}

	// Сверточный кодер
	void convolutionEncoded(uint8_t bit, uint8_t& Q, uint8_t& R) { // на вход поступает 1 бит, на выходе получается 2 бита Q и R
		uint8_t reg = (bit << 6) | conv_state; // собираем полный 7-битный регистр сдвигового регистра
		// новый бит ставился в самую левую(старшую) позицию, а старое состояние сдвигается вправо

		// вычисление выходов по генераторным полиномам
		Q = parity(reg & POLY_Q); // & - оставляет только те биты регистра, где в полиноме стоит 1
		R = parity(reg & POLY_R);

		conv_state = reg >> 1; // сдвигаем регистр вправо 
	}

public:
	// Функция кодирования одного символа 
	void encode(uint8_t bit1, uint8_t bit2, uint8_t& P, uint8_t& Q, uint8_t& R) { //  первый и второй инф. биты, выходные биты
		uint8_t diff_bit = differentialEncoded(bit2, diff_state); // диф. кодер
		convolutionEncoded(diff_bit, Q, R); // сверточный кодер

		if (Q == 0) { // в зависимости от значения Q выполняем диф.кодир.
			P = differentialEncoded(bit1, u_state);
		}
		else {
			P = differentialEncoded(bit1, v_state);
		}
	}
	// Функция обработки целого потока бит
	void encodeStream(const uint8_t* input, size_t input_len, uint8_t* output) {
		if (input_len % 2 != 0) { // проверяем кол-во бит на четность( тк берем попарам)
			throw invalid_argument("Кол-во бит должно быть четным");
		}

		size_t out_idx = 0;
		// идем по входному массиву с шагом 2
		for (size_t i = 0; i < input_len; i += 2) { 
			uint8_t P, Q, R;
			encode(input[i], input[i + 1], P, Q, R);// берем пару бит и кодируем их
			// записываем получившиеся на выходе биты в выходной массив
			output[out_idx++] = P;
			output[out_idx++] = Q;
			output[out_idx++] = R;
		}
	}
	// Функция обнуляет всю память кодера 
	void reset() {
		conv_state = 0;
		diff_state = 0; 
		u_state = 0; 
		v_state = 0; 
	}
};

int main()
{

	setlocale(LC_ALL, "Rus");
	PragmaticTCMEncoder encoder; 

	uint8_t input[] = {0, 1, 1, 0, 1, 1, 0, 0, 0, 1};


	size_t input_len = sizeof(input) / sizeof(input[0]); // сколько байт занимает весь массив/сколько байт занимае один элемент массива
	size_t size_output = (input_len / 2) * 3;
	uint8_t* output = new uint8_t [size_output];

	cout << "Вход: ";
	for (size_t i = 0; i < input_len; i++) {
		cout << (int)input[i] << " ";
	}

	encoder.encodeStream(input, input_len, output);

	cout << "\n";
	size_t num_symbol = input_len / 2;
	for (size_t i = 0; i < num_symbol; i++) {
		uint8_t P = output[i * 3 + 0];
		uint8_t Q = output[i * 3 + 1];
		uint8_t R = output[i * 3 + 2];
		uint8_t phase = (P << 2) | (Q << 1) | R; // (P двигаем на 2 влево, Q сдвигаем на 1 влево, R остается на своем месте) - обьединяем три сдвинутых бита в одну структуру

		cout << "Символ " << (i + 1) << ": " << (int)P << (int)Q << (int)R << " фаза " << (int)phase << "\n";
	}
	delete[] output;
	return 0;
}
