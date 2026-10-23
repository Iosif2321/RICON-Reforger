// claude, TOOL_ONLY: reference driver for unmodified NTIA LFMF and ITM sources (public domain, NTIA/ITS).
// Reads commands from stdin, one per line:
//   LFMF h_tx h_rx f_mhz P_tx_w N_s d_km eps sigma pol -> rc, basic transmission loss (dB), E, P_rx, method
//   ITM  h_tx h_rx f_mhz eps sigma pol N0 climate mdvar time loc sit pfl_csv_file row
//        -> ITM basic transmission loss A (dB), A_fs (dB), A_ref (dB), mode, warnings
//        pfl file: ITM format per line "np,xi,z0,...,z_np" (comma separated); row is 0-based line index
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "LFMF.h"
#include "itm.h"

int main()
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        if (cmd == "LFMF")
        {
            double htx, hrx, f, ptx, ns, d, eps, sigma;
            int pol;
            in >> htx >> hrx >> f >> ptx >> ns >> d >> eps >> sigma >> pol;
            ITS::Propagation::LFMF::Result r{};
            int rc = ITS::Propagation::LFMF::LFMF(htx, hrx, f, ptx, ns, d, eps, sigma, pol, r);
            std::printf("LFMF rc=%d A_btl=%.9f E=%.9f P_rx=%.9f method=%d\n", rc, r.A_btl__db, r.E_dBuVm, r.P_rx__dbm, (int)r.method);
        }
        else if (cmd == "ITM")
        {
            double htx, hrx, f, eps, sigma, n0, tm, lc, st;
            int pol, climate, mdvar, row;
            std::string file;
            in >> htx >> hrx >> f >> eps >> sigma >> pol >> n0 >> climate >> mdvar >> tm >> lc >> st >> file >> row;
            std::ifstream pf(file);
            std::string pl;
            for (int k = 0; k <= row && std::getline(pf, pl); k++) {}
            std::vector<double> pfl;
            std::istringstream ps(pl);
            std::string tok;
            while (std::getline(ps, tok, ','))
                if (!tok.empty()) pfl.push_back(std::stod(tok));
            double a = 0;
            long warnings = 0;
            IntermediateValues iv{};
            int rc = ITM_P2P_TLS_Ex(htx, hrx, pfl.data(), climate, n0, f, pol, eps, sigma, mdvar, tm, lc, st, &a, &warnings, &iv);
            std::printf("ITM rc=%d A=%.9f A_fs=%.9f A_ref=%.9f mode=%d warnings=0x%lx d_km=%.6f n=%zu\n", rc, a, iv.A_fs__db, iv.A_ref__db, iv.mode, warnings, iv.d__km, pfl.size());
        }
        std::fflush(stdout);
    }
    return 0;
}
