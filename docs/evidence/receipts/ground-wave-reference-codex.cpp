// Независимая оболочка Codex для неизменённых исходников NTIA LFMF v1.1.
// Численные функции NTIA сюда не копируются; сборка находится только во временной папке.
// Строка stdin: id htx hrx fMHz watts Ns distanceKm epsilon sigma polarization.
// MSVC: /EHs, поскольку extern C API LFMF пропускает C++-исключения; /EHsc предполагает обратное.
#include "LFMF.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static void Number(double value) {
    if (std::isfinite(value)) std::cout << std::setprecision(17) << value;
    else std::cout << "null";
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream input(line);
        int id, polarization;
        double htx, hrx, frequency, power, refractivity, distance, epsilon, sigma;
        if (!(input >> id >> htx >> hrx >> frequency >> power >> refractivity >> distance >> epsilon >> sigma >> polarization))
            return 2;
        std::string extra;
        if (input >> extra) return 2;
        ITS::Propagation::LFMF::Result result{};
        try {
            int code = ITS::Propagation::LFMF::LFMF(htx, hrx, frequency, power, refractivity,
                                                   distance, epsilon, sigma, polarization, result);
            std::cout << "{\"id\":" << id << ",\"exception\":false,\"rc\":" << code
                      << ",\"method\":" << static_cast<int>(result.method) << ",\"loss\":";
            Number(result.A_btl__db);
            std::cout << ",\"field\":"; Number(result.E_dBuVm);
            std::cout << ",\"received\":"; Number(result.P_rx__dbm);
            std::cout << "}\n";
        } catch (...) {
            // Ошибка эталонной функции остаётся данными, а не потерей оставшихся строк проверки.
            std::cout << "{\"id\":" << id << ",\"exception\":true}\n";
        }
    }
}
